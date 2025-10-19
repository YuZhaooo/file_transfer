#include "DMSSimRenderer.h"
#include "DMSSimConfig.h"
#include "DMSSimLog.h"
#include "DMSSimGroundTruthRecorder.h"
#include "DMSSimScenarioParser.h"
#include "DMSSimVideoEncoder.h"
#include "DMSSimVideoRecordingRunable.h"
#include "Camera/CameraActor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "DistanceFieldAtlas.h"
#include "DrawDebugHelpers.h"
#include "EngineModule.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "LegacyScreenPercentageDriver.h"
#include "ShaderCompiler.h"
#include "Misc/App.h"
#include <ctime>
#include <string>
#include <numeric>
#include <sstream>

namespace {
void SetPostProcessingParams(const DMSSimCamera& Camera, FPostProcessSettings& Settings) {
	if (Camera.GetFocalDistance() <= 0.0f) { return; }
	Settings.bOverride_DepthOfFieldFstop = 1;
	Settings.bOverride_DepthOfFieldMinFstop = 1;
	Settings.bOverride_DepthOfFieldBladeCount = 1;
	Settings.bOverride_DepthOfFieldFocalDistance = 1;
	Settings.DepthOfFieldFstop = Camera.GetMaxFStop();
	Settings.DepthOfFieldMinFstop = Camera.GetMinFStop();
	Settings.DepthOfFieldBladeCount = Camera.GetDiaphragmBladeCount();
	Settings.DepthOfFieldFocalDistance = Camera.GetFocalDistance();
}
} // anonymous namespace

UDMSSimRenderer::UDMSSimRenderer() {
	DMSSimLog::Info() << "Renderer Created" << FL;
	DMSSimConfig::Initialize();
	Initialize();
}

void UDMSSimRenderer::OnBeginFrame() {
	if (World_) {
		const auto Player = World_->GetFirstPlayerController();
		if (Player) { Player->SetPause(false); }
	}
}

void UDMSSimRenderer::OnEndFrame() {
	if (World_) {
		const auto Player = World_->GetFirstPlayerController();
		if (Player) {
			Player->SetPause(true);
			RenderFrame();
		}
		DMSSimConfig::SetNumRemainingFrames(
			std::accumulate(VideoRecorders_.begin(), VideoRecorders_.end(), 0, [](int sum, auto b) { return sum + static_cast<DMSSimVideoRecordingRunable*>(b.Get())->GetNumPendingFrames(); })
		);
	}
}

bool UDMSSimRenderer::GetNumRemainingFramesInRenderQueue(int& NumRemainingFrames) {
	NumRemainingFrames = DMSSimConfig::GetNumRemainingFrames();
	return true;
}

void UDMSSimRenderer::RenderFrame() {
	if (!CameraSetup_) { return; }
	if (++CurrentFrame_ <= NUMBER_OF_FRAMES_TO_SKIP) { return; }

	const float UnpausedTime = World_->UnpausedTimeSeconds;
	bool ScenarioChange = false;

	// Take next requests from the queues and pass to recording thread
	if (VideoRecorder_) {
		DMSSimVideoRecordingRunable* const VideoRecorder = static_cast<DMSSimVideoRecordingRunable*>(VideoRecorder_.Get());
		while (!RenderRequestQueue_.IsEmpty() && !GroundTruthRequestQueue_.IsEmpty()) {
			TSharedPtr<FDMSSimRenderRequest> NextRenderRequest;
			TSharedPtr<DMSSimGroundTruthFrame> NextGroundTruthRequest;
			RenderRequestQueue_.Peek(NextRenderRequest);
			GroundTruthRequestQueue_.Peek(NextGroundTruthRequest);
			NextRenderRequest->RenderFence.Wait(true);
			if (NextRenderRequest && NextRenderRequest->RenderFence.IsFenceComplete() && NextGroundTruthRequest) {
				ScenarioChange = ScenarioIdxPrev_ < NextGroundTruthRequest->Common.ScenarioIdx;
				if (!ScenarioChange) {
					VideoRecorder->AddFrame(NextRenderRequest->Image, NextGroundTruthRequest);
					VideoRecorder->IncPendingFrames();
				}
				ScenarioIdxPrev_ = NextGroundTruthRequest->Common.ScenarioIdx;
			}
			RenderRequestQueue_.Pop();
			GroundTruthRequestQueue_.Pop();
		}
		DMSSimConfig::UpdateDisplayedFrameTime(UnpausedTime - StartTime_);
		if (GroundTruthStream_.is_open()) { DMSSimGroundTruthRecorder::AddFrame(GroundTruthStream_, UnpausedTime - StartTime_, DMSSimConfig::GetGroundTruthFrame()); }
	}

	//end of scenario, 
	if (VideoRecorder_ && (ScenarioChange || !DMSSimConfig::IsRecording())) {
		DMSSimLog::Info() << "Renderer" << " -- " << "scenario stop" << FL;
		VideoRecorder_->Stop();
		VideoRecorder_ = nullptr;
		CurrentFrame_ = 0;
		if (GroundTruthStream_.is_open() && (ScenarioChange || !DMSSimConfig::IsRecording())) { GroundTruthStream_.close(); }
	}

	//start of scenario
	if (!VideoRecorder_ && DMSSimConfig::IsRecording()) {
		DMSSimLog::Info() << "Renderer" << " -- " << "scenario start" << FL;
		VideoRecorder_ = TSharedPtr<DMSSimVideoRecordingRunable>(new DMSSimVideoRecordingRunable(
			GetVideoFileName(), 
			DMSSimConfig::GetCamera().GetFrameWidth(),
			DMSSimConfig::GetCamera().GetFrameHeight(), 
			DMSSimConfig::GetCamera().GetFrameWidth(), 
			DMSSimConfig::GetCamera().GetFrameHeight(), 
			DMSSimConfig::GetCamera().GetFrameRate(), 
			DMSSimConfig::GetCamera().GetDepth16Bit(), 
			DMSSimConfig::GetCamera().GetNIR()
		));
		VideoRecorders_.Emplace(VideoRecorder_);
		StartTime_ = UnpausedTime;

		if (!GroundTruthStream_.is_open() && DMSSimConfig::GetCurrentScenarioParser()->GetCamera().GetCsvOut()) {
			//DMSSimConfig::ResetGroundTruthData();
			GroundTruthStream_.open(GetCsvFileName(), std::ios_base::out | std::ios_base::trunc);
			DMSSimGroundTruthRecorder::AddHeader(GroundTruthStream_);
		}
	}

	//remove all runnables that have finished and stop recording, if all runnables are finished
	VideoRecorders_.RemoveAll([](TSharedPtr<FRunnable> VideoRecorder) {	return static_cast<DMSSimVideoRecordingRunable*>(VideoRecorder.Get())->IsDone(); });
	if (VideoRecorders_.Num() == 0) {
		World_ = nullptr;
		FCoreDelegates::OnBeginFrame.RemoveAll(this);
		FCoreDelegates::OnEndFrame.RemoveAll(this);
		DMSSimLog::Info() << "Renderer" << " -- " << "Exit" << FL;
		FGenericPlatformMisc::RequestExit(false);
	}

	//add new image, ground truth and parser requests
	EnqueueRequests();
}

void UDMSSimRenderer::EnqueueRequests() {
	RenderTarget_->TargetGamma = GEngine->GetDisplayGamma();
	FTextureRenderTargetResource* const RenderTargetRes = RenderTarget_->GameThread_GetRenderTargetResource();

	struct FReadSurfaceContext {
		FRenderTarget* SrcRenderTarget;
		TArray<FColor>* OutData;
		FIntRect Rect;
		FReadSurfaceDataFlags Flags;
	};

	TSharedPtr<FDMSSimRenderRequest> RenderRequest(new FDMSSimRenderRequest);

	// Setup GPU command
	FReadSurfaceContext ReadSurfaceContext = {
		RenderTargetRes,
		&(*RenderRequest->Image),
		FIntRect(0,0, RenderTargetRes->GetSizeXY().X, RenderTargetRes->GetSizeXY().Y),
		FReadSurfaceDataFlags(RCM_UNorm, CubeFace_MAX)
	};

	ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
		[ReadSurfaceContext](FRHICommandListImmediate& RHICmdList) {
			//CreateAndInitSingleView(RHICmdList, ReadSurfaceContext.ViewFamily, ReadSurfaceContext.ViewInitOptions);
			RHICmdList.ReadSurfaceData(
				ReadSurfaceContext.SrcRenderTarget->GetRenderTargetTexture(),
				ReadSurfaceContext.Rect,
				*ReadSurfaceContext.OutData,
				ReadSurfaceContext.Flags
			);
		});

	RenderRequest->RenderFence.BeginFence();
	if (RenderRequest) {
		RenderRequestQueue_.Enqueue(RenderRequest);
		GroundTruthRequestQueue_.Enqueue(MakeShared<DMSSimGroundTruthFrame>(DMSSimConfig::GetGroundTruthFrame()));
	}
}

std::wstring UDMSSimRenderer::GetVideoFileName() {
	std::wstringstream ss;
	ss << DMSSimConfig::GetFilePrefix()<< L"_Scenario_" << std::setw(4) << std::setfill(L'0') << (DMSSimConfig::GetGroundTruthFrame().Common.ScenarioIdx);
	return ss.str();
}

std::wstring UDMSSimRenderer::GetCsvFileName() {
	std::wstringstream ss;
	ss << DMSSimConfig::GetFilePrefix() << L"_Scenario_" << std::setw(4) << std::setfill(L'0') << (DMSSimConfig::GetGroundTruthFrame().Common.ScenarioIdx) << L".csv";
	return ss.str();
}

void UDMSSimRenderer::SetupCamera(UCameraComponent* const Camera) {
	if (!Camera) return;
	const auto& CameraInfo = DMSSimConfig::GetCamera();
	FMinimalViewInfo CameraView = {};
	Camera->GetCameraView(0, CameraView);

	if (CameraInfo.GetFrameHeight() > 0 && CameraView.AspectRatio > 0.0f) {
		const float TargetFOV = CameraInfo.GetFOV();
		const float TargetAspectRatio = float(CameraInfo.GetFrameWidth()) / float(CameraInfo.GetFrameHeight());

		if (!CameraSetup_) {
			const float OldFOV = CameraView.FOV;
			const float OldAspectRatio = CameraView.AspectRatio;
			const float NewFOV = FMath::RadiansToDegrees(2.0f * atan((OldAspectRatio / TargetAspectRatio) * tan(FMath::DegreesToRadians(TargetFOV / 2.0f))));

			SetPostProcessingParams(CameraInfo, Camera->PostProcessSettings);

			Camera->SetRelativeLocation(CameraInfo.GetPosition());
			Camera->SetRelativeRotation(CameraInfo.GetRotation());
			Camera->SetFieldOfView(NewFOV);
			Camera->PostLoad();
			Camera->GetCameraView(0, CameraView);
			CameraSetup_ = true;
		}
		CameraView.FOV = TargetFOV;
		CameraView.AspectRatio = TargetAspectRatio;

		SceneCapture_->SetCameraView(CameraView);
		SceneCapture_->PostProcessBlendWeight = CameraView.PostProcessBlendWeight;
		SceneCapture_->PostProcessSettings = CameraView.PostProcessSettings;

		SetPostProcessingParams(CameraInfo, SceneCapture_->PostProcessSettings);
	}
}

void UDMSSimRenderer::OnMapLoadFinished(UWorld* World) {
	if (World == nullptr) return;

	const bool FirstMap = !World_;
	World_ = World;
	bool test = false;
	const auto& Camera = DMSSimConfig::GetCamera();

	/* force fixed frame rate */
	FFrameRate FrameRate(Camera.GetFrameRate(), 1);
	FApp::SetFixedDeltaTime(FrameRate.AsInterval());
	FApp::SetUseFixedTimeStep(true);

	//DrawDebugSphere(World_, CameraActor->GetActorLocation(), 50, 32, FColor::Yellow, false, 10.0f);
	ASceneCapture2D* const SceneCaptureActor = (ASceneCapture2D*)World_->SpawnActor<ASceneCapture2D>(ASceneCapture2D::StaticClass());
	//SceneCaptureActor->AttachToActor(CameraActor, FAttachmentTransformRules::SnapToTargetIncludingScale);

	SceneCapture_ = SceneCaptureActor->GetCaptureComponent2D();

	RenderTarget_ = NewObject<UTextureRenderTarget2D>();
	RenderTarget_->TargetGamma = 1.2f;// for Vulkan //GEngine->GetDisplayGamma(); // for DX11/12

	// Setup the RenderTarget capture format
	RenderTarget_->InitAutoFormat(256, 256); // some random format, got crashing otherwise
	RenderTarget_->InitCustomFormat(Camera.GetFrameWidth(), Camera.GetFrameHeight(), PF_B8G8R8A8, true); // PF_B8G8R8A8 disables HDR which will boost storing to disk due to less image information
	RenderTarget_->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
	RenderTarget_->bGPUSharedFlag = true; // demand buffer on GPU

	SceneCapture_->TextureTarget = RenderTarget_;
	SceneCapture_->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneCapture_->ShowFlags.SetTemporalAA(true);
	SceneCapture_->RegisterComponent();

	if (FirstMap) {
		FCoreDelegates::OnBeginFrame.AddUObject(this, &UDMSSimRenderer::OnBeginFrame);
		FCoreDelegates::OnEndFrame.AddUObject(this, &UDMSSimRenderer::OnEndFrame);
		DMSSimConfig::SetCameraCallback([this](UCameraComponent* const Camera){ this->SetupCamera(Camera); });
	}
}