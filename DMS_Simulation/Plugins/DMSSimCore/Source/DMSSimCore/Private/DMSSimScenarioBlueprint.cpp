#include "DMSSimScenarioBlueprint.h"
#include "DMSSimAnimationFilter.h"
#include "DMSSimAnimationBuilder.h"
#include "DMSSimConfig.h"
#include "DMSSimLog.h"
#include "DMSSimCurveGenerator.h"
#include "DMSSimMontageBuilder.h"
#include "DMSSimOrchestrator.h"
#include "DMSSimScenarioParserUtils.h"
#include "DMSSimUtils.h"
#include "DMSSimConstants.h"
#include <stdexcept>
#include <algorithm>
#include <random>
#include <memory>
#include <chrono>


namespace {
using TCurve = DMSSimCurveGenerator::TCurve;

// Global variable to enable getting value in Face_PostProcess_AnimBP
static FDMSRandomMovements RandomMovementsStatus = {};
// Global generator for random numbers (would be expensive to create new every time)
static std::unique_ptr<std::mt19937> RandomGenerator;

class DMSSimScenarioParserWrapper {
public:
	DMSSimScenarioParserWrapper(const FString& Path, FString& ErrorMessage, const size_t index): ParserObj_(DMSSimConfig::GetScenarioParser(index)) {
		ErrorMessage = DMSSimConfig::GetScenarioParserErrorMessage().c_str();
		if (!ParserObj_) {
			auto& FileManager = FPlatformFileManager::Get().GetPlatformFile();

			DMSSimLog::Info() << "Trying to load " << *Path << FL;
			// Currently this will only allow to have one scenario file being loaded during editor play. Which is what is wanted.
			if (FileManager.FileExists(*Path)) {
				DMSSimLog::Info() << "Loading scenario: " << Path << FL;
				DMSSimConfig::AddScenarioParser(CreateScenarioParser(*Path));
			} else if (FileManager.DirectoryExists(*Path)) {
				DMSSimLog::Info() << "Loading all scenarios from directory: " << Path << FL;
				DMSSimConfig::SetScenarioParsers(CreateScenarioParsers(*Path));
			} else {
				FString ErrorMessage = FString::Printf(TEXT("Error: File path is invalid. Path: %s"), *Path);
				throw std::runtime_error(TCHAR_TO_UTF8(*ErrorMessage));
			}

			ParserObj_ = DMSSimConfig::GetScenarioParser(index);
			if (!ParserObj_) {
				FString ErrorMessage = FString::Printf(TEXT("Scenario index is higher than the loaded scenarios!"));
				throw std::runtime_error(TCHAR_TO_UTF8(*ErrorMessage));
			}
		}

		DMSSimConfig::SetCurrentScenarioParser(ParserObj_);
	}

	operator const DMSSimScenarioParser*() const { return ParserObj_.Get(); }
	const DMSSimScenarioParser* operator->() const { return ParserObj_.Get(); }

private:
	//const DMSSimScenarioParser*      Parser_;
	TSharedPtr<DMSSimScenarioParser> ParserObj_;
};

struct MontageBuilderEnvironment : public DMSSimMontageBuilder::TEnvironment {
	virtual UAnimMontage* CreateMontage() override;
	virtual UAnimSequenceBase* LoadAnimationSequence(DMSSimAnimationChannelType TargetChannel, const char* Name, const char* Path, bool* ExactMatch) override;
	virtual FSlotAnimationTrack* GetMontageSlot(DMSSimAnimationChannelType TargetChannel, UAnimMontage* Montage) override;
	virtual void AddMontageSection(DMSSimAnimationChannelType TargetChannel, UAnimMontage* Montage) override;
	virtual std::vector<std::string> GetNotifyEvents(UAnimSequenceBase* Sequence) override;
};

UAnimMontage* MontageBuilderEnvironment::CreateMontage() { return NewObject<UAnimMontage>(); }

UAnimSequenceBase* MontageBuilderEnvironment::LoadAnimationSequence(const DMSSimAnimationChannelType TargetChannel, const char* const Name, const char* const Path, bool* const ExactMatch) {
	return DMSSimAnimationBuilder::LoadAnimationSequence(TargetChannel, Name, Path, ExactMatch);
}

FSlotAnimationTrack* MontageBuilderEnvironment::GetMontageSlot(const DMSSimAnimationChannelType TargetChannel, UAnimMontage* const Montage) {
	if (TargetChannel == DMSSimAnimationChannelCommon) { return Montage->SlotAnimTracks.FindByPredicate([](const FSlotAnimationTrack& track) { return track.SlotName == FAnimSlotGroup::DefaultSlotName; }); }

	static const struct {
		DMSSimAnimationChannelType Channel;
		const char* Name;
	} SlotInfoList[] = {
		{ DMSSimAnimationChannelEyeGaze,   "EyeGazeSlot"   },
		{ DMSSimAnimationChannelEyelids,   "EyelidsSlot"   },
		{ DMSSimAnimationChannelFace,      "FaceSlot"      },
		{ DMSSimAnimationChannelHead,      "HeadSlot"      },
		{ DMSSimAnimationChannelUpperBody, "UpperBodySlot" },
		{ DMSSimAnimationChannelLeftHand,  "LeftHandSlot"  },
		{ DMSSimAnimationChannelRightHand, "RightHandSlot" },
	};

	for (const auto SlotInfo : SlotInfoList) {
		if (SlotInfo.Channel == TargetChannel) {
			for (auto& Track : Montage->SlotAnimTracks) {
				if (Track.SlotName == SlotInfo.Name) { return &Track; }
			}
			return &Montage->AddSlot(SlotInfo.Name);
		}
	}
	check(0);
	return nullptr;
}

void MontageBuilderEnvironment::AddMontageSection(const DMSSimAnimationChannelType TargetChannel, UAnimMontage* Montage) {
	static const struct {
		DMSSimAnimationChannelType Channel;
		const char* Name;
	} SectionInfoList[] = {
		{ DMSSimAnimationChannelCommon,    "Default"   },
		{ DMSSimAnimationChannelEyeGaze,   "EyeGaze"   },
		{ DMSSimAnimationChannelEyelids,   "Eyelids"   },
		{ DMSSimAnimationChannelFace,      "Face"      },
		{ DMSSimAnimationChannelHead,      "Head"      },
		{ DMSSimAnimationChannelUpperBody, "UpperBody" },
		{ DMSSimAnimationChannelLeftHand,  "LeftHand"  },
		{ DMSSimAnimationChannelRightHand, "RightHand" },
	};

	for (const auto SectionInfo : SectionInfoList) {
		if (SectionInfo.Channel == TargetChannel) {
			FCompositeSection Section;
			Section.SectionName = SectionInfo.Name;
			Montage->CompositeSections.Add(std::move(Section));
			return;
		}
	}
	check(0);
}

std::vector<std::string> MontageBuilderEnvironment::GetNotifyEvents(UAnimSequenceBase* Sequence) {
	std::vector<std::string> Events;
	const auto& Notifies = Sequence->Notifies;
	for (const auto& Event : Notifies) {
		const auto Name = Event.NotifyName.ToString();
		if (!Name.IsEmpty()) { Events.emplace_back(WideToNarrow(FStringToWide(Name).c_str())); }
	}
	return Events;
}

float ComputeTotalAnimationChannelTime(const TArray<FDMSSimMontage>& Channel) {
	float Time = 0.0f;
	for (const auto& MontageInfo : Channel) { Time += MontageInfo.Time; }
	return Time;
}

void AddCurveToMontageList(TArray<TCurve>& Curves, TArray<FDMSSimMontage>& MontageList, bool Last) {
	if (Curves.Num() > 0) {
		float CurveTime = 0;
		UCurveVector* const CurveVector = DMSSimCurveGenerator::Generate(Curves, CurveTime);
		float BlendIn = 0;
		float BlendOut = 0;

		if (MontageList.Num() > 0) { BlendIn = MontageList.Last().BlendOut; }
		if (!Last) {
			BlendOut = Curves.Last().BlendOut;
			CurveTime = std::max(0.0f, CurveTime - BlendOut);
		}
		MontageList.Push(FDMSSimMontage{ nullptr, CurveTime, BlendIn, BlendOut, CurveVector, false });
		Curves.Empty();
	}
}

UAnimMontage* FilterEmptyMontage(UAnimMontage* const Montage) {
	if (Montage) {
		for (const auto& Track : Montage->SlotAnimTracks) {
			if (Track.AnimTrack.AnimSegments.Num() > 0) {
				Montage->AddToRoot(); // prevent garbage collection
				return Montage;
			}
		}
	}
	return nullptr;
}

bool BuildChannelMontageList(const DMSSimScenarioParser* const Parser, const DMSSimAssetRegistry* const AssetRegistry, const FDMSSimOccupantType Occupant, const DMSSimAnimationChannelType Channel, USkeleton* const Skeleton, TArray<FDMSSimMontage>& MontageList, float& TotalTime) {
	TArray<DMSSimMontageBuilder::TMontage> MontageListTmp;
	MontageBuilderEnvironment Environment;
	const bool Result = DMSSimMontageBuilder::Build(Environment, Parser, AssetRegistry, Occupant, Channel, Skeleton, MontageListTmp);
	if (Result) {
		TArray<TCurve> Curves;
		for (const auto& Montage : MontageListTmp) {
			if (!Montage.Montage) { Curves.Push(Montage.Curve); }
			else {
				AddCurveToMontageList(Curves, MontageList, false);
				MontageList.Push(FDMSSimMontage{ FilterEmptyMontage(Montage.Montage), Montage.Time, Montage.BlendIn, Montage.BlendOut, nullptr, true });
			}
		}
		AddCurveToMontageList(Curves, MontageList, true);
	}
	TotalTime = std::max(TotalTime, ComputeTotalAnimationChannelTime(MontageList));
	return Result;
}
} // anonymous namespace

bool UDMSSimScenarioBlueprint::IsDmsExecutable() { return (WITH_EDITOR)? false : true; }

void UDMSSimScenarioBlueprint::ResetScenarios()
{
	DMSSimConfig::ResetScenarioParsers();
	RandomMovementsStatus = {};
}

bool UDMSSimScenarioBlueprint::LoadDmsScenarioMulti(const FString& Path, const int32& ScenarioIndex, TArray<FDMSSimOccupant>& Occupants, FString& ErrorMessage, FDMSScenario& Scenario, float& CarSpeed) {
	DMSSimLog::Info() << "Scenario Blueprint: " << __func__ << FL;
	try {
		DMSSimScenarioParserWrapper Parser(Path, ErrorMessage, ScenarioIndex); 
		if (!Parser) { return false; }

		TSharedPtr<DMSSimAssetRegistry> AssetRegistry(DMSSimAssetRegistry::Create());
		if (!AssetRegistry) {
			DMSSimLog::Info() << "Failed to load asset registry: " << __func__ << " - " << __FILE__ << ": " << __LINE__ << FL;
			return false;
		}

		const auto OccupantCount = Parser->GetOccupantCount();
		for (size_t i = 0; i < OccupantCount; ++i) {
			const auto& OccupantInfo = Parser->GetOccupant(i);
			const auto OccupantType = OccupantInfo.GetType();

			DMSSimOrchestrator Orchestrator;
			if (!Orchestrator.Initialize(Parser, AssetRegistry.Get(), OccupantType)) { return false; }

			FDMSSimOccupant Occupant;
			Occupant.Type = OccupantType;
			Occupant.Character = Orchestrator.GetCharacter();
			Occupant.Uppercloth = Orchestrator.GetUppercloth();
			Occupant.Headgear = Orchestrator.GetHeadgear();
			Occupant.Glasses.Model = Orchestrator.GetGlasses();
			Occupant.Glasses.Color = Orchestrator.GetGlassesColor();
			Occupant.Glasses.Opacity = Orchestrator.GetGlassesOpacity();
			Occupant.Glasses.Reflective = Orchestrator.GetGlassesReflective();
			Occupant.Mask = Orchestrator.GetMask();
			Occupant.Scarf = Orchestrator.GetScarf();
			Occupant.Hair = Orchestrator.GetHair();
			Occupant.Beard = Orchestrator.GetBeard();
			Occupant.Mustache = Orchestrator.GetMustache();
			Occupant.PupilSize = Orchestrator.GetPupilSize();
			Occupant.PupilBrightness = Orchestrator.GetPupilBrightness();
			Occupant.IrisSize = Orchestrator.GetIrisSize();
			Occupant.IrisBrightness = Orchestrator.GetIrisBrightness();
			Occupant.IrisBorderWidth = Orchestrator.GetIrisBorderWidth();
			Occupant.LimbusDarkAmount = Orchestrator.GetLimbusDarkAmount();
			Occupant.IrisColor = Orchestrator.GetIrisColor();
			Occupant.ScleraBrightness = Orchestrator.GetScleraBrightness();
			Occupant.ScleraVeins = Orchestrator.GetScleraVeins();
			Occupant.SkinWrinkles = Orchestrator.GetSkinWrinkles();
			Occupant.SkinRoughness = Orchestrator.GetSkinRoughness();
			Occupant.SkinSpecularity = Orchestrator.GetSkinSpecularity();
			Occupant.Height = Orchestrator.GetHeight();
			Occupant.SeatOffset = Orchestrator.GetSeatOffset();
			Occupants.Push(Occupant);
		}

		Scenario.ScenarioIdx = ScenarioIndex;
		Scenario.ScenariosCount = DMSSimConfig::GetScenarioParsersCount();
		Scenario.VersionMajor = Parser->GetVersionMajor();
		Scenario.VersionMinor = Parser->GetVersionMinor();
		Scenario.Description = Parser->GetDescription() == nullptr ? "" : Parser->GetDescription();
		Scenario.Environment = Parser->GetEnvironment() == nullptr ? "" : Parser->GetEnvironment();
		Scenario.RandomMovements.Smiling = Parser->GetRandomSmiling();
		Scenario.RandomMovements.Blinking = Parser->GetRandomBlinking();
		Scenario.RandomMovements.Head = Parser->GetRandomHeadMovements();
		Scenario.RandomMovements.Body = Parser->GetRandomBodyMovements();
		Scenario.RandomMovements.Gaze= Parser->GetRandomGaze();
		RandomMovementsStatus = Scenario.RandomMovements;
		Scenario.CarModel = Parser->GetCarModel() == nullptr ? "" : Parser->GetCarModel();
		Scenario.CarSpeed = Parser->GetCarSpeed();
		Scenario.SunLight.Rotation = Parser->GetSunRotation();
		Scenario.SunLight.Intensity = Parser->GetSunIntensity();
		Scenario.SunLight.Temperature = Parser->GetSunTemperature();
		Scenario.BlendoutDefaults.Common = Parser->GetDefaultBlendOut(DMSSimAnimationChannelCommon);
		Scenario.BlendoutDefaults.EyeGaze = Parser->GetDefaultBlendOut(DMSSimAnimationChannelEyeGaze);
		Scenario.BlendoutDefaults.Eyelids = Parser->GetDefaultBlendOut(DMSSimAnimationChannelEyelids);
		Scenario.BlendoutDefaults.Face = Parser->GetDefaultBlendOut(DMSSimAnimationChannelFace);
		Scenario.BlendoutDefaults.Head= Parser->GetDefaultBlendOut(DMSSimAnimationChannelHead);
		Scenario.BlendoutDefaults.UpperBody= Parser->GetDefaultBlendOut(DMSSimAnimationChannelUpperBody);
		Scenario.BlendoutDefaults.LeftHand = Parser->GetDefaultBlendOut(DMSSimAnimationChannelLeftHand);
		Scenario.BlendoutDefaults.RightHand = Parser->GetDefaultBlendOut(DMSSimAnimationChannelRightHand);
		Scenario.BlendoutDefaults.SteeringWheel = Parser->GetDefaultBlendOut(DMSSimAnimationChannelSteeringWheel);
 
		const auto& CameraIllumination = Parser->GetCameraIllumination();
		Scenario.CameraLight.Intensity = CameraIllumination.GetIntensity();
		Scenario.CameraLight.AttenuationRadius = CameraIllumination.GetAttenuationRadius();
		Scenario.CameraLight.SourceRadius = CameraIllumination.GetSourceRadius();
		Scenario.CameraLight.InnerConeAngle = CameraIllumination.InnerConeAngle();
		Scenario.CameraLight.OuterConeAngle = CameraIllumination.OuterConeAngle();
		Scenario.CameraLight.Position= CameraIllumination.GetPosition();
		Scenario.CameraLight.Rotation = CameraIllumination.GetRotation();

		const auto& SteeringWheelColumn = Parser->GetSteeringWheelColumn();
		Scenario.SteeringWheelColumn.IsCameraIntegrated = SteeringWheelColumn.GetIsCameraIntegrated();
		Scenario.SteeringWheelColumn.PitchAngle = SteeringWheelColumn.GetPitchAngle();

		const auto& GroundTruthSettings = Parser->GetGroundTruthSettings();
		Scenario.GroundTruthSettings.EyeBoundingBoxDepth = GroundTruthSettings.GetEyeBoundingBoxDepth();
		Scenario.GroundTruthSettings.EyeBoundingBoxHeightFactor = GroundTruthSettings.GetEyeBoundingBoxHeightFactor();
		Scenario.GroundTruthSettings.EyeBoundingBoxWidthFactor = GroundTruthSettings.GetEyeBoundingBoxWidthFactor();
		Scenario.GroundTruthSettings.BoundingBoxPaddingFactorFace = GroundTruthSettings.GetBoundingBoxPaddingFactorFace();

		// NOTE: We are currently limited to only videos where the desired aspect ratio is equal to the true aspect ratio.
		// TODO: Enable scaling capabilities
		const auto& Camera = Parser->GetCamera();
		const size_t Width = Camera.GetFrameWidth();
		const size_t Height = Camera.GetFrameHeight();
		const float AspectRatio = Height != 0 ? float(Width) / float(Height) : 0;
		Scenario.Camera.MinimalViewInfo.Location = Camera.GetPosition();
		Scenario.Camera.MinimalViewInfo.Rotation = Camera.GetRotation();
		Scenario.Camera.MinimalViewInfo.FOV = Camera.GetFOV();
		Scenario.Camera.MinimalViewInfo.DesiredFOV = Camera.GetFOV();
		Scenario.Camera.MinimalViewInfo.AspectRatio = AspectRatio;
		Scenario.Camera.NIR = Camera.GetNIR();
		Scenario.Camera.FrameSize.X = Width;
		Scenario.Camera.FrameSize.Y = Height;
		Scenario.Camera.FrameRate = Camera.GetFrameRate();
		Scenario.Camera.Mirrored = Camera.GetMirrored();
		Scenario.Camera.GrainIntensity = Camera.GetGrainIntensity();
		Scenario.Camera.GrainJitter = Camera.GetGrainJitter();
		Scenario.Camera.Saturation = Camera.GetSaturation();
		Scenario.Camera.Gamma = Camera.GetGamma();
		Scenario.Camera.Contrast = Camera.GetContrast();
		Scenario.Camera.BloomIntensity = Camera.GetBloomIntensity();
		Scenario.Camera.FocusOffset = Camera.GetFocusOffset();
		CarSpeed = Parser->GetCarSpeed();
	}
	catch (const std::exception& e) {
		ErrorMessage = e.what();
		DMSSimLog::Fatal() << "Scenario Blueprint: " << e.what() << FL;
		return false;
	}
	return true;
}

bool UDMSSimScenarioBlueprint::GetDmsEnvironment(const FString& Path, const int32& ScenarioIndex, FString& Environment, FString& ErrorMessage) {
	DMSSimLog::Info() << "Scenario Blueprint: " << __func__ << FL;
	try {
		DMSSimScenarioParserWrapper Parser(Path, ErrorMessage, ScenarioIndex);
		Environment = Parser->GetEnvironment();
	}
	catch (const std::exception& e) {
		ErrorMessage = e.what();
		DMSSimLog::Fatal() << "Scenario Blueprint: " << e.what() << FL;
		return false;
	}
	return true;
}

bool UDMSSimScenarioBlueprint::GetDmsAnimationsMulti(FDMSSimOccupantType OccupantType, bool ResetAnimations, USkeleton* const FaceSkeleton, USkeleton* const BodySkeleton, FDMSSimAnimationContainer& Animations, float& TotalTime, FString& ErrorMessage) {
	try {
		auto* Parser = DMSSimConfig::GetCurrentScenarioParser().Get();
		if (!Parser) { return false; }
		TSharedPtr<DMSSimAssetRegistry> AssetRegistry(DMSSimAssetRegistry::Create());
		if (!AssetRegistry) {
			DMSSimLog::Info() << "Failed to load asset registry: " << __func__ << " - " << __FILE__ << ": " << __LINE__ << FL;
			return false;
		}

		TotalTime = 0.0f;
		if (ResetAnimations) { Animations.Reset(); }

		BuildChannelMontageList(Parser, AssetRegistry.Get(), OccupantType, DMSSimAnimationChannelCommon, FaceSkeleton, Animations.CommonChannel, TotalTime);
		BuildChannelMontageList(Parser, AssetRegistry.Get(), OccupantType, DMSSimAnimationChannelEyeGaze, FaceSkeleton, Animations.EyeGazeChannel, TotalTime);
		BuildChannelMontageList(Parser, AssetRegistry.Get(), OccupantType, DMSSimAnimationChannelEyelids, FaceSkeleton, Animations.EyelidsChannel, TotalTime);
		BuildChannelMontageList(Parser, AssetRegistry.Get(), OccupantType, DMSSimAnimationChannelFace, FaceSkeleton, Animations.FaceChannel, TotalTime);
		BuildChannelMontageList(Parser, AssetRegistry.Get(), OccupantType, DMSSimAnimationChannelHead, BodySkeleton, Animations.HeadChannel, TotalTime);
		BuildChannelMontageList(Parser, AssetRegistry.Get(), OccupantType, DMSSimAnimationChannelUpperBody, BodySkeleton, Animations.UpperBodyChannel, TotalTime);
		BuildChannelMontageList(Parser, AssetRegistry.Get(), OccupantType, DMSSimAnimationChannelLeftHand, BodySkeleton, Animations.LeftHandChannel, TotalTime);
		BuildChannelMontageList(Parser, AssetRegistry.Get(), OccupantType, DMSSimAnimationChannelRightHand, BodySkeleton, Animations.RightHandChannel, TotalTime);
		BuildChannelMontageList(Parser, AssetRegistry.Get(), OccupantType, DMSSimAnimationChannelSteeringWheel, BodySkeleton, Animations.SteeringWheelChannel, TotalTime);
	}
	catch (const std::exception& e) {
		ErrorMessage = e.what();
		DMSSimLog::Fatal() << "Scenario Blueprint: " << e.what() << FL;
		return false;
	}
	return false;
}

FDMSRandomMovements UDMSSimScenarioBlueprint::GetRandomMovementsStatus() { return RandomMovementsStatus; }

float UDMSSimScenarioBlueprint::GetRandomFloat(const float& Min, const float& Max) {
	if (!RandomGenerator) {
		const auto seed = std::chrono::system_clock::now().time_since_epoch().count();
		RandomGenerator = std::make_unique<std::mt19937>(seed);
	}
	
	// Random numbers generating only work for integers
	constexpr int STEPS_PER_UNIT = 10000;
	std::uniform_int_distribution<int> Distribution(static_cast<int>(Min) * STEPS_PER_UNIT, static_cast<int>(Max) * STEPS_PER_UNIT);
	const auto result = static_cast<float>(Distribution(*RandomGenerator)) / STEPS_PER_UNIT;

	return result;
}