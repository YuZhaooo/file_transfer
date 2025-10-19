#include "DMSSimGroundTruthBlueprint.h"
#include "DMSSimConfig.h"
#include "DMSSimLog.h"
#include "DMSSimConstants.h"

#define DMS_GT_COPY_VALUE(NAME) Frame.##NAME = NAME;

static constexpr size_t R_EYELID_UPPER_MID = 68;
static constexpr size_t R_EYELID_LOWER_MID = 69;
static constexpr size_t L_EYELID_UPPER_MID = 70;
static constexpr size_t L_EYELID_LOWER_MID = 71;

static float ComputeEyeOpening(const FVector& MidUpper, const FVector& MidLower) {
	if (MidUpper.Z < MidLower.Z) { return 0; }// Edge case when upper eyelid goes over lower eyelid.
	return FVector::Dist(MidUpper, MidLower);
}

static FVector ToOutputFormat(const FVector& In) {
	FVector Out = In;
	Out.X *= DMSSIM_CM_TO_M;
	Out.Y *= DMSSIM_CM_TO_M;
	Out.Y *= -1;
	Out.Z *= DMSSIM_CM_TO_M;
	return Out;
}

static FRotator ToOutputFormat(const FRotator& In) {
	FRotator Out = In;
	Out.Pitch *= -1;
	return Out;
}

void UDMSSimGroundTruthBlueprint::StoreDmsGroundTruthCommonData(
	const FDMSScenario& Scenario,
	const FVector& CameraLocation_inCar,
	const FRotator& CameraRotation_inCar,
	const FVector& IlluminationLocation_inCar,
	const FRotator IlluminationRotation_inCar,
	const FVector& CarPosition_inWorld,
	const FRotator& CarRotation_inWorld,
	const int& OccupantCount,
	const TArray<FDMSSimOccupant>& Occupants)
{
	DMSSimGroundTruthCommon GroundTruth;

	// Directly map the properties from FDMSScenario to DMSSimGroundTruthCommon
	GroundTruth.ScenarioIdx = Scenario.ScenarioIdx;
	GroundTruth.ScenariosCount = Scenario.ScenariosCount;
	GroundTruth.VersionMajor = Scenario.VersionMajor;
	GroundTruth.VersionMinor = Scenario.VersionMinor;
	GroundTruth.Description = std::string(TCHAR_TO_UTF8(*Scenario.Description));
	GroundTruth.Environment = std::string(TCHAR_TO_UTF8(*Scenario.Environment));
	// TODO better output for random movements?
	const auto& RM = Scenario.RandomMovements;
	GroundTruth.RandomMovements = RM.Blinking || RM.Smiling || RM.Head || RM.Body || RM.Gaze;
	GroundTruth.CarModel = std::string(TCHAR_TO_UTF8(*Scenario.CarModel));
	GroundTruth.CarSpeed = Scenario.CarSpeed;
	GroundTruth.CarPosition_inWorld = ToOutputFormat(CarPosition_inWorld);
	GroundTruth.CarRotation_inWorld = ToOutputFormat(CarRotation_inWorld);

	// Convert FDMSSimSunLight to DMSSimSunLight
	GroundTruth.SunLight.Intensity = Scenario.SunLight.Intensity;
	GroundTruth.SunLight.Temperature = Scenario.SunLight.Temperature;
	GroundTruth.SunLight.Rotation = ToOutputFormat(Scenario.SunLight.Rotation);

	// Convert FDMSCamera to DMSCamera
	GroundTruth.Camera.Position_inCar = ToOutputFormat(CameraLocation_inCar);
	GroundTruth.Camera.Rotation_inCar = CameraRotation_inCar;
	GroundTruth.Camera.FOV = Scenario.Camera.MinimalViewInfo.FOV;
	GroundTruth.Camera.AspectRatio = Scenario.Camera.MinimalViewInfo.AspectRatio;
	GroundTruth.Camera.NIR = Scenario.Camera.NIR;
	GroundTruth.Camera.FrameSize = FIntPoint(static_cast<int32>(Scenario.Camera.FrameSize.X), static_cast<int32>(Scenario.Camera.FrameSize.Y));
	GroundTruth.Camera.FrameRate = Scenario.Camera.FrameRate;
	GroundTruth.Camera.Mirrored = Scenario.Camera.Mirrored;
	GroundTruth.Camera.GrainIntensity = Scenario.Camera.GrainIntensity;
	GroundTruth.Camera.GrainJitter = Scenario.Camera.GrainJitter;
	GroundTruth.Camera.Saturation = Scenario.Camera.Saturation;
	GroundTruth.Camera.Gamma = Scenario.Camera.Gamma;
	GroundTruth.Camera.Contrast = Scenario.Camera.Contrast;
	GroundTruth.Camera.BloomIntensity = Scenario.Camera.BloomIntensity;
	GroundTruth.Camera.FocusOffset = Scenario.Camera.FocusOffset;

	// Convert FDMSSimCustomLight to DMSSimCustomLight
	GroundTruth.CameraLight.Intensity = Scenario.CameraLight.Intensity;
	GroundTruth.CameraLight.AttenuationRadius = Scenario.CameraLight.AttenuationRadius;
	GroundTruth.CameraLight.SourceRadius = Scenario.CameraLight.SourceRadius;
	GroundTruth.CameraLight.InnerConeAngle = Scenario.CameraLight.InnerConeAngle;
	GroundTruth.CameraLight.OuterConeAngle = Scenario.CameraLight.OuterConeAngle;
	GroundTruth.CameraLight.Position = ToOutputFormat(IlluminationLocation_inCar);
	GroundTruth.CameraLight.Rotation = IlluminationRotation_inCar;

	// Convert FDMSBlendoutDefaults to DMSBlendoutDefaults
	GroundTruth.BlendoutDefaults.Common = Scenario.BlendoutDefaults.Common;
	GroundTruth.BlendoutDefaults.EyeGaze = Scenario.BlendoutDefaults.EyeGaze;
	GroundTruth.BlendoutDefaults.Eyelids = Scenario.BlendoutDefaults.Eyelids;
	GroundTruth.BlendoutDefaults.Face = Scenario.BlendoutDefaults.Face;
	GroundTruth.BlendoutDefaults.Head = Scenario.BlendoutDefaults.Head;
	GroundTruth.BlendoutDefaults.UpperBody = Scenario.BlendoutDefaults.UpperBody;
	GroundTruth.BlendoutDefaults.LeftHand = Scenario.BlendoutDefaults.LeftHand;
	GroundTruth.BlendoutDefaults.RightHand = Scenario.BlendoutDefaults.RightHand;
	GroundTruth.BlendoutDefaults.SteeringWheel = Scenario.BlendoutDefaults.SteeringWheel;

	GroundTruth.GroundTruthSettings.BoundingBoxPaddingFactorFace = Scenario.GroundTruthSettings.BoundingBoxPaddingFactorFace;
	GroundTruth.GroundTruthSettings.EyeBoundingBoxWidthFactor = Scenario.GroundTruthSettings.EyeBoundingBoxWidthFactor;
	GroundTruth.GroundTruthSettings.EyeBoundingBoxHeightFactor = Scenario.GroundTruthSettings.EyeBoundingBoxHeightFactor;
	GroundTruth.GroundTruthSettings.EyeBoundingBoxDepth = Scenario.GroundTruthSettings.EyeBoundingBoxDepth;

	// Directly map remaining properties to DMSSimGroundTruthCommon
	GroundTruth.Occupants = Occupants;
	GroundTruth.OccupantCount = OccupantCount;

	DMSSimConfig::SetGroundTruthCommonData(GroundTruth);
}

void UDMSSimGroundTruthBlueprint::StoreDmsGroundTruthFrame(
	const FDMSSimOccupantType& OccupantType,
	const FVector& NosePoint,
	const FVector& LEarPoint,
	const FVector& REarPoint,
	const FVector& LeftEyePoint,
	const FVector& RightEyePoint,
	const FVector& LeftGazeOrigin_inCam,
	const FVector& RightGazeOrigin_inCam,
	const FVector& GazeOrigin_inCam,
	const FVector& LeftGazeDirection_inCam,
	const FVector& RightGazeDirection_inCam,
	const FVector& GazeDirection_inCam,
	const FVector& LeftGazeOrigin_inCar,
	const FVector& RightGazeOrigin_inCar,
	const FVector& GazeOrigin_inCar,
	const FVector& LeftGazeDirection_inCar,
	const FVector& RightGazeDirection_inCar,
	const FVector& GazeDirection_inCar,
	const FVector& HeadOriginEyesCenter_inCam,
	const FVector& HeadOriginEarsCenter_inCam,
	const FVector& HeadDirection_inCam,
	const FRotator& HeadRotation_inCam,
	const FVector& HeadOriginEyesCenter_inCar,
	const FVector& HeadOriginEarsCenter_inCar,
	const FVector& HeadDirection_inCar,
	const FRotator& HeadRotation_inCar,
	const FVector& LeftShoulderPoint,
	const FVector& RightShoulderPoint,
	const FVector& LeftElbowPoint,
	const FVector& RightElbowPoint,
	const FVector& LeftWristPoint,
	const FVector& RightWristPoint,
	const FVector& LeftPinkyKnucklePoint,
	const FVector& RightPinkyKnucklePoint,
	const FVector& LeftIndexKnucklePoint,
	const FVector& RightIndexKnucklePoint,
	const FVector& LeftThumbKnucklePoint,
	const FVector& RightThumbKnucklePoint,
	const FVector& LeftHipPoint,
	const FVector& RightHipPoint,
	const FVector& LeftKneePoint,
	const FVector& RightKneePoint,
	const FVector& LeftAnklePoint,
	const FVector& RightAnklePoint,
	const FVector& LeftHeelPoint,
	const FVector& RightHeelPoint,
	const FVector& LeftFootIndexPoint,
	const FVector& RightFootIndexPoint,
	const float    HorizontalMouthOpening,
	const float    VerticalMouthOpening,
	const TArray<bool>& FacialLandmarksVisible,
	const TArray<FVector>& FacialLandmarks3D_inCam,
	const TArray<FVector2D>& FacialLandmarks2D,
	const TArray<FVector>& FaceBoundingBox3D_inCam,
	const bool FaceBoundingBox3DVisible,
	const FDMSBoundingBox2D& FaceBoundingBox2D,
	const bool FaceBoundingBox2DVisible,
	const FDMSBoundingBox2D& RightEyeBoundingBox2D,
	const bool RightEyeBoundingBox2DVisible,
	const FDMSBoundingBox2D& LeftEyeBoundingBox2D,
	const bool LeftEyeBoundingBox2DVisible,
	const TArray<FVector2D>& RightEyePupilLandmarks2D,
	const TArray<FVector2D>& RightEyeIrisLandmarks2D,
	const TArray<FVector2D>& LeftEyePupilLandmarks2D,
	const TArray<FVector2D>& LeftEyeIrisLandmarks2D,
	const TArray<bool>& PupilIrisLandmarksVisible,
	const float LeftEyeLidVisibilityPerc,
	const float RightEyeLidVisibilityPerc,
	const float LeftEyePupilVisibilityPerc,
	const float RightEyePupilVisibilityPerc
	)
{
	DMSSimGroundTruthOccupant Frame;
	memset(&Frame, 0, sizeof(Frame));
	Frame.Initialized = true;

	DMS_GT_COPY_VALUE(NosePoint);
	DMS_GT_COPY_VALUE(LEarPoint);
	DMS_GT_COPY_VALUE(REarPoint)
	DMS_GT_COPY_VALUE(LeftEyePoint)
	DMS_GT_COPY_VALUE(RightEyePoint)
	Frame.LeftGazeOrigin_inCam = ToOutputFormat(LeftGazeOrigin_inCam);
	Frame.RightGazeOrigin_inCam = ToOutputFormat(RightGazeOrigin_inCam);
	Frame.GazeOrigin_inCam = ToOutputFormat(GazeOrigin_inCam);
	Frame.LeftGazeDirection_inCam = ToOutputFormat(LeftGazeDirection_inCam).GetSafeNormal(0.0);
	Frame.RightGazeDirection_inCam = ToOutputFormat(RightGazeDirection_inCam).GetSafeNormal(0.0);
	Frame.GazeDirection_inCam = ToOutputFormat(GazeDirection_inCam).GetSafeNormal(0.0);
	Frame.LeftGazeOrigin_inCar = ToOutputFormat(LeftGazeOrigin_inCar);
	Frame.RightGazeOrigin_inCar = ToOutputFormat(RightGazeOrigin_inCar);
	Frame.GazeOrigin_inCar = ToOutputFormat(GazeOrigin_inCar);
	Frame.LeftGazeDirection_inCar = ToOutputFormat(LeftGazeDirection_inCar).GetSafeNormal(0.0);
	Frame.RightGazeDirection_inCar = ToOutputFormat(RightGazeDirection_inCar).GetSafeNormal(0.0);
	Frame.GazeDirection_inCar = ToOutputFormat(GazeDirection_inCar).GetSafeNormal(0.0);
	Frame.HeadOriginEyesCenter_inCam = ToOutputFormat(HeadOriginEyesCenter_inCam);
	Frame.HeadOriginEarsCenter_inCam = ToOutputFormat(HeadOriginEarsCenter_inCam);
	Frame.HeadDirection_inCam = ToOutputFormat(HeadDirection_inCam).GetSafeNormal(0.0);
	Frame.HeadOriginEyesCenter_inCar = ToOutputFormat(HeadOriginEyesCenter_inCar);
	Frame.HeadOriginEarsCenter_inCar = ToOutputFormat(HeadOriginEarsCenter_inCar);
	Frame.HeadDirection_inCar = ToOutputFormat(HeadDirection_inCar).GetSafeNormal(0.0);
	DMS_GT_COPY_VALUE(HeadRotation_inCam)
	DMS_GT_COPY_VALUE(HeadRotation_inCar)
	DMS_GT_COPY_VALUE(HeadRotation_inCam);
	DMS_GT_COPY_VALUE(LeftShoulderPoint)
	DMS_GT_COPY_VALUE(RightShoulderPoint)
	DMS_GT_COPY_VALUE(LeftElbowPoint)
	DMS_GT_COPY_VALUE(RightElbowPoint)
	DMS_GT_COPY_VALUE(LeftWristPoint)
	DMS_GT_COPY_VALUE(RightWristPoint)
	DMS_GT_COPY_VALUE(LeftPinkyKnucklePoint)
	DMS_GT_COPY_VALUE(RightPinkyKnucklePoint)
	DMS_GT_COPY_VALUE(LeftIndexKnucklePoint)
	DMS_GT_COPY_VALUE(RightIndexKnucklePoint)
	DMS_GT_COPY_VALUE(LeftThumbKnucklePoint)
	DMS_GT_COPY_VALUE(RightThumbKnucklePoint)
	DMS_GT_COPY_VALUE(LeftHipPoint)
	DMS_GT_COPY_VALUE(RightHipPoint)
	DMS_GT_COPY_VALUE(LeftKneePoint)
	DMS_GT_COPY_VALUE(RightKneePoint)
	DMS_GT_COPY_VALUE(LeftAnklePoint)
	DMS_GT_COPY_VALUE(RightAnklePoint)
	DMS_GT_COPY_VALUE(LeftHeelPoint)
	DMS_GT_COPY_VALUE(RightHeelPoint)
	DMS_GT_COPY_VALUE(LeftFootIndexPoint)
	DMS_GT_COPY_VALUE(RightFootIndexPoint)
	DMS_GT_COPY_VALUE(HorizontalMouthOpening);
	DMS_GT_COPY_VALUE(VerticalMouthOpening);
	DMS_GT_COPY_VALUE(FacialLandmarksVisible);
	DMS_GT_COPY_VALUE(FacialLandmarks3D_inCam);
	DMS_GT_COPY_VALUE(FacialLandmarks2D);
	DMS_GT_COPY_VALUE(FaceBoundingBox3D_inCam);
	DMS_GT_COPY_VALUE(FaceBoundingBox3DVisible);
	DMS_GT_COPY_VALUE(FaceBoundingBox2D);
	DMS_GT_COPY_VALUE(FaceBoundingBox2DVisible);
	DMS_GT_COPY_VALUE(RightEyeBoundingBox2D);
	DMS_GT_COPY_VALUE(RightEyeBoundingBox2DVisible);
	DMS_GT_COPY_VALUE(LeftEyeBoundingBox2D);
	DMS_GT_COPY_VALUE(LeftEyeBoundingBox2DVisible);
	DMS_GT_COPY_VALUE(RightEyePupilLandmarks2D);
	DMS_GT_COPY_VALUE(RightEyeIrisLandmarks2D);
	DMS_GT_COPY_VALUE(LeftEyePupilLandmarks2D);
	DMS_GT_COPY_VALUE(LeftEyeIrisLandmarks2D);
	DMS_GT_COPY_VALUE(PupilIrisLandmarksVisible);
	DMS_GT_COPY_VALUE(LeftEyeLidVisibilityPerc);
	DMS_GT_COPY_VALUE(RightEyeLidVisibilityPerc);
	DMS_GT_COPY_VALUE(LeftEyePupilVisibilityPerc);
	DMS_GT_COPY_VALUE(RightEyePupilVisibilityPerc);

	Frame.LeftEyeOpening = ComputeEyeOpening(FacialLandmarks3D_inCam[L_EYELID_UPPER_MID], FacialLandmarks3D_inCam[L_EYELID_LOWER_MID]);
	Frame.RightEyeOpening = ComputeEyeOpening(FacialLandmarks3D_inCam[R_EYELID_UPPER_MID], FacialLandmarks3D_inCam[R_EYELID_LOWER_MID]);
	DMSSimConfig::SetGroundTruthOccupantData(OccupantType, Frame);
}
#undef DMS_GT_COPY_VALUE

void UDMSSimGroundTruthBlueprint::ResetGroundTruthData() { DMSSimConfig::ResetGroundTruthData(); }

bool UDMSSimGroundTruthBlueprint::StartDmsRecording() {
	DMSSimLog::Info() << __func__ << FL;
	return DMSSimConfig::StartRecording();
}

bool UDMSSimGroundTruthBlueprint::StopDmsRecording() {
	DMSSimLog::Info() << __func__ << FL;
	return DMSSimConfig::StopRecording();
}

void UDMSSimGroundTruthBlueprint::GetDmsFrameTime(int32& FrameIndex, float& FrameTime) {
	FrameIndex = DMSSimConfig::GetCurrentFrame();
	FrameTime = DMSSimConfig::GetCurrentTime();
}