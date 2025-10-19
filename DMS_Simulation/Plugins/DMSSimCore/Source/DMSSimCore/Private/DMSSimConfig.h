/**
 * @brief Set of global functions and data for communication between components.
 * Basically, it's a singleton that mediates signals between the Unreal Editor, the renderer and the ground truth recorder.
 */
#pragma once

#include <functional>
#include <string>
#include <vector>
#include "DMSSimScenarioParser.h"
#include "DMSSimScenarioBlueprint.h"


constexpr size_t NUMBER_OF_FRAMES_TO_SKIP = 3; // to make sure all resources are properly loaded

struct DMSSimCustomLight
{
	float Intensity;
	float AttenuationRadius;
	float SourceRadius;
	float InnerConeAngle;
	float OuterConeAngle;
	FVector Position;
	FRotator Rotation;
};

struct DMSSimSunLight
{
	float Intensity;
	float Temperature;
	FRotator Rotation;
};

struct DMSGroundTruthSettings
{
	float BoundingBoxPaddingFactorFace;
	float EyeBoundingBoxWidthFactor;
	float EyeBoundingBoxHeightFactor;
	float EyeBoundingBoxDepth;
};

struct DMSCamera
{
	bool Mirrored;
	bool NIR;
	FVector Position_inCar;
	FRotator Rotation_inCar;
	float FOV;
	float AspectRatio;
	FIntPoint FrameSize;
	int32 FrameRate;
	float GrainIntensity;
	float GrainJitter;
	float Saturation;
	float Gamma;
	float Contrast;
	float BloomIntensity;
	float FocusOffset;
};

struct DMSBlendoutDefaults
{
	float Common;
	float EyeGaze;
	float Eyelids;
	float Face;
	float Head;
	float UpperBody;
	float LeftHand;
	float RightHand;
	float SteeringWheel;
};

struct DMSSimGroundTruthCommon
{
	int32 ScenarioIdx;
	int32 ScenariosCount;
	int32 VersionMajor;
	int32 VersionMinor;
	std::string Description;
	std::string Environment;
	bool RandomMovements;
	std::string CarModel;
	float CarSpeed;
	DMSSimSunLight SunLight;
	DMSCamera Camera;
	DMSSimCustomLight CameraLight;
	DMSBlendoutDefaults BlendoutDefaults;
	FVector  CarPosition_inWorld;
	FRotator CarRotation_inWorld;
	int	OccupantCount;
	TArray<FDMSSimOccupant> Occupants;
	DMSGroundTruthSettings GroundTruthSettings;
};

struct DMSSimGroundTruthOccupant
{
	bool     Initialized;
	FVector  NosePoint;
	FVector  LEarPoint;
	FVector  REarPoint;
	FVector  LeftEyePoint;
	FVector  RightEyePoint;
	FVector  LeftGazeOrigin_inCam;
	FVector  RightGazeOrigin_inCam;
	FVector  GazeOrigin_inCam;
	FVector  LeftGazeDirection_inCam;
	FVector  RightGazeDirection_inCam;
	FVector  GazeDirection_inCam;
	FVector  LeftGazeOrigin_inCar;
	FVector  RightGazeOrigin_inCar;
	FVector  GazeOrigin_inCar;
	FVector  LeftGazeDirection_inCar;
	FVector  RightGazeDirection_inCar;
	FVector  GazeDirection_inCar;
	FVector  HeadOriginEyesCenter_inCam;
	FVector  HeadOriginEarsCenter_inCam;
	FVector  HeadDirection_inCam;
	FRotator HeadRotation_inCam;
	FVector  HeadOriginEyesCenter_inCar;
	FVector  HeadOriginEarsCenter_inCar;
	FVector  HeadDirection_inCar;
	FRotator HeadRotation_inCar;
	FVector  LeftShoulderPoint;
	FVector  RightShoulderPoint;
	FVector  LeftElbowPoint;
	FVector  RightElbowPoint;
	FVector  LeftWristPoint;
	FVector  RightWristPoint;
	FVector  LeftPinkyKnucklePoint;
	FVector  RightPinkyKnucklePoint;
	FVector  LeftIndexKnucklePoint;
	FVector  RightIndexKnucklePoint;
	FVector  LeftThumbKnucklePoint;
	FVector  RightThumbKnucklePoint;
	FVector  LeftHipPoint;
	FVector  RightHipPoint;
	FVector  LeftKneePoint;
	FVector  RightKneePoint;
	FVector  LeftAnklePoint;
	FVector  RightAnklePoint;
	FVector  LeftHeelPoint;
	FVector  RightHeelPoint;
	FVector  LeftFootIndexPoint;
	FVector  RightFootIndexPoint;

	float    HorizontalMouthOpening;
	float    VerticalMouthOpening;

	TArray<bool> FacialLandmarksVisible;
	TArray<FVector> FacialLandmarks3D_inCam;
	TArray<FVector2D> FacialLandmarks2D;
	bool FaceBoundingBox3DVisible;
	TArray<FVector> FaceBoundingBox3D_inCam;
	bool FaceBoundingBox2DVisible;
	FDMSBoundingBox2D FaceBoundingBox2D;
	bool RightEyeBoundingBox2DVisible;
	FDMSBoundingBox2D RightEyeBoundingBox2D;
	bool LeftEyeBoundingBox2DVisible;
	FDMSBoundingBox2D LeftEyeBoundingBox2D;
	TArray<FVector2D> RightEyePupilLandmarks2D;
	TArray<FVector2D> RightEyeIrisLandmarks2D;
	TArray<FVector2D> LeftEyePupilLandmarks2D;
	TArray<FVector2D> LeftEyeIrisLandmarks2D;
	TArray<bool> PupilIrisLandmarksVisible;

	float LeftEyeOpening;
	float RightEyeOpening;
	float LeftEyeLidVisibilityPerc;
	float RightEyeLidVisibilityPerc;
	float LeftEyePupilVisibilityPerc;
	float RightEyePupilVisibilityPerc;
};

struct DMSSimGroundTruthFrame
{
	DMSSimGroundTruthCommon Common;
	DMSSimGroundTruthOccupant Occupants[FDMSSimOccupantType::PassengerCount];
};

class DMSSimScenarioParser;

namespace DMSSimConfig {

using CameraSetCallback = std::function<void(UCameraComponent*)>;

void Initialize();
//void Terminate();

const TSharedPtr<DMSSimScenarioParser> GetScenarioParser(int32 index);
void AddScenarioParser(const TSharedPtr<DMSSimScenarioParser> Parser);
void SetScenarioParsers(const std::vector<TSharedPtr<DMSSimScenarioParser>> Parsers);
void SetCurrentScenarioParser(const TSharedPtr<DMSSimScenarioParser> Parser);
TSharedPtr<DMSSimScenarioParser> GetCurrentScenarioParser();
void ResetScenarioParsers();
int32 GetScenarioParsersCount();

std::string GetScenarioParserErrorMessage();
bool IsOutputDirectoryPresent();

std::wstring GetFilePrefix();

const DMSSimCoordinateSpace& GetCoordinateSpace();
const DMSSimCamera& GetCamera();

bool StartRecording();
bool StopRecording();
bool IsRecording();

void SetNumRemainingFrames(int NumFrames);
int GetNumRemainingFrames();

void SetCameraCallback(const CameraSetCallback& Callback);
void SetCameraComponent(UCameraComponent* Camera);

void UpdateDisplayedFrameTime(const float Time);
int GetCurrentFrame();
float GetCurrentTime();

void ResetGroundTruthData();
const DMSSimGroundTruthFrame& GetGroundTruthFrame();
void SetGroundTruthCommonData(const DMSSimGroundTruthCommon& CommonData);
void SetGroundTruthOccupantData(FDMSSimOccupantType OccupantType, const DMSSimGroundTruthOccupant& OccupantData);

} // namespace DMSSimConfig
