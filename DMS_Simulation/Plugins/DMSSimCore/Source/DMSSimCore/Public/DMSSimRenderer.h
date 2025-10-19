#pragma once
#include <fstream>
#include "Camera/CameraComponent.h"
#include "SceneView.h"
#include "UObject/Object.h"
#include "DMSSimRenderRequest.h"
#include "DMSSimConfig.h"
#include "DMSSimRenderer.generated.h"

class UWorld;

/**
 * @class UDMSSimRenderer
 * @brief The class is responsible for setting up the camera and rendering the scene into an offscreen surface.
 * Then the offscreen surface data is read and sent to VideoRecorder_, which, in its turn, saves frames into a video stream.
 * Besides this, the class stores ground truth data in a ground truth CSV file, GroundTruthStream_.
 * The object is automatically created at the start-up and terminates on the app exit.
 *
 * @var FDMSSimOccupant::World_                    Unreal World pointer
 * @var FDMSSimOccupant::RenderTarget_             Offscreen surface to render scene to
 * @var FDMSSimOccupant::SceneCapture_             Scene Capture component, needed for rendering into an offscreen surface 
 * @var FDMSSimOccupant::RenderRequestQueue_       Queue where render jobs for each frame are stored
 * @var FDMSSimOccupant::GroundTruthRequestQueue_  Queue where ground truth for each frame is stored
 * @var FDMSSimOccupant::VideoRecorder_            Object with its own thread that does actual video stream generation from rendered frames
 * @var FDMSSimOccupant::VideoRecorders_           Vector of all active recording threads, each scenario has one thread
 * @var FDMSSimOccupant::GroundTruthStream_        File where ground truth signals are recorded
 * @var FDMSSimOccupant::CurrentFrame_             Frame counter, used to skip some number of frames at the beginning of the simulation
 * @var FDMSSimOccupant::StartTime_                Time when the simulation has started
 * @var FDMSSimOccupant::CameraSetup_              Internal flag, true if camera was configured
 */
UCLASS()
class UDMSSimRenderer : public UObject
{
	GENERATED_BODY()
public:
	UDMSSimRenderer();
	virtual ~UDMSSimRenderer() { };

private:
	UWorld*                                   World_ = nullptr;
	UTextureRenderTarget2D*                   RenderTarget_ = nullptr;
	USceneCaptureComponent2D*                 SceneCapture_ = nullptr;
	TQueue<TSharedPtr<FDMSSimRenderRequest> > RenderRequestQueue_;
	TQueue<TSharedPtr<DMSSimGroundTruthFrame> > GroundTruthRequestQueue_;
	TSharedPtr<DMSSimGroundTruthFrame>        PrevGroundTruth_ = nullptr;
	TSharedPtr<FRunnable>                     VideoRecorder_ = nullptr;
	TArray<TSharedPtr<FRunnable>>             VideoRecorders_;
	int                                       ScenarioIdxPrev_ = -1;
	std::ofstream                             GroundTruthStream_;
	size_t                                    CurrentFrame_ = 0;
	float                                     StartTime_ = 0.0f;
	bool                                      CameraSetup_ = false;

	/**
	 * Initializes the renderer.
	 * Namely it sets a callback on the scene load event, UDMSSimRenderer::OnMapLoadFinished.
	 */
	void Initialize() { FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UDMSSimRenderer::OnMapLoadFinished); };

	/**
	 * Creates an offscreen render target and scene capture component.
	 * Sets callbacks on the begin / end events, UDMSSimRenderer::OnBeginFrame / UDMSSimRenderer::OnEndFrame.
	 */
	void OnMapLoadFinished(UWorld* NewWorld);

	/**
	 * Pauses the simulation, until next frame.
	 */
	void OnBeginFrame();

	/**
	 * Renders scene into the offscreen surface (calls RenderFrame) and resumes the simulation.
	 */
	void OnEndFrame();

	/**
	 * Configures camera according to the scenario.
	 */
	void SetupCamera(UCameraComponent* const Camera);

	/**
	 * Saves the ground truth data.
	 * Checks the render queue for ready frames.
	 * Sends the ready frames to the video recorder, VideoRecorder_.
	 */
	void RenderFrame();

	/*
	helper  function to initiate rendering of the current frame into an offscreen surface.
	and enqueue image, groundtruth and parser to the request queues
	*/
	void EnqueueRequests();

	/*helper function to create vidoe/image file prefix*/
	std::wstring GetVideoFileName();

	/*helper function to create csv file prefix*/
	std::wstring GetCsvFileName();

	/*
	 * Returns the number of frames in the render queue
	*/
	UFUNCTION(BlueprintCallable, Category = "DMSSimCore")
	static bool GetNumRemainingFramesInRenderQueue(int& NumRemainingFrames);
};
