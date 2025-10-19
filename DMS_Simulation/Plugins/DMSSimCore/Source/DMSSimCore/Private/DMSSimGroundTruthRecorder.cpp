#include "DMSSimGroundTruthRecorder.h"
#include "DMSSimGroundTruthBlueprint.h"
#include "DMSSimConfig.h"
#include "DMSSimConstants.h"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <string>
#include <regex>

namespace {

constexpr bool PRINT_ALL_COLUMNS = false;

thread_local size_t FrameNumber_ = 0;

struct DMSSimFrameComputed {
	double   Time;
	FVector  HeadPosition;
	FRotator HeadRotation;
	FVector  CameraPosition;
	FRotator CameraRotation;
	FVector  LeftEyePoint;
	FVector  RightEyePoint;
	FVector  GazeOrigin_inCam;
	FVector  LeftGazeOrigin_inCam;
	FVector  RightGazeOrigin_inCam;
	FVector  GazeDirection_inCam;
	FVector  LeftGazeDirection_inCam;
	FVector  RightGazeDirection_inCam;

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
	TArray <bool> FacialLandmarksVisible;
	TArray <FVector> FacialLandmarks3D;
	TArray <FVector2D> FacialLandmarks2D;
	TArray<FVector> FaceBoundingBox3D;
	bool FaceBoundingBox3DVisible;
	FDMSBoundingBox2D FaceBoundingBox2D;
	bool FaceBoundingBox2DVisible;
	FDMSBoundingBox2D RightEyeBoundingBox2D;
	bool RightEyeBoundingBox2DVisible;
	FDMSBoundingBox2D LeftEyeBoundingBox2D;
	bool LeftEyeBoundingBox2DVisible;

};

typedef void (*ColumnHandlerFunc)(std::ostream& Stream, const DMSSimFrameComputed& Frame);

struct OccupantInfo {
	FDMSSimOccupantType Occupant;
	const char* Prefix;
};

const OccupantInfo GTOccupantInfos[] =
{
	{ FDMSSimOccupantType::Driver, "" },
	{ FDMSSimOccupantType::PassengerFront, "PF_" },
	{ FDMSSimOccupantType::PassengerRearLeft, "PRL_" },
	{ FDMSSimOccupantType::PassengerRearMiddle, "PRM_" },
	{ FDMSSimOccupantType::PassengerRearRight, "PRR_" },
};

struct ColumnHandler {
	const char*       ColumnName;
	ColumnHandlerFunc Func;
	bool              DriverOnly;
	bool              WithoutPrefix;
};
std::ostream& PrintBool(std::ostream& Stream, const bool Value) {
	Stream << int(Value);
	return Stream;
}
std::ostream& PrintFloat(std::ostream& Stream, const float Value) {
	Stream << std::setprecision(8) << Value;
	return Stream;
}

std::ostream& PrintInt(std::ostream& Stream, const int Value) {
	Stream << Value;
	return Stream;
}

void NormalizeVectorInCM(FVector& V) {
	V.Normalize();
	V *= 100.0f;
}

std::ostream& PrintFloatDiv100(std::ostream& Stream, const float Value) {
	Stream << std::setprecision(8) << (Value / 100.0f);
	return Stream;
}

std::ostream& PrintAngleFloat(std::ostream& Stream, const float Value) {
	const float ValueRad = static_cast<float>((PI * Value) / 180.0);
	return PrintFloat(Stream, ValueRad);
}

void ColumnHandler_Time(std::ostream& Stream, const DMSSimFrameComputed& Frame) {
	double FullSeconds = 0.0f;
	const double FracSeconds = std::modf(Frame.Time, &FullSeconds);
	const int SecondsTotal = static_cast<int>(FullSeconds);
	const int Seconds = SecondsTotal % 60;
	const int MinutesTotal = SecondsTotal / 60;
	const int Minutes = MinutesTotal % 60;
	const int Hours = MinutesTotal / 60;

	const int Fraction = static_cast<int>(FracSeconds * 1000);

	Stream << std::setfill('0') << std::setw(2) << Hours;
	Stream << ":";
	Stream << std::setfill('0') << std::setw(2) << Minutes;
	Stream << ":";
	Stream << std::setfill('0') << std::setw(2) << Seconds;
	Stream << ".";
	Stream << std::setfill('0') << std::setw(3) << Fraction;
}

void ColumnHandler_Availability(std::ostream& Stream, const DMSSimFrameComputed& Frame) { Stream << 1; }

void ColumnHandler_FrameNumber(std::ostream& Stream, const DMSSimFrameComputed& Frame) { Stream << FrameNumber_; }

#define DMS_POSITION_EXTRACTOR(NAME, COMPONENT) void ColumnHandler_DM_##NAME##COMPONENT(std::ostream& Stream, const DMSSimFrameComputed& Frame)\
{\
	PrintFloat(Stream, Frame.##NAME##.##COMPONENT);\
}

#define DMS_POSITION_EXTRACTOR_SET(NAME) DMS_POSITION_EXTRACTOR(NAME, X)\
	DMS_POSITION_EXTRACTOR(NAME, Y)\
	DMS_POSITION_EXTRACTOR(NAME, Z)

DMS_POSITION_EXTRACTOR_SET(HeadPosition)
DMS_POSITION_EXTRACTOR_SET(CameraPosition)
DMS_POSITION_EXTRACTOR_SET(LeftEyePoint)
DMS_POSITION_EXTRACTOR_SET(RightEyePoint)
DMS_POSITION_EXTRACTOR_SET(GazeOrigin_inCam)
DMS_POSITION_EXTRACTOR_SET(LeftGazeOrigin_inCam)
DMS_POSITION_EXTRACTOR_SET(RightGazeOrigin_inCam)
DMS_POSITION_EXTRACTOR_SET(LeftShoulderPoint)
DMS_POSITION_EXTRACTOR_SET(RightShoulderPoint)
DMS_POSITION_EXTRACTOR_SET(LeftElbowPoint)
DMS_POSITION_EXTRACTOR_SET(RightElbowPoint)
DMS_POSITION_EXTRACTOR_SET(LeftWristPoint)
DMS_POSITION_EXTRACTOR_SET(RightWristPoint)
DMS_POSITION_EXTRACTOR_SET(LeftPinkyKnucklePoint)
DMS_POSITION_EXTRACTOR_SET(RightPinkyKnucklePoint)
DMS_POSITION_EXTRACTOR_SET(LeftIndexKnucklePoint)
DMS_POSITION_EXTRACTOR_SET(RightIndexKnucklePoint)
DMS_POSITION_EXTRACTOR_SET(LeftThumbKnucklePoint)
DMS_POSITION_EXTRACTOR_SET(RightThumbKnucklePoint)
DMS_POSITION_EXTRACTOR_SET(LeftHipPoint)
DMS_POSITION_EXTRACTOR_SET(RightHipPoint)
DMS_POSITION_EXTRACTOR_SET(LeftKneePoint)
DMS_POSITION_EXTRACTOR_SET(RightKneePoint)
DMS_POSITION_EXTRACTOR_SET(LeftAnklePoint)
DMS_POSITION_EXTRACTOR_SET(RightAnklePoint)
DMS_POSITION_EXTRACTOR_SET(LeftHeelPoint)
DMS_POSITION_EXTRACTOR_SET(RightHeelPoint)
DMS_POSITION_EXTRACTOR_SET(LeftFootIndexPoint)
DMS_POSITION_EXTRACTOR_SET(RightFootIndexPoint)
DMS_POSITION_EXTRACTOR_SET(GazeDirection_inCam)
DMS_POSITION_EXTRACTOR_SET(LeftGazeDirection_inCam)
DMS_POSITION_EXTRACTOR_SET(RightGazeDirection_inCam)

void ColumnHandler_DM_HeadRotationYaw(std::ostream& Stream, const DMSSimFrameComputed& Frame) { PrintAngleFloat(Stream, Frame.HeadRotation.Yaw); }

void ColumnHandler_DM_HeadRotationPitch(std::ostream& Stream, const DMSSimFrameComputed& Frame) { PrintAngleFloat(Stream, Frame.HeadRotation.Pitch); }

void ColumnHandler_DM_HeadRotationRoll(std::ostream& Stream, const DMSSimFrameComputed& Frame) { PrintAngleFloat(Stream, Frame.HeadRotation.Roll); }

#define DMS_POSITION_IN_SM_EXTRACTOR_EYE_OPEN(NAME) void ColumnHandler_DM_##NAME##EyeOpen(std::ostream& Stream, const DMSSimFrameComputed& Frame)\
{\
	const int Value = (Frame.NAME##EyeOpen > 0)? 1 : 0;\
	PrintInt(Stream, Value);\
}

#define DMS_POSITION_IN_SM_EXTRACTOR_EYE_CLOSED(NAME) void ColumnHandler_DM_##NAME##EyeClosed(std::ostream& Stream, const DMSSimFrameComputed& Frame)\
{\
	const int Value = (Frame.NAME##EyeOpen > 0)? 0 : 1;\
	PrintInt(Stream, Value);\
}

void ColumnHandler_DM_HorizontalMouthOpening(std::ostream& Stream, const DMSSimFrameComputed& Frame) { PrintFloatDiv100(Stream, Frame.HorizontalMouthOpening); }

void ColumnHandler_DM_VerticalMouthOpening(std::ostream& Stream, const DMSSimFrameComputed& Frame) { PrintFloatDiv100(Stream, Frame.VerticalMouthOpening); }

void ColumnHandler_FaceBB3DVisible(std::ostream& Stream, const DMSSimFrameComputed& Frame) { PrintBool(Stream, Frame.FaceBoundingBox3DVisible); }

void ColumnHandler_FaceBB2DVisible(std::ostream& Stream, const DMSSimFrameComputed& Frame) { PrintBool(Stream, Frame.FaceBoundingBox2DVisible); }

void ColumnHandler_RightEyeBB2DVisible(std::ostream& Stream, const DMSSimFrameComputed& Frame) { PrintBool(Stream, Frame.RightEyeBoundingBox2DVisible); }

void ColumnHandler_LeftEyeBB2DVisible(std::ostream& Stream, const DMSSimFrameComputed& Frame) { PrintBool(Stream, Frame.LeftEyeBoundingBox2DVisible); }

void ColumnHandler_FillFacialLandmarks(std::ostream& Stream, const DMSSimFrameComputed& Frame) {
	// Print each facial landmark in the csv file like:
	// "<FacialLandmark_0_x>"; "<FacialLandmark_0_y>"; "<FacialLandmark_0_z>";
	bool firstLandmark = true;
	for (auto i = 0; i < Frame.FacialLandmarks3D.Num(); i++) {
		if (!firstLandmark) { Stream << ";\""; }
		PrintBool(Stream, Frame.FacialLandmarksVisible[i]);
		Stream << "\";\"";
		PrintFloat(Stream, Frame.FacialLandmarks3D[i].X);
		Stream << "\";\"";
		PrintFloat(Stream, Frame.FacialLandmarks3D[i].Y);
		Stream << "\";\"";
		PrintFloat(Stream, Frame.FacialLandmarks3D[i].Z);
		Stream << "\";\"";
		PrintInt(Stream, int(Frame.FacialLandmarks2D[i].X));
		Stream << "\";\"";
		PrintInt(Stream, int(Frame.FacialLandmarks2D[i].Y));

		bool lastLandmark = i == Frame.FacialLandmarks3D.Num() - 1;
		if (!lastLandmark) { Stream << "\""; }
		firstLandmark = false;
	}
}

void PrintVectorArray(std::ostream& Stream, const TArray<FVector> VectorArray) {
	bool firstElement = true;
	for (auto i = 0; i < VectorArray.Num(); i++) {
		if (!firstElement) { Stream << ";\""; }
		PrintFloat(Stream, VectorArray[i].X);
		Stream << "\";\"";
		PrintFloat(Stream, VectorArray[i].Y);
		Stream << "\";\"";
		PrintFloat(Stream, VectorArray[i].Z);

		bool lastElement = i == VectorArray.Num() - 1;
		if (!lastElement) {	Stream << "\""; }
		firstElement = false;
	}
}

void PrintVector2DArray(std::ostream& Stream, const TArray<FVector2D> VectorArray) {
	bool firstElement = true;
	for (auto i = 0; i < VectorArray.Num(); i++) {
		if (!firstElement) { Stream << ";\""; }
		PrintFloat(Stream, VectorArray[i].X);
		Stream << "\";\"";
		PrintFloat(Stream, VectorArray[i].Y);

		bool lastElement = i == VectorArray.Num() - 1;
		if (!lastElement) { Stream << "\""; }
		firstElement = false;
	}
}

void PrintBoundingBox2D(std::ostream& Stream, const FDMSBoundingBox2D BoundingBox) {
	PrintFloat(Stream, BoundingBox.Center.X);
	Stream << "\";\"";
	PrintFloat(Stream, BoundingBox.Center.Y);
	Stream << "\";\"";
	PrintFloat(Stream, BoundingBox.Width);
	Stream << "\";\"";
	PrintFloat(Stream, BoundingBox.Height);
}

void ColumnHandler_FillFaceBoundingBox3D(std::ostream& Stream, const DMSSimFrameComputed& Frame) { PrintVectorArray(Stream, Frame.FaceBoundingBox3D); }

void ColumnHandler_FillFaceBoundingBox2D(std::ostream& Stream, const DMSSimFrameComputed& Frame) { PrintBoundingBox2D(Stream, Frame.FaceBoundingBox2D); }

void ColumnHandler_FillRightEyeBoundingBox2D(std::ostream& Stream, const DMSSimFrameComputed& Frame) { PrintBoundingBox2D(Stream, Frame.RightEyeBoundingBox2D); }

void ColumnHandler_FillLeftEyeBoundingBox2D(std::ostream& Stream, const DMSSimFrameComputed& Frame) { PrintBoundingBox2D(Stream, Frame.LeftEyeBoundingBox2D); }

#define QUOTE_STR_INRL(x) #x
#define QUOTE_STR(x) QUOTE_STR_INRL(x)
#define CONCAT_STR_INRL(a, b) a##b
#define CONCAT_STR(a, b) CONCAT_STR_INRL(a, b)

#define SET_COLUMN_PROCESSOR_POINT_COMPONENT(name, comp) { QUOTE_STR(CONCAT_STR(name, comp)), CONCAT_STR(CONCAT_STR(&ColumnHandler_DM_, name), comp) },
#define SET_COLUMN_PROCESSOR_POINT(name)  SET_COLUMN_PROCESSOR_POINT_COMPONENT(name, X) SET_COLUMN_PROCESSOR_POINT_COMPONENT(name, Y) SET_COLUMN_PROCESSOR_POINT_COMPONENT(name, Z)

ColumnHandler Columns[] = {
	{ "Time", &ColumnHandler_Time, true, true},
	{ "LeftEyePositionX", &ColumnHandler_DM_LeftEyePointX },
	{ "LeftEyePositionY", &ColumnHandler_DM_LeftEyePointY },
	{ "LeftEyePositionZ", &ColumnHandler_DM_LeftEyePointZ },
	{ "RightEyePositionX", &ColumnHandler_DM_RightEyePointX },
	{ "RightEyePositionY", &ColumnHandler_DM_RightEyePointY },
	{ "RightEyePositionZ", &ColumnHandler_DM_RightEyePointZ },
	{ "LeftGazeOriginX", &ColumnHandler_DM_LeftGazeOrigin_inCamX },
	{ "LeftGazeOriginY", &ColumnHandler_DM_LeftGazeOrigin_inCamY },
	{ "LeftGazeOriginZ", &ColumnHandler_DM_LeftGazeOrigin_inCamZ },
	{ "RightGazeOriginX", &ColumnHandler_DM_RightGazeOrigin_inCamX },
	{ "RightGazeOriginY", &ColumnHandler_DM_RightGazeOrigin_inCamY },
	{ "RightGazeOriginZ", &ColumnHandler_DM_RightGazeOrigin_inCamZ },
	{ "LeftGazeDirectionX", &ColumnHandler_DM_LeftGazeDirection_inCamX },
	{ "LeftGazeDirectionY", &ColumnHandler_DM_LeftGazeDirection_inCamY },
	{ "LeftGazeDirectionZ", &ColumnHandler_DM_LeftGazeDirection_inCamZ },
	{ "RightGazeDirectionX", &ColumnHandler_DM_RightGazeDirection_inCamX },
	{ "RightGazeDirectionY", &ColumnHandler_DM_RightGazeDirection_inCamY },
	{ "RightGazeDirectionZ", &ColumnHandler_DM_RightGazeDirection_inCamZ },
	{ "GazeOriginX", &ColumnHandler_DM_GazeOrigin_inCamX },
	{ "GazeOriginY", &ColumnHandler_DM_GazeOrigin_inCamY },
	{ "GazeOriginZ", &ColumnHandler_DM_GazeOrigin_inCamZ },
	{ "GazeVectorX", &ColumnHandler_DM_GazeDirection_inCamX },
	{ "GazeVectorY", &ColumnHandler_DM_GazeDirection_inCamY },
	{ "GazeVectorZ", &ColumnHandler_DM_GazeDirection_inCamZ },
	{ "HeadPositionX", &ColumnHandler_DM_HeadPositionX },
	{ "HeadPositionY", &ColumnHandler_DM_HeadPositionY },
	{ "HeadPositionZ", &ColumnHandler_DM_HeadPositionZ },
	{ "HeadRotationYaw", &ColumnHandler_DM_HeadRotationYaw },
	{ "HeadRotationPitch", &ColumnHandler_DM_HeadRotationPitch },
	{ "HeadRotationRoll", &ColumnHandler_DM_HeadRotationRoll },
	{ "CameraPositionZ", &ColumnHandler_DM_CameraPositionZ, true},
	{ "CameraPositionX", &ColumnHandler_DM_CameraPositionX, true},
	{ "CameraPositionY", &ColumnHandler_DM_CameraPositionY, true},
	{ "Availability", &ColumnHandler_Availability, true},
	{ "HorizontalMouthOpening", &ColumnHandler_DM_HorizontalMouthOpening },
	{ "VerticalMouthOpening",   &ColumnHandler_DM_VerticalMouthOpening },
	{ "FacialLandmarks_$LX68$", &ColumnHandler_FillFacialLandmarks, false, true },
	{ "FaceBoundingBox3DVisible", &ColumnHandler_FaceBB3DVisible, false, true },
	{ "FaceBoundingBox3D_$V3DX8$", &ColumnHandler_FillFaceBoundingBox3D, false, true },
	{ "FaceBoundingBox2DVisible", &ColumnHandler_FaceBB2DVisible, false, true },
	{ "FaceBoundingBox2D_$BB2D$", &ColumnHandler_FillFaceBoundingBox2D, false, true },
	{ "RightEyeBoundingBox2DVisible", &ColumnHandler_RightEyeBB2DVisible, false, true},
	{ "RightEyeBoundingBox2D_$BB2D$", &ColumnHandler_FillRightEyeBoundingBox2D, false, true },
	{ "LeftEyeBoundingBox2DVisible", &ColumnHandler_LeftEyeBB2DVisible, false, true},
	{ "LeftEyeBoundingBox2D_$BB2D$", &ColumnHandler_FillLeftEyeBoundingBox2D, false, true },

	SET_COLUMN_PROCESSOR_POINT(LeftShoulderPoint)
	SET_COLUMN_PROCESSOR_POINT(RightShoulderPoint)
	SET_COLUMN_PROCESSOR_POINT(LeftElbowPoint)
	SET_COLUMN_PROCESSOR_POINT(RightElbowPoint)
	SET_COLUMN_PROCESSOR_POINT(LeftWristPoint)
	SET_COLUMN_PROCESSOR_POINT(RightWristPoint)
	SET_COLUMN_PROCESSOR_POINT(LeftPinkyKnucklePoint)
	SET_COLUMN_PROCESSOR_POINT(RightPinkyKnucklePoint)
	SET_COLUMN_PROCESSOR_POINT(LeftIndexKnucklePoint)
	SET_COLUMN_PROCESSOR_POINT(RightIndexKnucklePoint)
	SET_COLUMN_PROCESSOR_POINT(LeftThumbKnucklePoint)
	SET_COLUMN_PROCESSOR_POINT(RightThumbKnucklePoint)
	SET_COLUMN_PROCESSOR_POINT(LeftHipPoint)
	SET_COLUMN_PROCESSOR_POINT(RightHipPoint)
	SET_COLUMN_PROCESSOR_POINT(LeftKneePoint)
	SET_COLUMN_PROCESSOR_POINT(RightKneePoint)
	SET_COLUMN_PROCESSOR_POINT(LeftAnklePoint)
	SET_COLUMN_PROCESSOR_POINT(RightAnklePoint)
	SET_COLUMN_PROCESSOR_POINT(LeftHeelPoint)
	SET_COLUMN_PROCESSOR_POINT(RightHeelPoint)
	SET_COLUMN_PROCESSOR_POINT(LeftFootIndexPoint)
	SET_COLUMN_PROCESSOR_POINT(RightFootIndexPoint)
};

FVector ConvertPointToBaseCoordinateSpace(const DMSSimCoordinateSpace& CoordinateSpace, const FVector& Point) {
	FVector NewPoint(Point);
	const auto Rotation = CoordinateSpace.GetRotation();
	const auto Translation = CoordinateSpace.GetTranslation();
	const auto Scale = CoordinateSpace.GetScale();
	NewPoint /= DMSSIM_M_TO_CM;
	NewPoint /= Scale;
	NewPoint = Rotation.UnrotateVector(NewPoint);
	NewPoint -= Translation;
	return NewPoint;
}

FVector ConvertVectorToBaseCoordinateSpace(const DMSSimCoordinateSpace& CoordinateSpace, const FVector& Vector) {
	FVector NewVector(Vector);
	const auto Rotation = CoordinateSpace.GetRotation();
	const auto Translation = CoordinateSpace.GetTranslation();
	const auto Scale = CoordinateSpace.GetScale();
	NewVector /= Scale;
	NewVector = Rotation.UnrotateVector(NewVector);
	NewVector.Normalize();
	return NewVector;
}

void ComputeGroundTruthData(const double Time, const DMSSimGroundTruthCommon& Common, const DMSSimGroundTruthOccupant& Frame, DMSSimFrameComputed& DMSSimFrameComputed) {
	memset(&DMSSimFrameComputed, 0, sizeof(DMSSimFrameComputed));
	if (!Frame.Initialized) { return; }
	DMSSimFrameComputed.Time = Time;

	FVector CoordinateOffset(-256.275f, 0.0f, -159.9f); // temporary hardcoded offset
	const auto& CarRotation_inWorld = Common.CarRotation_inWorld;
	const auto& FrontAxleMidPoint = Common.CarPosition_inWorld;
	const auto& CoordinateSpace = DMSSimConfig::GetCoordinateSpace();
	const auto& Camera = DMSSimConfig::GetCamera();
	const FVector BetweenEarsPoint = (Frame.LEarPoint + Frame.REarPoint) / 2;
	FVector Forward = (Frame.NosePoint - BetweenEarsPoint);
	Forward.Normalize();
	FVector Right = (Frame.REarPoint - Frame.LEarPoint);
	Right.Normalize();
	FVector Up = Forward ^ Right;
	Forward = ConvertVectorToBaseCoordinateSpace(CoordinateSpace, CarRotation_inWorld.UnrotateVector(Forward));
	Right = ConvertVectorToBaseCoordinateSpace(CoordinateSpace, CarRotation_inWorld.UnrotateVector(Right));
	Up = ConvertVectorToBaseCoordinateSpace(CoordinateSpace, CarRotation_inWorld.UnrotateVector(Up));
	DMSSimFrameComputed.HeadRotation = FTransform(Forward, Right, Up, FVector(0, 0, 0)).Rotator();
	DMSSimFrameComputed.HeadRotation *= -1.0f;
	if (Camera.GetMirrored()) { DMSSimFrameComputed.HeadRotation.Yaw *= -1.0f; }

	const auto TransformPoint = [&CoordinateSpace, &CarRotation_inWorld, FrontAxleMidPoint](const FVector& Point) {
		return ConvertPointToBaseCoordinateSpace(CoordinateSpace, CarRotation_inWorld.UnrotateVector(Point - FrontAxleMidPoint));
	};

#define TRANSFORM_POINT(x) DMSSimFrameComputed.##x = TransformPoint(Frame.##x);

	DMSSimFrameComputed.HeadPosition = TransformPoint(Frame.HeadOriginEyesCenter_inCam);
	DMSSimFrameComputed.CameraPosition = TransformPoint(Common.Camera.Position_inCar);
	DMSSimFrameComputed.CameraRotation = FRotator(0, 0, 0); // Identity

	TRANSFORM_POINT(LeftEyePoint)
	TRANSFORM_POINT(RightEyePoint)
	TRANSFORM_POINT(LeftGazeOrigin_inCam)
	TRANSFORM_POINT(RightGazeOrigin_inCam)
	TRANSFORM_POINT(LeftShoulderPoint)
	TRANSFORM_POINT(RightShoulderPoint)
	TRANSFORM_POINT(LeftElbowPoint)
	TRANSFORM_POINT(RightElbowPoint)
	TRANSFORM_POINT(LeftWristPoint)
	TRANSFORM_POINT(RightWristPoint)
	TRANSFORM_POINT(LeftPinkyKnucklePoint)
	TRANSFORM_POINT(RightPinkyKnucklePoint)
	TRANSFORM_POINT(LeftIndexKnucklePoint)
	TRANSFORM_POINT(RightIndexKnucklePoint)
	TRANSFORM_POINT(LeftThumbKnucklePoint)
	TRANSFORM_POINT(RightThumbKnucklePoint)
	TRANSFORM_POINT(LeftHipPoint)
	TRANSFORM_POINT(RightHipPoint)
	TRANSFORM_POINT(LeftKneePoint)
	TRANSFORM_POINT(RightKneePoint)
	TRANSFORM_POINT(LeftAnklePoint)
	TRANSFORM_POINT(RightAnklePoint)
	TRANSFORM_POINT(LeftHeelPoint)
	TRANSFORM_POINT(RightHeelPoint)
	TRANSFORM_POINT(LeftFootIndexPoint)
	TRANSFORM_POINT(RightFootIndexPoint)

#undef TRANSFORM_POINT

	DMSSimFrameComputed.GazeOrigin_inCam = (DMSSimFrameComputed.LeftGazeOrigin_inCam + DMSSimFrameComputed.RightGazeOrigin_inCam) * 0.5f;
	DMSSimFrameComputed.LeftGazeDirection_inCam = ConvertVectorToBaseCoordinateSpace(CoordinateSpace, CarRotation_inWorld.UnrotateVector(Frame.LeftGazeDirection_inCam));
	DMSSimFrameComputed.RightGazeDirection_inCam = ConvertVectorToBaseCoordinateSpace(CoordinateSpace, CarRotation_inWorld.UnrotateVector(Frame.RightGazeDirection_inCam));
	DMSSimFrameComputed.GazeDirection_inCam = (DMSSimFrameComputed.LeftGazeDirection_inCam + DMSSimFrameComputed.RightGazeDirection_inCam);
	DMSSimFrameComputed.GazeDirection_inCam.Normalize();
	DMSSimFrameComputed.HorizontalMouthOpening = Frame.HorizontalMouthOpening;
	DMSSimFrameComputed.VerticalMouthOpening = Frame.VerticalMouthOpening;
	
	// Transform facial landmarks and save them.
	DMSSimFrameComputed.FacialLandmarksVisible.Empty();
	for (bool landmarkVisble : Frame.FacialLandmarksVisible){ DMSSimFrameComputed.FacialLandmarksVisible.Add(landmarkVisble); }

	DMSSimFrameComputed.FacialLandmarks3D.Empty();
	for (FVector landmark : Frame.FacialLandmarks3D_inCam){ DMSSimFrameComputed.FacialLandmarks3D.Add(landmark); }
	
	DMSSimFrameComputed.FacialLandmarks2D.Empty();
	for (FVector2D landmark : Frame.FacialLandmarks2D){ DMSSimFrameComputed.FacialLandmarks2D.Add(landmark); }

	DMSSimFrameComputed.FaceBoundingBox3D.Empty();
	for (FVector landmark : Frame.FaceBoundingBox3D_inCam){ DMSSimFrameComputed.FaceBoundingBox3D.Add(landmark); }
	DMSSimFrameComputed.FaceBoundingBox3DVisible = Frame.FaceBoundingBox3DVisible;
	DMSSimFrameComputed.FaceBoundingBox2DVisible = Frame.FaceBoundingBox2DVisible;
	DMSSimFrameComputed.FaceBoundingBox2D = Frame.FaceBoundingBox2D;
	DMSSimFrameComputed.RightEyeBoundingBox2DVisible = Frame.RightEyeBoundingBox2DVisible;
	DMSSimFrameComputed.RightEyeBoundingBox2D = Frame.RightEyeBoundingBox2D;
	DMSSimFrameComputed.LeftEyeBoundingBox2DVisible = Frame.LeftEyeBoundingBox2DVisible;
	DMSSimFrameComputed.LeftEyeBoundingBox2D = Frame.LeftEyeBoundingBox2D;

}
} // anonymous namespace

namespace RegexPatterns {
	inline const std::regex& Landmark() {
		static const std::regex instance(R"(((\w+)_\$LX(\d+)\$))");
		return instance;
	}
	inline const std::regex& Vector() {
		static const std::regex instance(R"(((\w+)_\$V3DX(\d+)\$))");
		return instance;
	}
	inline const std::regex& Vector2D() {
		static const std::regex instance(R"(((\w+)_\$V2DX(\d+)\$))");
		return instance;
	}	
	inline const std::regex& BoundingBox2D() {
		static const std::regex instance(R"(((\w+)_\$BB2D\$))");
		return instance;
	}
}
void DMSSimGroundTruthRecorder::AddHeader(std::ostream& Stream) {
	FrameNumber_ = 0;

	bool RowStarted = false;
	for (const auto& Occupant : GTOccupantInfos) {
		for (const ColumnHandler& Column : Columns) {
			if (Column.DriverOnly && Occupant.Occupant != FDMSSimOccupantType::Driver) { continue; }
			if (PRINT_ALL_COLUMNS || Column.Func) {
				auto AddColumn = [&Stream, &RowStarted](const std::string& columnName, bool columnWithoutPrefix, const auto& Occupant) {
					if (RowStarted) { Stream << ";"; }
					Stream << "\"";
					if (!columnWithoutPrefix) { Stream << "s_DM_"; }
					Stream << Occupant.Prefix;
					Stream << columnName << "\"";
					RowStarted = true;
				};
				std::smatch match;
				std::string columnName = Column.ColumnName;
				bool columnWithoutPrefix = Column.WithoutPrefix;
				
				if (std::regex_search(columnName, match, RegexPatterns::Landmark()) && match.size() > 3) {
					std::string baseName = match[2].str();			// Captures e.g. "FacialLandmarks"
					int numLandmarks = std::stoi(match[3].str());	// Captures "10" and converts to int
					for (int i = 1; i <= numLandmarks; i++) {
						AddColumn(baseName + "_" + std::to_string(i) + "_" + "visible", columnWithoutPrefix, Occupant);
						AddColumn(baseName + "_" + std::to_string(i) + "_" + "x", columnWithoutPrefix, Occupant);
						AddColumn(baseName + "_" + std::to_string(i) + "_" + "y", columnWithoutPrefix, Occupant);
						AddColumn(baseName + "_" + std::to_string(i) + "_" + "z", columnWithoutPrefix, Occupant);
						AddColumn(baseName + "_" + std::to_string(i) + "_" + "u", columnWithoutPrefix, Occupant);
						AddColumn(baseName + "_" + std::to_string(i) + "_" + "v", columnWithoutPrefix, Occupant);
					}
				}
				else if (std::regex_search(columnName, match, RegexPatterns::Vector()) && match.size() > 3) {
					std::string baseName = match[2].str();			// Captures e.g. "FacialLandmarks"
					int numVectors = std::stoi(match[3].str());	// Captures "10" and converts to int
					for (int i = 0; i < numVectors; i++) {
						AddColumn(baseName + "_" + std::to_string(i) + "_" + "x", columnWithoutPrefix, Occupant);
						AddColumn(baseName + "_" + std::to_string(i) + "_" + "y", columnWithoutPrefix, Occupant);
						AddColumn(baseName + "_" + std::to_string(i) + "_" + "z", columnWithoutPrefix, Occupant);
					}
				}
				else if (std::regex_search(columnName, match, RegexPatterns::Vector2D()) && match.size() > 3) {
					std::string baseName = match[2].str();			// Captures e.g. "FacialLandmarks"
					int numVectors = std::stoi(match[3].str());	// Captures "10" and converts to int
					for (int i = 0; i < numVectors; i++) {
						AddColumn(baseName + "_" + std::to_string(i) + "_" + "x", columnWithoutPrefix, Occupant);
						AddColumn(baseName + "_" + std::to_string(i) + "_" + "y", columnWithoutPrefix, Occupant);
					}
				}
				else if (std::regex_search(columnName, match, RegexPatterns::BoundingBox2D())) {
					std::string baseName = match[2].str();			// Captures e.g. "FacialLandmarks"

					AddColumn(baseName + "_" + "x", columnWithoutPrefix, Occupant);
					AddColumn(baseName + "_" + "y", columnWithoutPrefix, Occupant);
					AddColumn(baseName + "_" + "w", columnWithoutPrefix, Occupant);
					AddColumn(baseName + "_" + "h", columnWithoutPrefix, Occupant);
				}
				else {
					AddColumn(columnName, columnWithoutPrefix, Occupant);
				}				
			}
		}
	}
	Stream << std::endl;
}

void DMSSimGroundTruthRecorder::AddFrame(std::ostream& Stream, const double Time, const DMSSimGroundTruthFrame& Frame) {
	if (!Stream.good()) { return; }

	bool RowStarted = false;
	for (const auto& Occupant : GTOccupantInfos) {
		DMSSimFrameComputed FrameComputed{};

		ComputeGroundTruthData(Time, Frame.Common, Frame.Occupants[static_cast<uint8>(Occupant.Occupant)], FrameComputed);
		for (const auto& Column : Columns) {
			if (Column.DriverOnly && Occupant.Occupant != FDMSSimOccupantType::Driver) { continue; }

			if (PRINT_ALL_COLUMNS || Column.Func) {
				if (RowStarted) { Stream << ";"; }
				Stream << "\"";
				if (Column.Func) { Column.Func(Stream, FrameComputed); }
				else { Stream << "0"; }
				Stream << "\"";
				RowStarted = true;
			}
		}
	}
	Stream << std::endl;
	++FrameNumber_;
}