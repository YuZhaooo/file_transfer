#include "DMSSimImageLabeler.h"
#include "DMSSimConstants.h"
#include "DMSSimLog.h"

#include <sstream>
#include <fstream>
#include <iomanip>
#include <cmath>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

using BoolAttributes = std::vector<std::pair<std::string, bool>>;
using NumAttributes = std::vector<std::pair<std::string, float>>;
using TextAttributes = std::vector<std::pair<std::string, std::string>>;

const auto SOURCE_SDT_ATTR = std::pair<std::string, std::string>{ "source", "sdt" };
const auto STATE_LABELED_ATTR = std::pair<std::string, std::string>{ "state", "labeled" };
const auto QUALITY_SDT_ATTR = std::pair<std::string, float>{"quality", SDT_QUALITY};

const auto DEFAULT_ATTRS_TEXT = TextAttributes{ SOURCE_SDT_ATTR, STATE_LABELED_ATTR };
const auto DEFAULT_ATTRS_NUM = NumAttributes{ QUALITY_SDT_ATTR};


struct OccupantBasicCharacterictics {
	uint8 Height;
	uint8 Weight;
	uint8 Age;
	std::string AgeCategory;
	std::string Gender;
	std::string SkinTone;
	std::string DeleteToken;
};

static std::map<FString, OccupantBasicCharacterictics> MetahumanCharacterictics = {
	{"Gavin", {172, 72, 41, "adult", "male", "3_olive", "SYNTH00001"}},
	{"Hana", {152, 46, 21, "adult", "female", "2_fair", "SYNTH00002"}},
	{"Ada", {160, 54, 29, "adult", "female", "5_brown", "SYNTH00003"}},
	{"Benedikt", {174, 85, 35, "adult", "male", "2_fair", "SYNTH00004"}},
	{"Jesse", {180, 77, 30, "adult", "male", "5_brown", "SYNTH00005"}},
	{"Mylen", {172, 69, 27, "adult", "male", "3_olive", "SYNTH00006"}},
	{"Bernice", {162, 49, 32, "adult", "female", "2_fair", "SYNTH00007"}},
	{"Danielle", {165, 53, 28, "adult", "female", "3_olive", "SYNTH00008"}},
	{"Glenda", {166, 52, 77, "senior", "female", "2_fair", "SYNTH00009"}},
	{"Keiji", {169, 72, 71, "senior", "male", "3_olive", "SYNTH00010"}},
	{"Lucian", {176, 74, 70, "senior", "male", "6_black_brown", "SYNTH00011"}},
	{"Maria", {164, 96, 41, "adult", "female", "4_light_brown", "SYNTH00012"}},
	{"Neema", {173, 58, 67, "senior", "female", "5_brown", "SYNTH00013"}},
	{"Omar", {180, 82, 39, "adult", "male", "3_olive", "SYNTH00014"}},
	{"Roux", {164, 65, 36, "adult", "female", "3_olive", "SYNTH00015"}},
	{"Sook-ja", {166, 58, 80, "senior", "female", "3_olive", "SYNTH00016"}},
	{"Stephane", {185, 80, 79, "senior", "male", "3_olive", "SYNTH00017"}},
	{"Taro", {179, 79, 33, "adult", "male", "3_olive", "SYNTH00018"}},
	{"Trey", {184, 84, 28, "adult", "male", "5_brown", "SYNTH00019"}}
};

const std::map<int, std::string> EyeLandmarksNames{
	{37, "right_eye_corner_out_eyelid"},
	{38, "right_eye_up_out_eyelid"},
	{39, "right_eye_up_in_eyelid"},
	{40, "right_eye_corner_in_eyelid"},
	{41, "right_eye_down_in_eyelid"},
	{42, "right_eye_down_out_eyelid"},
	{43, "left_eye_corner_in_eyelid"},
	{44, "left_eye_up_in_eyelid"},
	{45, "left_eye_up_out_eyelid"},
	{46, "left_eye_corner_out_eyelid"},
	{47, "left_eye_down_out_eyelid"},
	{48, "left_eye_down_in_eyelid"},
};

static std::string GetEyeStatus(const float PupilVisibility, const float EyelidVisibility) {
	if (EyelidVisibility < UNKNOWN_STATE_VISIBILITY_THRESHOLD) { return "unknown"; }
	if (PupilVisibility > 0.0) { return "open"; }
	return "closed";
}

static std::string GetSeatPositionString(FDMSSimOccupantType Type) {
	switch (Type) {
	case FDMSSimOccupantType::Driver:
		return "driver";
	case FDMSSimOccupantType::PassengerFront:
		return "codriver";
	case FDMSSimOccupantType::PassengerRearLeft:
		return "second_nearside";
	case FDMSSimOccupantType::PassengerRearMiddle:
		return "second_middle";
	case FDMSSimOccupantType::PassengerRearRight:
		return "second_offside";
	default:
		return "undefined";
	}
}

static std::string GetGlasses(const int type) {
	switch (type) {
	case 7:
	case 8:
	case 15:
		return "sunglasses";
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
		return "glasses";
	default: 
		return "no_glasses";
	}
}

static std::string GetHeadGear(const int type) {
	switch (type) {
	case 1:
	case 5:
		return "baseball_cap";
	case 2:
	case 7:
		return "beanie";
	case 3:
	case 4:
	case 6:
		return "other";
	default:
		return "no_headgear";
	}
}

static std::string GetFaceMask(const int type) {
	switch (type) {
	case 1:
	case 2:
	case 3:
	case 6:
		return "face_mask_simple";
	case 4:
	case 5:
	case 7:
		return "face_mask_ffp";
	default:
		return "no_face_mask";
	}
}

static rapidjson::Value CreateJsonBBoxes(const DMSSimGroundTruthOccupant& PrevGtOccupant, const DMSSimGroundTruthOccupant& GtOccupant, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value BBox(rapidjson::kArrayType);
	AddLabelWithCoordinatesAndAttributes(BBox, "face_bbox", PrevGtOccupant.FaceBoundingBox2D, "frame", allocator, BoolAttributes{ {"visible", GtOccupant.FaceBoundingBox2DVisible} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);
	AddLabelWithCoordinatesAndAttributes(BBox, "left_eye_bbox", PrevGtOccupant.LeftEyeBoundingBox2D, "frame", allocator, BoolAttributes{ {"visible", GtOccupant.LeftEyeBoundingBox2DVisible} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);
	AddLabelWithCoordinatesAndAttributes(BBox, "right_eye_bbox", PrevGtOccupant.RightEyeBoundingBox2D, "frame", allocator, BoolAttributes{ {"visible", GtOccupant.RightEyeBoundingBox2DVisible} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);
	return BBox;
}

static rapidjson::Value CreateJsonVec(const DMSSimGroundTruthOccupant& PrevGtOccupant, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value Vec(rapidjson::kArrayType);
	for (const auto& el : EyeLandmarksNames) {
		const auto& name = el.second;
		const auto& value = PrevGtOccupant.FacialLandmarks2D[el.first];
		const auto& visible = PrevGtOccupant.FacialLandmarksVisible[el.first];
		AddLabelWithCoordinatesAndAttributes(Vec, name, value, "frame", allocator, BoolAttributes{ {"visible", visible} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);
	}

	for (int k = 0; k < PrevGtOccupant.LeftEyePupilLandmarks2D.Num(); k++) {
		const bool visible = PrevGtOccupant.PupilIrisLandmarksVisible[k + 25];
		if (LandmarkPupilIrisNames[k] == "up" || LandmarkPupilIrisNames[k] == "down" || LandmarkPupilIrisNames[k] == "in" || LandmarkPupilIrisNames[k] == "out" || LandmarkPupilIrisNames[k] == "center") {
			AddLabelWithCoordinatesAndAttributes(Vec, "left_pupil_" + LandmarkPupilIrisNames[k], PrevGtOccupant.LeftEyePupilLandmarks2D[k], "frame", allocator, BoolAttributes{ {"visible", visible} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);
		}
	}

	for (int k = 0; k < PrevGtOccupant.RightEyePupilLandmarks2D.Num(); k++) {
		const bool visible = PrevGtOccupant.PupilIrisLandmarksVisible[k + 8];
		if (LandmarkPupilIrisNames[k] == "up" || LandmarkPupilIrisNames[k] == "down" || LandmarkPupilIrisNames[k] == "in" || LandmarkPupilIrisNames[k] == "out" || LandmarkPupilIrisNames[k] == "center") {
			AddLabelWithCoordinatesAndAttributes(Vec, "right_pupil_" + LandmarkPupilIrisNames[k], PrevGtOccupant.RightEyePupilLandmarks2D[k], "frame", allocator, BoolAttributes{ {"visible", visible} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);
		}
	}

	// head direction
	AddLabelWithCoordinatesAndAttributes(Vec, "head_direction", PrevGtOccupant.HeadDirection_inCam, "ifcd_sdt", allocator, BoolAttributes{ {"visible", true} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);
	AddLabelWithCoordinatesAndAttributes(Vec, "head_direction", PrevGtOccupant.HeadDirection_inCar, "vehicle", allocator, BoolAttributes{ {"visible", true} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);

	// head origin eyes center
	AddLabelWithCoordinatesAndAttributes(Vec, "head_origin_eyecenter", PrevGtOccupant.HeadOriginEyesCenter_inCam, "ifcd_sdt", allocator, BoolAttributes{ {"visible", true} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);
	AddLabelWithCoordinatesAndAttributes(Vec, "head_origin_eyecenter", PrevGtOccupant.HeadOriginEyesCenter_inCar, "vehicle", allocator, BoolAttributes{ {"visible", true} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);
	
	// head origin ear center
	AddLabelWithCoordinatesAndAttributes(Vec, "head_origin_earcenter", PrevGtOccupant.HeadOriginEarsCenter_inCam, "ifcd_sdt", allocator, BoolAttributes{ {"visible", true} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);
	AddLabelWithCoordinatesAndAttributes(Vec, "head_origin_earcenter", PrevGtOccupant.HeadOriginEarsCenter_inCar, "vehicle", allocator, BoolAttributes{ {"visible", true} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);

	//Head Rotation
	AddLabelWithCoordinatesAndAttributes(Vec, "head_rotation", PrevGtOccupant.HeadRotation_inCam, "ifcd_sdt", allocator, BoolAttributes{ {"visible", true} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);
	AddLabelWithCoordinatesAndAttributes(Vec, "head_rotation", PrevGtOccupant.HeadRotation_inCar, "vehicle", allocator, BoolAttributes{ {"visible", true} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);

	// Left Eye Gaze Direction
	AddLabelWithCoordinatesAndAttributes(Vec, "left_gaze_direction", PrevGtOccupant.LeftGazeDirection_inCam, "ifcd_sdt", allocator, BoolAttributes{ {"visible", true} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);
	AddLabelWithCoordinatesAndAttributes(Vec, "left_gaze_direction", PrevGtOccupant.LeftGazeDirection_inCar, "vehicle", allocator, BoolAttributes{ {"visible", true} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);

	// Right Eye Gaze Direction
	AddLabelWithCoordinatesAndAttributes(Vec, "right_gaze_direction", PrevGtOccupant.RightGazeDirection_inCam, "ifcd_sdt", allocator, BoolAttributes{ {"visible", true} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);
	AddLabelWithCoordinatesAndAttributes(Vec, "right_gaze_direction", PrevGtOccupant.RightGazeDirection_inCar, "vehicle", allocator, BoolAttributes{ {"visible", true} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);

	// Gaze Direction
	AddLabelWithCoordinatesAndAttributes(Vec, "gaze_direction", PrevGtOccupant.GazeDirection_inCam, "ifcd_sdt", allocator, BoolAttributes{ {"visible", true} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);
	AddLabelWithCoordinatesAndAttributes(Vec, "gaze_direction", PrevGtOccupant.GazeDirection_inCar, "vehicle", allocator, BoolAttributes{ {"visible", true} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);

	// Gaze Origin
	const auto& Left_inCam = PrevGtOccupant.LeftGazeOrigin_inCam;
	const auto& Right_inCam = PrevGtOccupant.RightGazeOrigin_inCam;
	const auto Mean_inCam = FVector((Left_inCam.X + Right_inCam.X) / 2, (Left_inCam.Y + Right_inCam.Y) / 2, (Left_inCam.Z + Right_inCam.Z) / 2);
	AddLabelWithCoordinatesAndAttributes(Vec, "left_gaze_origin", Left_inCam, "ifcd_sdt", allocator, BoolAttributes{ {"visible", true} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);
	AddLabelWithCoordinatesAndAttributes(Vec, "right_gaze_origin", Right_inCam, "ifcd_sdt", allocator, BoolAttributes{ {"visible", true} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);
	AddLabelWithCoordinatesAndAttributes(Vec, "gaze_origin", Mean_inCam, "ifcd_sdt", allocator, BoolAttributes{ {"visible", true} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);
	const auto& Left_inCar = PrevGtOccupant.LeftGazeOrigin_inCar;
	const auto& Right_inCar = PrevGtOccupant.RightGazeOrigin_inCar;
	const auto Mean_inCar = FVector((Left_inCar.X + Right_inCar.X) / 2, (Left_inCar.Y + Right_inCar.Y) / 2, (Left_inCar.Z + Right_inCar.Z) / 2);
	AddLabelWithCoordinatesAndAttributes(Vec, "left_gaze_origin", Left_inCar, "vehicle", allocator, BoolAttributes{ {"visible", true} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);
	AddLabelWithCoordinatesAndAttributes(Vec, "right_gaze_origin", Right_inCar, "vehicle", allocator, BoolAttributes{ {"visible", true} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);
	AddLabelWithCoordinatesAndAttributes(Vec, "gaze_origin", Mean_inCar, "vehicle", allocator, BoolAttributes{ {"visible", true} }, DEFAULT_ATTRS_NUM, DEFAULT_ATTRS_TEXT);

	return Vec;
}

static rapidjson::Value CreateDynamicOccupant(const TSharedPtr<DMSSimGroundTruthFrame>& PrevGroundTruth, const TSharedPtr<DMSSimGroundTruthFrame>& GroundTruth, const int i, rapidjson::Document::AllocatorType& allocator) {
	const uint8 Type =static_cast<uint8>(PrevGroundTruth->Common.Occupants[i].Type);
	const auto& PrevGtOccupant = PrevGroundTruth->Occupants[Type];
	const auto& GtOccupant = GroundTruth->Occupants[Type];
	const auto& PrevFDMSSimOccupant = PrevGroundTruth->Common.Occupants[i];

	rapidjson::Value Occupant(rapidjson::kObjectType);
	rapidjson::Value ObjectData(rapidjson::kObjectType);

	// Text data
	rapidjson::Value Text(rapidjson::kArrayType);
	AddLabel(Text, "right_eye_status", std::string(GetEyeStatus(PrevGtOccupant.RightEyePupilVisibilityPerc, PrevGtOccupant.RightEyeLidVisibilityPerc)), allocator);
	AddLabel(Text, "left_eye_status", std::string(GetEyeStatus(PrevGtOccupant.LeftEyePupilVisibilityPerc, PrevGtOccupant.LeftEyeLidVisibilityPerc)), allocator);
	AddLabel(Text, "face_expression", std::string("neutral"), allocator);
	AddLabel(Text, "face_occlusion", std::string("no_accessory"), allocator);
	AddLabel(Text, "glasses", GetGlasses(PrevGroundTruth->Common.Occupants[i].Glasses.Model), allocator);
	AddLabel(Text, "headgear", GetHeadGear(PrevGroundTruth->Common.Occupants[i].Headgear), allocator);
	AddLabel(Text, "face_mask", GetFaceMask(PrevGroundTruth->Common.Occupants[i].Mask), allocator);
	ObjectData.AddMember("text", Text, allocator);

	// Num data (opening, eye visibility, pupil visibility)
	const auto AsIntegerPerc = [](const float x) {return (x > 0 && x <= 1.0) ? static_cast<uint8_t>(x * 100) : 0; };
	rapidjson::Value Num(rapidjson::kArrayType);
	AddLabel(Num, "left_eye_opening", PrevGtOccupant.LeftEyeOpening * DMSSIM_CM_TO_MM, allocator);
	AddLabel(Num, "right_eye_opening", PrevGtOccupant.RightEyeOpening * DMSSIM_CM_TO_MM, allocator);
	AddLabel(Num, "left_eyelid_visibility", AsIntegerPerc(PrevGtOccupant.LeftEyeLidVisibilityPerc), allocator);
	AddLabel(Num, "right_eyelid_visibility", AsIntegerPerc(PrevGtOccupant.RightEyeLidVisibilityPerc), allocator);
	AddLabel(Num, "left_pupil_visibility", AsIntegerPerc(PrevGtOccupant.LeftEyePupilVisibilityPerc), allocator);
	AddLabel(Num, "right_pupil_visibility", AsIntegerPerc(PrevGtOccupant.RightEyePupilVisibilityPerc), allocator);
	ObjectData.AddMember("num", Num, allocator);
	ObjectData.AddMember("bbox", CreateJsonBBoxes(PrevGtOccupant, GtOccupant, allocator), allocator);
	ObjectData.AddMember("vec", CreateJsonVec(PrevGtOccupant, allocator), allocator);

	Occupant.AddMember("object_data", ObjectData, allocator);
	return Occupant;
}

static rapidjson::Value CreateFrames(const TSharedPtr<DMSSimGroundTruthFrame>& PrevGroundTruth, const TSharedPtr<DMSSimGroundTruthFrame>& GroundTruth, rapidjson::Document::AllocatorType& allocator) {
	const auto& Camera = PrevGroundTruth->Common.Camera;

	//intrinsics custom
	rapidjson::Value IntrinsicsCustom(rapidjson::kObjectType);
	rapidjson::Value CamMatrix3x3(rapidjson::kArrayType);
	rapidjson::Value Row0(rapidjson::kArrayType);
	rapidjson::Value Row1(rapidjson::kArrayType);
	rapidjson::Value Row2(rapidjson::kArrayType);
	Row0.PushBack(0.0, allocator).PushBack(0.0, allocator).PushBack(0.0, allocator);
	Row1.PushBack(0.0, allocator).PushBack(0.0, allocator).PushBack(0.0, allocator);
	Row2.PushBack(0.0, allocator).PushBack(0.0, allocator).PushBack(0.0, allocator);
	CamMatrix3x3.PushBack(Row0, allocator).PushBack(Row1, allocator).PushBack(Row2, allocator);
	rapidjson::Value Distortion(rapidjson::kArrayType);
	Distortion.PushBack(0.0, allocator).PushBack(0.0, allocator).PushBack(0.0, allocator).PushBack(0.0, allocator).PushBack(0.0, allocator);
	IntrinsicsCustom.AddMember("camera_matrix", CamMatrix3x3, allocator);
	IntrinsicsCustom.AddMember("distortion_coeffs", Distortion, allocator);
	IntrinsicsCustom.AddMember("height_px", PrevGroundTruth->Common.Camera.FrameSize.Y, allocator);
	IntrinsicsCustom.AddMember("width_px", PrevGroundTruth->Common.Camera.FrameSize.X, allocator);

	//sensor position rotation
	rapidjson::Value Position(rapidjson::kArrayType);
	rapidjson::Value Rotation(rapidjson::kArrayType);
	Position.PushBack(Camera.Position_inCar.X, allocator).PushBack(Camera.Position_inCar.Y, allocator).PushBack(Camera.Position_inCar.Z, allocator);
	Rotation.PushBack(Camera.Rotation_inCar.Roll, allocator).PushBack(Camera.Rotation_inCar.Pitch, allocator).PushBack(Camera.Rotation_inCar.Yaw, allocator);

	//acc intrinsics
	rapidjson::Value AccIntrinsics(rapidjson::kArrayType);
	AccIntrinsics.PushBack(0.0, allocator).PushBack(0.0, allocator).PushBack(0.0, allocator);

	//stream properties
	rapidjson::Value StreamProperties(rapidjson::kObjectType);
	StreamProperties.AddMember("intrinsics_custom", IntrinsicsCustom, allocator);
	StreamProperties.AddMember("sensor_position", Position, allocator);
	StreamProperties.AddMember("sensor_rotation", Rotation, allocator);
	StreamProperties.AddMember("holder", "prototype 1", allocator);
	StreamProperties.AddMember("steering_column_adjusted", false, allocator);
	StreamProperties.AddMember("coordinate_system", "vehicle", allocator);
	StreamProperties.AddMember("acc_intrinsics", AccIntrinsics, allocator);

	//ifcd_sdt
	rapidjson::Value IfcdSdt(rapidjson::kObjectType);
	IfcdSdt.AddMember("description", "driver camera in SDT", allocator);
	IfcdSdt.AddMember("stream_properties", StreamProperties, allocator);

	//streams
	rapidjson::Value Streams(rapidjson::kObjectType);
	Streams.AddMember("ifcd_sdt", IfcdSdt, allocator);
	
	//frame properties
	rapidjson::Value FrameProperties(rapidjson::kObjectType);
	FrameProperties.AddMember("timestamp", rapidjson::Value(TCHAR_TO_UTF8(*FDateTime::Now().ToString()), allocator), allocator);
	FrameProperties.AddMember("streams", Streams, allocator);

	//objects
	rapidjson::Value Objects(rapidjson::kObjectType);
	for (int i = 0; i < PrevGroundTruth->Common.OccupantCount; i++) {
		auto Occupant = CreateDynamicOccupant(PrevGroundTruth, GroundTruth, i, allocator);
		Objects.AddMember(rapidjson::Value(std::to_string(i).c_str(), allocator), Occupant, allocator);
	}

	rapidjson::Value Frame(rapidjson::kObjectType);
	Frame.AddMember("frame_properties", FrameProperties, allocator);
	Frame.AddMember("objects", Objects, allocator);

	rapidjson::Value Frames(rapidjson::kObjectType);
	Frames.AddMember("0", Frame, allocator);
	return Frames;
}

static rapidjson::Value CreateStaticOccupant(const TSharedPtr<DMSSimGroundTruthFrame>& PrevGroundTruth, const int i, rapidjson::Document::AllocatorType& allocator) {
	const auto Character = PrevGroundTruth->Common.Occupants[i].Character;

	rapidjson::Value Occupant(rapidjson::kObjectType);
	Occupant.AddMember("name", rapidjson::Value(TCHAR_TO_UTF8(*Character), allocator), allocator);
	Occupant.AddMember("type", "metahuman", allocator);

	rapidjson::Value TextData(rapidjson::kArrayType);
	AddLabel(TextData, "age_category", MetahumanCharacterictics[Character].AgeCategory, allocator);
	AddLabel(TextData, "contact_lenses", std::string("no_contact_lenses"), allocator);
	AddLabel(TextData, "delete_token", MetahumanCharacterictics[Character].DeleteToken, allocator);
	AddLabel(TextData, "gender", MetahumanCharacterictics[Character].Gender, allocator);
	AddLabel(TextData, "seat_position", GetSeatPositionString(PrevGroundTruth->Common.Occupants[i].Type), allocator);
	AddLabel(TextData, "skin_tone", MetahumanCharacterictics[Character].SkinTone, allocator);
	AddLabel(TextData, "eye_makeup", std::string("no_makeup_or_light"), allocator);
	AddLabel(TextData, "facial_accessories", std::string("no_accessories"), allocator);
	AddLabel(TextData, "facial_hair", std::string("no_facial_hair"), allocator);

	rapidjson::Value NumData(rapidjson::kArrayType);
	AddLabel(NumData, "age", MetahumanCharacterictics[Character].Age, allocator);
	AddLabel(NumData, "height", MetahumanCharacterictics[Character].Height, allocator);
	AddLabel(NumData, "weight", MetahumanCharacterictics[Character].Weight, allocator);
	AddLabel(NumData, "eye_opening", (MaxLeftEyeOpenings[Character] + MaxRightEyeOpenings[Character]) * 0.5 * DMSSIM_CM_TO_MM, allocator);

	rapidjson::Value ObjectData(rapidjson::kObjectType);
	ObjectData.AddMember("text", TextData, allocator);
	ObjectData.AddMember("num", NumData, allocator);
	Occupant.AddMember("object_data", ObjectData, allocator);

	return Occupant;
}

static rapidjson::Value CreateStreams(const TSharedPtr<DMSSimGroundTruthFrame>& PrevGroundTruth, rapidjson::Document::AllocatorType& allocator) {
	const auto& Camera = PrevGroundTruth->Common.Camera;

	//intrinsics custom
	rapidjson::Value IntrinsicsCustom(rapidjson::kObjectType);
	rapidjson::Value CamMatrix3x3(rapidjson::kArrayType);
	float focal_len = DMSSimConfig::GetCamera().GetFrameWidth() * 0.5f / std::tan(FMath::DegreesToRadians(DMSSimConfig::GetCamera().GetFOV() * 0.5));
	float cx = DMSSimConfig::GetCamera().GetFrameWidth() * 0.5f;
	float cy = DMSSimConfig::GetCamera().GetFrameHeight() * 0.5f;
	rapidjson::Value Row0(rapidjson::kArrayType);
	rapidjson::Value Row1(rapidjson::kArrayType);
	rapidjson::Value Row2(rapidjson::kArrayType);
	Row0.PushBack(focal_len, allocator).PushBack(0.0, allocator).PushBack(cx, allocator);
	Row1.PushBack(0.0, allocator).PushBack(focal_len, allocator).PushBack(cy, allocator);
	Row2.PushBack(0.0, allocator).PushBack(0.0, allocator).PushBack(1.0, allocator);
	CamMatrix3x3.PushBack(Row0, allocator).PushBack(Row1, allocator).PushBack(Row2, allocator);
	rapidjson::Value Distortion(rapidjson::kArrayType);
	Distortion.PushBack(0.0, allocator).PushBack(0.0, allocator).PushBack(0.0, allocator).PushBack(0.0, allocator).PushBack(0.0, allocator);
	IntrinsicsCustom.AddMember("camera_matrix", CamMatrix3x3, allocator);
	IntrinsicsCustom.AddMember("distortion_coeffs", Distortion, allocator);
	IntrinsicsCustom.AddMember("height_px", PrevGroundTruth->Common.Camera.FrameSize.Y, allocator);
	IntrinsicsCustom.AddMember("width_px", PrevGroundTruth->Common.Camera.FrameSize.X, allocator);

	//sensor position rotation
	rapidjson::Value Position(rapidjson::kArrayType);
	rapidjson::Value Rotation(rapidjson::kArrayType);
	Position.PushBack(Camera.Position_inCar.X, allocator).PushBack(Camera.Position_inCar.Y, allocator).PushBack(Camera.Position_inCar.Z, allocator);
	Rotation.PushBack(Camera.Rotation_inCar.Roll, allocator).PushBack(Camera.Rotation_inCar.Pitch, allocator).PushBack(Camera.Rotation_inCar.Yaw, allocator);

	//acc intrinsics
	rapidjson::Value AccIntrinsics(rapidjson::kArrayType);
	AccIntrinsics.PushBack(0.0, allocator).PushBack(0.0, allocator).PushBack(0.0, allocator);

	//stream properties
	rapidjson::Value StreamProperties(rapidjson::kObjectType);
	StreamProperties.AddMember("intrinsics_custom", IntrinsicsCustom, allocator);
	StreamProperties.AddMember("sensor_position", Position, allocator);
	StreamProperties.AddMember("sensor_rotation", Rotation, allocator);
	StreamProperties.AddMember("holder", "prototype 1", allocator);
	StreamProperties.AddMember("steering_column_adjusted", false, allocator);
	StreamProperties.AddMember("coordinate_system", "vehicle", allocator);
	StreamProperties.AddMember("acc_intrinsics", AccIntrinsics, allocator);
	
	rapidjson::Value StreamIfcd(rapidjson::kObjectType);
	StreamIfcd.AddMember("description", "driver camera in SDT", allocator);
	StreamIfcd.AddMember("stream_properties", StreamProperties, allocator);

	rapidjson::Value Streams(rapidjson::kObjectType);
	Streams.AddMember("ifcd_sdt", StreamIfcd, allocator);
	return Streams;
}

static rapidjson::Value CreateObjects(const TSharedPtr<DMSSimGroundTruthFrame>& PrevGroundTruth, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value Objects(rapidjson::kObjectType);

	const auto OccupantCount = PrevGroundTruth->Common.OccupantCount;
	for (int i = 0; i < OccupantCount; i++) {
		auto Occupant = CreateStaticOccupant(PrevGroundTruth, i, allocator);
		Objects.AddMember(rapidjson::Value(std::to_string(i).c_str(), allocator), Occupant, allocator);
	}

	return Objects;
}

static rapidjson::Value CreateCoordinateSystems(const DMSSimGroundTruthCommon& GT, rapidjson::Document::AllocatorType& allocator) {
	//vehicle coordinate sys
	rapidjson::Value CoordSysVeh(rapidjson::kObjectType);
	rapidjson::Value ChildrenVehCoord(rapidjson::kArrayType);
	ChildrenVehCoord.PushBack("ifcd_sdt", allocator);
	CoordSysVeh.AddMember("type", "vw_construction_VWC-CS", allocator);
	CoordSysVeh.AddMember("parent", "", allocator);
	CoordSysVeh.AddMember("children", ChildrenVehCoord, allocator);

	//ifcd coordinate sys
	rapidjson::Value CoordSysIfcd(rapidjson::kObjectType);
	rapidjson::Value ChildrenIfcdCoord(rapidjson::kArrayType);
	ChildrenIfcdCoord.PushBack("frame", allocator);
	rapidjson::Value Mat4x4Ifcd(rapidjson::kArrayType);
	auto TransformToCar = FTransform(GT.Camera.Rotation_inCar, GT.Camera.Position_inCar).ToMatrixWithScale();
	for (int r = 0; r < 4; r++) {
		for (int c = 0; c < 4; c++) { Mat4x4Ifcd.PushBack(TransformToCar.M[c][r], allocator); }
	}
	rapidjson::Value PoseIfcd(rapidjson::kObjectType);
	PoseIfcd.AddMember("matrix4x4", Mat4x4Ifcd, allocator);
	CoordSysIfcd.AddMember("type", "cartesian_sensor_CS-CS", allocator);
	CoordSysIfcd.AddMember("parent", "vehicle", allocator);
	CoordSysIfcd.AddMember("children", ChildrenIfcdCoord, allocator);
	CoordSysIfcd.AddMember("pose_wrt_parent", PoseIfcd, allocator);

	//frame coordinate sys (x beeing optical axis of the camera)
	rapidjson::Value CoordSysFrame(rapidjson::kObjectType);
	rapidjson::Value Mat3x4Frame(rapidjson::kArrayType);
	float focal_len = DMSSimConfig::GetCamera().GetFrameWidth() * 0.5f / std::tan(FMath::DegreesToRadians(DMSSimConfig::GetCamera().GetFOV() * 0.5));
	float cx = DMSSimConfig::GetCamera().GetFrameWidth() * 0.5f;
	float cy = DMSSimConfig::GetCamera().GetFrameHeight() * 0.5f;
	Mat3x4Frame.PushBack(cx, allocator).PushBack(-focal_len, allocator).PushBack(0.0, allocator).PushBack(0.0, allocator);
	Mat3x4Frame.PushBack(cy, allocator).PushBack(0.0, allocator).PushBack(-focal_len, allocator).PushBack(0.0, allocator);
	Mat3x4Frame.PushBack(1.0, allocator).PushBack(0.0, allocator).PushBack(0.0, allocator).PushBack(0.0, allocator);
	rapidjson::Value PoseFrame(rapidjson::kObjectType);
	PoseFrame.AddMember("matrix3x4", Mat3x4Frame, allocator);
	CoordSysFrame.AddMember("type", "frame", allocator);
	CoordSysFrame.AddMember("parent", "ifcd_sdt", allocator);
	CoordSysFrame.AddMember("children", rapidjson::Value(rapidjson::kArrayType), allocator);
	CoordSysFrame.AddMember("pose_wrt_parent", PoseFrame, allocator);

	rapidjson::Value CoordSys(rapidjson::kObjectType);
	CoordSys.AddMember("vehicle", CoordSysVeh, allocator);
	CoordSys.AddMember("ifcd_sdt", CoordSysIfcd, allocator);
	CoordSys.AddMember("frame", CoordSysFrame, allocator);
	return CoordSys;
}

static rapidjson::Value CreateMetadata(const DMSSimGroundTruthCommon& GT, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value Metadata;
	Metadata.SetObject();

	// Schema version
	std::string schemaVersion = std::to_string(GT.VersionMajor) + "." + std::to_string(GT.VersionMinor);
	Metadata.AddMember("schema_version", "1.0.0", allocator);

	// Description
	Metadata.AddMember("name", rapidjson::Value(GT.Description.c_str(), allocator), allocator);
	Metadata.AddMember("annotator", "DMS Simulation", allocator);
	Metadata.AddMember("ics_label_guidelines_version", "2.0.2", allocator);

	return Metadata;
}

void DMSSimImageLabelerImpl::AddFrame(const TSharedPtr<DMSSimGroundTruthFrame>& PrevGroundTruth, const TSharedPtr<DMSSimGroundTruthFrame>& GroundTruth, int FrameIdx) {
	std::wstringstream ss;
	ss << BaseFileName_ << L"_" << std::setw(5) << std::setfill(L'0') << FrameIdx << L".json";

	// Serialize JSON to string
	rapidjson::StringBuffer buffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
	CreateLabelFile(PrevGroundTruth, GroundTruth).Accept(writer);
	std::ofstream JsonFile(ss.str());
	if (JsonFile.is_open()) {
		JsonFile << buffer.GetString();
		JsonFile.close();
	}
	else { DMSSimLog::Info() << "fail at creating json file" << FL; }
}

rapidjson::Document DMSSimImageLabelerImpl::CreateLabelFile(const TSharedPtr<DMSSimGroundTruthFrame>& PrevGroundTruth, const TSharedPtr<DMSSimGroundTruthFrame>& GroundTruth) {
	rapidjson::Document Root;
	Root.SetObject();
	rapidjson::Document::AllocatorType& allocator = Root.GetAllocator();
	Root.AddMember("openlabel", rapidjson::Value(rapidjson::kObjectType), allocator);

	rapidjson::Value Metadata = CreateMetadata(PrevGroundTruth->Common, allocator);
	rapidjson::Value CoordinateSystems = CreateCoordinateSystems(PrevGroundTruth->Common, allocator);
	rapidjson::Value Objects = CreateObjects(PrevGroundTruth, allocator);
	rapidjson::Value Streams = CreateStreams(PrevGroundTruth, allocator);
	rapidjson::Value Frames = CreateFrames(PrevGroundTruth, GroundTruth, allocator);
	Root["openlabel"].AddMember("metadata", Metadata, allocator);
	Root["openlabel"].AddMember("coordinate_systems", CoordinateSystems, allocator);
	Root["openlabel"].AddMember("objects", Objects, allocator);
	Root["openlabel"].AddMember("streams", Streams, allocator);
	Root["openlabel"].AddMember("frames", Frames, allocator);
	return Root;
}