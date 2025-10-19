#include "DMSSimImageLabeler.h"
#include "DMSSimConstants.h"
#include "DMSSimLog.h"

#include <sstream>
#include <fstream>
#include <iomanip>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>


static bool IsEyeOpened(const float EyeOpening, const float MaxEyeOpening, const float EyesOpenThreshhold) { return EyeOpening / MaxEyeOpening > EyesOpenThreshhold; }

static std::string GetSeatPositionString(uint8 Type) {
	switch (Type) {
	case 0:
		return "driver";
	case 1:
		return "codriver";
	case 2:
		return "second_nearside";
	case 3:
		return "second_middle";
	case 4:
		return "second_offside";
	default:
		return "undefined";
	}
}

static rapidjson::Document CreateJsonMetadataOld(const DMSSimGroundTruthCommon& GT, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Document Metadata;
	Metadata.SetObject();

	// Schema version
	std::string schemaVersion = std::to_string(GT.VersionMajor) + "." + std::to_string(GT.VersionMinor);
	Metadata.AddMember("schema_version", rapidjson::Value(schemaVersion.c_str(), allocator), allocator);

	// Description
	Metadata.AddMember("description", rapidjson::Value(GT.Description.c_str(), allocator), allocator);

	// Environment
	Metadata.AddMember("environment", rapidjson::Value(GT.Environment.c_str(), allocator), allocator);

	// Random movements
	Metadata.AddMember("random_movements", GT.RandomMovements, allocator);

	// Car
	rapidjson::Value Car(rapidjson::kObjectType);
	Car.AddMember("model", rapidjson::Value(GT.CarModel.c_str(), allocator), allocator);
	Car.AddMember("speed", GT.CarSpeed, allocator);
	Metadata.AddMember("car", Car, allocator);

	// Sun
	rapidjson::Value Sun(rapidjson::kObjectType);
	Sun.AddMember("intensity", GT.SunLight.Intensity, allocator);
	Sun.AddMember("temperature", GT.SunLight.Temperature, allocator);

	rapidjson::Value SunRotation(rapidjson::kObjectType);
	SunRotation.AddMember("roll", GT.SunLight.Rotation.Roll, allocator);
	SunRotation.AddMember("pitch", GT.SunLight.Rotation.Pitch, allocator);
	SunRotation.AddMember("yaw", GT.SunLight.Rotation.Yaw, allocator);
	Sun.AddMember("rotation", SunRotation, allocator);

	Metadata.AddMember("sun", Sun, allocator);

	// Camera
	rapidjson::Value Camera(rapidjson::kObjectType);

	rapidjson::Value CameraPosition(rapidjson::kObjectType);
	CameraPosition.AddMember("x", GT.Camera.Position_inCar.X, allocator);
	CameraPosition.AddMember("y", GT.Camera.Position_inCar.Y, allocator);
	CameraPosition.AddMember("z", GT.Camera.Position_inCar.Z, allocator);
	Camera.AddMember("position", CameraPosition, allocator);

	rapidjson::Value CameraRotation(rapidjson::kObjectType);
	CameraRotation.AddMember("roll", GT.Camera.Rotation_inCar.Roll, allocator);
	CameraRotation.AddMember("pitch", GT.Camera.Rotation_inCar.Pitch, allocator);
	CameraRotation.AddMember("yaw", GT.Camera.Rotation_inCar.Yaw, allocator);
	Camera.AddMember("rotation", CameraRotation, allocator);

	Camera.AddMember("fov", GT.Camera.FOV, allocator);
	Camera.AddMember("aspect_ratio", GT.Camera.AspectRatio, allocator);
	Camera.AddMember("spectrum", rapidjson::Value(GT.Camera.NIR ? "NIR" : "RGB", allocator), allocator);

	rapidjson::Value Resolution(rapidjson::kArrayType);
	Resolution.PushBack(GT.Camera.FrameSize.X, allocator).PushBack(GT.Camera.FrameSize.Y, allocator);
	Camera.AddMember("resolution", Resolution, allocator);

	Camera.AddMember("frame_rate", GT.Camera.FrameRate, allocator);
	Camera.AddMember("mirrored", GT.Camera.Mirrored, allocator);
	Camera.AddMember("grain_intensity", GT.Camera.GrainIntensity, allocator);
	Camera.AddMember("grain_jitter", GT.Camera.GrainJitter, allocator);
	Camera.AddMember("saturation", GT.Camera.Saturation, allocator);
	Camera.AddMember("gamma", GT.Camera.Gamma, allocator);
	Camera.AddMember("contrast", GT.Camera.Contrast, allocator);
	Camera.AddMember("bloom_intensity", GT.Camera.BloomIntensity, allocator);
	Camera.AddMember("focus_offset", GT.Camera.FocusOffset, allocator);

	Metadata.AddMember("camera", Camera, allocator);

	// Camera Light
	rapidjson::Value CameraLight(rapidjson::kObjectType);
	CameraLight.AddMember("intensity", GT.CameraLight.Intensity, allocator);
	CameraLight.AddMember("attenuation_radius", GT.CameraLight.AttenuationRadius, allocator);
	CameraLight.AddMember("inner_cone_angle", GT.CameraLight.InnerConeAngle, allocator);
	CameraLight.AddMember("outer_cone_angle", GT.CameraLight.OuterConeAngle, allocator);

	rapidjson::Value CameraLightPosition(rapidjson::kObjectType);
	CameraLightPosition.AddMember("x", GT.CameraLight.Position.X, allocator);
	CameraLightPosition.AddMember("y", GT.CameraLight.Position.Y, allocator);
	CameraLightPosition.AddMember("z", GT.CameraLight.Position.Z, allocator);
	CameraLight.AddMember("position", CameraLightPosition, allocator);

	rapidjson::Value CameraLightRotation(rapidjson::kObjectType);
	CameraLightRotation.AddMember("roll", GT.CameraLight.Rotation.Roll, allocator);
	CameraLightRotation.AddMember("pitch", GT.CameraLight.Rotation.Pitch, allocator);
	CameraLightRotation.AddMember("yaw", GT.CameraLight.Rotation.Yaw, allocator);
	CameraLight.AddMember("rotation", CameraLightRotation, allocator);

	Metadata.AddMember("camera_light", CameraLight, allocator);

	// Blendout Defaults
	rapidjson::Value BlendoutDefaults(rapidjson::kObjectType);
	BlendoutDefaults.AddMember("common", GT.BlendoutDefaults.Common, allocator);
	BlendoutDefaults.AddMember("eye_gaze", GT.BlendoutDefaults.EyeGaze, allocator);
	BlendoutDefaults.AddMember("eyelids", GT.BlendoutDefaults.Eyelids, allocator);
	BlendoutDefaults.AddMember("face", GT.BlendoutDefaults.Face, allocator);
	BlendoutDefaults.AddMember("head", GT.BlendoutDefaults.Head, allocator);
	BlendoutDefaults.AddMember("upper_body", GT.BlendoutDefaults.UpperBody, allocator);
	BlendoutDefaults.AddMember("left_hand", GT.BlendoutDefaults.LeftHand, allocator);
	BlendoutDefaults.AddMember("right_hand", GT.BlendoutDefaults.RightHand, allocator);
	BlendoutDefaults.AddMember("steering_wheel", GT.BlendoutDefaults.SteeringWheel, allocator);

	Metadata.AddMember("blendout_defaults", BlendoutDefaults, allocator);

	// Car Base
	rapidjson::Value CarPosition_inWorld(rapidjson::kObjectType);
	CarPosition_inWorld.AddMember("x", GT.CarPosition_inWorld.X, allocator);
	CarPosition_inWorld.AddMember("y", GT.CarPosition_inWorld.Y, allocator);
	CarPosition_inWorld.AddMember("z", GT.CarPosition_inWorld.Z, allocator);

	Metadata.AddMember("car_location", CarPosition_inWorld, allocator);

	// Car Rotation
	rapidjson::Value CarRotation_inWorld(rapidjson::kObjectType);
	CarRotation_inWorld.AddMember("roll", GT.CarRotation_inWorld.Roll, allocator);
	CarRotation_inWorld.AddMember("pitch", GT.CarRotation_inWorld.Pitch, allocator);
	CarRotation_inWorld.AddMember("yaw", GT.CarRotation_inWorld.Yaw, allocator);

	Metadata.AddMember("car_rotation", CarRotation_inWorld, allocator);

	return Metadata;
}

static rapidjson::Value CreateJsonTextForOccupantOld(const int Type, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value Text(rapidjson::kArrayType);
	rapidjson::Value SeatPosition(rapidjson::kObjectType);
	SeatPosition.AddMember("name", rapidjson::Value("seat position", allocator), allocator);
	SeatPosition.AddMember("val", rapidjson::Value(GetSeatPositionString(Type).c_str(), allocator), allocator);
	Text.PushBack(SeatPosition, allocator);
	return Text;
}

static rapidjson::Value CreateJsonBBoxesOld(const DMSSimGroundTruthOccupant& PrevGtOccupant, const DMSSimGroundTruthOccupant& GtOccupant, const FString& Character, rapidjson::Document::AllocatorType& allocator) {
	// BBox
	rapidjson::Value BBox(rapidjson::kArrayType);

	// Face
	rapidjson::Value Face(rapidjson::kObjectType);
	Face.AddMember("name", rapidjson::Value("face", allocator), allocator);

	rapidjson::Value FaceVal(rapidjson::kArrayType);
	FaceVal.PushBack(PrevGtOccupant.FaceBoundingBox2D.Center.X, allocator)
		.PushBack(PrevGtOccupant.FaceBoundingBox2D.Center.Y, allocator)
		.PushBack(PrevGtOccupant.FaceBoundingBox2D.Width, allocator)
		.PushBack(PrevGtOccupant.FaceBoundingBox2D.Height, allocator);
	Face.AddMember("val", FaceVal, allocator);

	rapidjson::Value FaceAttributes(rapidjson::kObjectType);
	rapidjson::Value BooleanArray(rapidjson::kArrayType);

	rapidjson::Value Visible(rapidjson::kObjectType);
	Visible.AddMember("name", rapidjson::Value("visible", allocator), allocator);
	Visible.AddMember("val", GtOccupant.FaceBoundingBox2DVisible, allocator);

	BooleanArray.PushBack(Visible, allocator);

	FaceAttributes.AddMember("boolean", BooleanArray, allocator);
	Face.AddMember("attributes", FaceAttributes, allocator);

	BBox.PushBack(Face, allocator);

	// Eye Left
	rapidjson::Value EyeLeft(rapidjson::kObjectType);
	EyeLeft.AddMember("name", rapidjson::Value("eye left", allocator), allocator);

	rapidjson::Value EyeLeftVal(rapidjson::kArrayType);
	EyeLeftVal.PushBack(PrevGtOccupant.LeftEyeBoundingBox2D.Center.X, allocator)
		.PushBack(PrevGtOccupant.LeftEyeBoundingBox2D.Center.Y, allocator)
		.PushBack(PrevGtOccupant.LeftEyeBoundingBox2D.Width, allocator)
		.PushBack(PrevGtOccupant.LeftEyeBoundingBox2D.Height, allocator);
	EyeLeft.AddMember("val", EyeLeftVal, allocator);

	rapidjson::Value EyeLeftAttributes(rapidjson::kObjectType);
	rapidjson::Value EyeLeftBooleanArray(rapidjson::kArrayType);

	rapidjson::Value EyeLeftVisible(rapidjson::kObjectType);
	// TODO: Outdated -> Remove
	EyeLeftVisible.AddMember("name", rapidjson::Value("visible", allocator), allocator);
	EyeLeftVisible.AddMember("val", GtOccupant.LeftEyeBoundingBox2DVisible, allocator);

	rapidjson::Value EyeLeftOpen(rapidjson::kObjectType);
	EyeLeftOpen.AddMember("name", rapidjson::Value("open", allocator), allocator);
	EyeLeftOpen.AddMember("val", IsEyeOpened(PrevGtOccupant.LeftEyeOpening, MaxLeftEyeOpenings[Character], EYE_OPENING_THRESHOLD), allocator);

	EyeLeftBooleanArray.PushBack(EyeLeftVisible, allocator).PushBack(EyeLeftOpen, allocator);

	EyeLeftAttributes.AddMember("boolean", EyeLeftBooleanArray, allocator);

	rapidjson::Value EyeLeftFloatArray(rapidjson::kArrayType);

	rapidjson::Value EyeLeftOpening(rapidjson::kObjectType);
	EyeLeftOpening.AddMember("name", rapidjson::Value("opening", allocator), allocator);
	EyeLeftOpening.AddMember("val", PrevGtOccupant.LeftEyeOpening, allocator);

	rapidjson::Value EyeLidLeftVisibility(rapidjson::kObjectType);
	EyeLidLeftVisibility.AddMember("name", rapidjson::Value("eye_lid_visibility", allocator), allocator);
	EyeLidLeftVisibility.AddMember("val", GtOccupant.LeftEyeLidVisibilityPerc, allocator);

	rapidjson::Value EyePupilLeftVisibility(rapidjson::kObjectType);
	EyePupilLeftVisibility.AddMember("name", rapidjson::Value("eye_pupil_visibility", allocator), allocator);
	EyePupilLeftVisibility.AddMember("val", GtOccupant.LeftEyePupilVisibilityPerc, allocator);

	EyeLeftFloatArray.PushBack(EyeLeftOpening, allocator).PushBack(EyeLidLeftVisibility, allocator).PushBack(EyePupilLeftVisibility, allocator);

	EyeLeftAttributes.AddMember("float", EyeLeftFloatArray, allocator);

	EyeLeft.AddMember("attributes", EyeLeftAttributes, allocator);

	BBox.PushBack(EyeLeft, allocator);

	// Eye Right
	rapidjson::Value EyeRight(rapidjson::kObjectType);
	EyeRight.AddMember("name", rapidjson::Value("eye right", allocator), allocator);

	rapidjson::Value EyeRightVal(rapidjson::kArrayType);
	EyeRightVal.PushBack(PrevGtOccupant.RightEyeBoundingBox2D.Center.X, allocator)
		.PushBack(PrevGtOccupant.RightEyeBoundingBox2D.Center.Y, allocator)
		.PushBack(PrevGtOccupant.RightEyeBoundingBox2D.Width, allocator)
		.PushBack(PrevGtOccupant.RightEyeBoundingBox2D.Height, allocator);
	EyeRight.AddMember("val", EyeRightVal, allocator);

	rapidjson::Value EyeRightAttributes(rapidjson::kObjectType);
	rapidjson::Value EyeRightBooleanArray(rapidjson::kArrayType);

	rapidjson::Value EyeRightVisible(rapidjson::kObjectType);
	// TODO: Outdated -> Remove
	EyeRightVisible.AddMember("name", rapidjson::Value("visible", allocator), allocator);
	EyeRightVisible.AddMember("val", GtOccupant.RightEyeBoundingBox2DVisible, allocator);

	rapidjson::Value EyeRightOpen(rapidjson::kObjectType);
	EyeRightOpen.AddMember("name", rapidjson::Value("open", allocator), allocator);
	EyeRightOpen.AddMember("val", IsEyeOpened(PrevGtOccupant.RightEyeOpening, MaxRightEyeOpenings[Character], EYE_OPENING_THRESHOLD), allocator);

	EyeRightBooleanArray.PushBack(EyeRightVisible, allocator).PushBack(EyeRightOpen, allocator);

	EyeRightAttributes.AddMember("boolean", EyeRightBooleanArray, allocator);

	rapidjson::Value EyeRightFloatArray(rapidjson::kArrayType);

	rapidjson::Value EyeRightOpening(rapidjson::kObjectType);
	EyeRightOpening.AddMember("name", rapidjson::Value("opening", allocator), allocator);
	EyeRightOpening.AddMember("val", PrevGtOccupant.RightEyeOpening, allocator);

	rapidjson::Value EyeLidRightVisibility(rapidjson::kObjectType);
	EyeLidRightVisibility.AddMember("name", rapidjson::Value("eye_lid_visibility", allocator), allocator);
	EyeLidRightVisibility.AddMember("val", GtOccupant.RightEyeLidVisibilityPerc, allocator);

	rapidjson::Value EyePupilRightVisibility(rapidjson::kObjectType);
	EyePupilRightVisibility.AddMember("name", rapidjson::Value("eye_pupil_visibility", allocator), allocator);
	EyePupilRightVisibility.AddMember("val", GtOccupant.RightEyePupilVisibilityPerc, allocator);

	EyeRightFloatArray.PushBack(EyeRightOpening, allocator).PushBack(EyeLidRightVisibility, allocator).PushBack(EyePupilRightVisibility, allocator);

	EyeRightAttributes.AddMember("float", EyeRightFloatArray, allocator);

	EyeRight.AddMember("attributes", EyeRightAttributes, allocator);

	BBox.PushBack(EyeRight, allocator);

	return BBox;
}

static rapidjson::Value CreateJsonPoints2dOld(const DMSSimGroundTruthOccupant& PrevGtOccupant, const DMSSimGroundTruthOccupant& GtOccupant, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value Point2D(rapidjson::kArrayType);

	for (int k = 0; k < PrevGtOccupant.FacialLandmarks2D.Num(); k++) {
		rapidjson::Value Landmark(rapidjson::kObjectType);
		Landmark.AddMember("name", rapidjson::Value(("Landmark" + std::to_string(k)).c_str(), allocator), allocator);

		rapidjson::Value LandmarkVal(rapidjson::kArrayType);
		LandmarkVal.PushBack(static_cast<int>(PrevGtOccupant.FacialLandmarks2D[k].X + 0.5f), allocator).PushBack(static_cast<int>(PrevGtOccupant.FacialLandmarks2D[k].Y + 0.5f), allocator);
		Landmark.AddMember("val", LandmarkVal, allocator);

		rapidjson::Value LandmarkAttributes(rapidjson::kObjectType);

		rapidjson::Value LandmarkVisible(rapidjson::kObjectType);
		LandmarkVisible.AddMember("name", rapidjson::Value("visible", allocator), allocator);
		LandmarkVisible.AddMember("val", GtOccupant.FacialLandmarksVisible[k], allocator);

		rapidjson::Value BooleanArray(rapidjson::kArrayType);
		BooleanArray.PushBack(LandmarkVisible, allocator);
		LandmarkAttributes.AddMember("boolean", BooleanArray, allocator);

		Landmark.AddMember("attributes", LandmarkAttributes, allocator);

		Point2D.PushBack(Landmark, allocator);
	}

	for (int k = 0; k < PrevGtOccupant.LeftEyePupilLandmarks2D.Num(); k++) {
		rapidjson::Value PupilLandmark(rapidjson::kObjectType);
		PupilLandmark.AddMember("name", rapidjson::Value(("left eye pupil " + LandmarkPupilIrisNames[k]).c_str(), allocator), allocator);

		rapidjson::Value PupilVal(rapidjson::kArrayType);
		PupilVal.PushBack(static_cast<int>(PrevGtOccupant.LeftEyePupilLandmarks2D[k].X + 0.5f), allocator).PushBack(static_cast<int>(PrevGtOccupant.LeftEyePupilLandmarks2D[k].Y + 0.5f), allocator);
		PupilLandmark.AddMember("val", PupilVal, allocator);

		rapidjson::Value LandmarkAttributes(rapidjson::kObjectType);

		rapidjson::Value LandmarkVisible(rapidjson::kObjectType);
		LandmarkVisible.AddMember("name", rapidjson::Value("visible", allocator), allocator);
		// Left Eye Pupil: 25 <= x < 34
		LandmarkVisible.AddMember("val", GtOccupant.PupilIrisLandmarksVisible[k + 25], allocator);

		rapidjson::Value BooleanArray(rapidjson::kArrayType);
		BooleanArray.PushBack(LandmarkVisible, allocator);
		LandmarkAttributes.AddMember("boolean", BooleanArray, allocator);

		PupilLandmark.AddMember("attributes", LandmarkAttributes, allocator);

		Point2D.PushBack(PupilLandmark, allocator);
	}

	for (int k = 0; k < PrevGtOccupant.LeftEyeIrisLandmarks2D.Num(); k++) {
		rapidjson::Value IrisLandmark(rapidjson::kObjectType);
		IrisLandmark.AddMember("name", rapidjson::Value(("left eye iris " + LandmarkPupilIrisNames[k]).c_str(), allocator), allocator);

		rapidjson::Value IrisVal(rapidjson::kArrayType);
		IrisVal.PushBack(static_cast<int>(PrevGtOccupant.LeftEyeIrisLandmarks2D[k].X + 0.5f), allocator).PushBack(static_cast<int>(PrevGtOccupant.LeftEyeIrisLandmarks2D[k].Y + 0.5f), allocator);
		IrisLandmark.AddMember("val", IrisVal, allocator);

		rapidjson::Value LandmarkAttributes(rapidjson::kObjectType);

		rapidjson::Value LandmarkVisible(rapidjson::kObjectType);
		LandmarkVisible.AddMember("name", rapidjson::Value("visible", allocator), allocator);
		// Left Eye Iris: 17 <= x < 25
		LandmarkVisible.AddMember("val", GtOccupant.PupilIrisLandmarksVisible[k + 17], allocator);

		rapidjson::Value BooleanArray(rapidjson::kArrayType);
		BooleanArray.PushBack(LandmarkVisible, allocator);
		LandmarkAttributes.AddMember("boolean", BooleanArray, allocator);

		IrisLandmark.AddMember("attributes", LandmarkAttributes, allocator);

		Point2D.PushBack(IrisLandmark, allocator);
	}

	for (int k = 0; k < PrevGtOccupant.RightEyePupilLandmarks2D.Num(); k++) {
		rapidjson::Value PupilLandmark(rapidjson::kObjectType);
		PupilLandmark.AddMember("name", rapidjson::Value(("right eye pupil " + LandmarkPupilIrisNames[k]).c_str(), allocator), allocator);

		rapidjson::Value PupilVal(rapidjson::kArrayType);
		PupilVal.PushBack(static_cast<int>(PrevGtOccupant.RightEyePupilLandmarks2D[k].X + 0.5f), allocator).PushBack(static_cast<int>(PrevGtOccupant.RightEyePupilLandmarks2D[k].Y + 0.5f), allocator);
		PupilLandmark.AddMember("val", PupilVal, allocator);

		rapidjson::Value LandmarkAttributes(rapidjson::kObjectType);

		rapidjson::Value LandmarkVisible(rapidjson::kObjectType);
		LandmarkVisible.AddMember("name", rapidjson::Value("visible", allocator), allocator);
		// Right Eye Pupil: 8 <= x < 17
		LandmarkVisible.AddMember("val", GtOccupant.PupilIrisLandmarksVisible[k + 8], allocator);

		rapidjson::Value BooleanArray(rapidjson::kArrayType);
		BooleanArray.PushBack(LandmarkVisible, allocator);
		LandmarkAttributes.AddMember("boolean", BooleanArray, allocator);

		PupilLandmark.AddMember("attributes", LandmarkAttributes, allocator);

		Point2D.PushBack(PupilLandmark, allocator);
	}

	for (int k = 0; k < PrevGtOccupant.RightEyeIrisLandmarks2D.Num(); k++) {
		rapidjson::Value IrisLandmark(rapidjson::kObjectType);
		IrisLandmark.AddMember("name", rapidjson::Value(("right eye iris " + LandmarkPupilIrisNames[k]).c_str(), allocator), allocator);

		rapidjson::Value IrisVal(rapidjson::kArrayType);
		IrisVal.PushBack(static_cast<int>(PrevGtOccupant.RightEyeIrisLandmarks2D[k].X + 0.5f), allocator).PushBack(static_cast<int>(PrevGtOccupant.RightEyeIrisLandmarks2D[k].Y + 0.5f), allocator);
		IrisLandmark.AddMember("val", IrisVal, allocator);
		rapidjson::Value LandmarkAttributes(rapidjson::kObjectType);
		rapidjson::Value BooleanArray(rapidjson::kArrayType);

		rapidjson::Value LandmarkVisible(rapidjson::kObjectType);
		LandmarkVisible.AddMember("name", rapidjson::Value("visible", allocator), allocator);
		// Right Eye Iris: 0 <= x < 8
		LandmarkVisible.AddMember("val", GtOccupant.PupilIrisLandmarksVisible[k + 0], allocator);

		BooleanArray.PushBack(LandmarkVisible, allocator);
		LandmarkAttributes.AddMember("boolean", BooleanArray, allocator);

		IrisLandmark.AddMember("attributes", LandmarkAttributes, allocator);
		Point2D.PushBack(IrisLandmark, allocator);
	}

	return Point2D;
}

static rapidjson::Value CreateJsonPoints3dOld(const DMSSimGroundTruthOccupant& PrevGtOccupant, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value Point3D(rapidjson::kArrayType);

	// Left Eye Gaze Origin
	rapidjson::Value LeftGazeOrigin(rapidjson::kObjectType);
	LeftGazeOrigin.AddMember("name", rapidjson::Value("left eye gaze origin", allocator), allocator);
	rapidjson::Value LeftGazeOriginVal(rapidjson::kArrayType);
	LeftGazeOriginVal.PushBack(PrevGtOccupant.LeftGazeOrigin_inCam.X, allocator).PushBack(PrevGtOccupant.LeftGazeOrigin_inCam.Y, allocator).PushBack(PrevGtOccupant.LeftGazeOrigin_inCam.Z, allocator);
	LeftGazeOrigin.AddMember("val", LeftGazeOriginVal, allocator);
	Point3D.PushBack(LeftGazeOrigin, allocator);

	// Left Eye Gaze Direction
	rapidjson::Value LeftGazeDirection(rapidjson::kObjectType);
	LeftGazeDirection.AddMember("name", rapidjson::Value("left eye gaze direction", allocator), allocator);

	rapidjson::Value LeftGazeDirectionVal(rapidjson::kArrayType);
	LeftGazeDirectionVal.PushBack(PrevGtOccupant.LeftGazeDirection_inCam.X, allocator).PushBack(PrevGtOccupant.LeftGazeDirection_inCam.Y, allocator).PushBack(PrevGtOccupant.LeftGazeDirection_inCam.Z, allocator);
	LeftGazeDirection.AddMember("val", LeftGazeDirectionVal, allocator);
	Point3D.PushBack(LeftGazeDirection, allocator);

	// Right Eye Gaze Origin
	rapidjson::Value RightGazeOrigin(rapidjson::kObjectType);
	RightGazeOrigin.AddMember("name", rapidjson::Value("right eye gaze origin", allocator), allocator);
	rapidjson::Value RightGazeOriginVal(rapidjson::kArrayType);
	RightGazeOriginVal.PushBack(PrevGtOccupant.RightGazeOrigin_inCam.X, allocator).PushBack(PrevGtOccupant.RightGazeOrigin_inCam.Y, allocator).PushBack(PrevGtOccupant.RightGazeOrigin_inCam.Z, allocator);
	RightGazeOrigin.AddMember("val", RightGazeOriginVal, allocator);
	Point3D.PushBack(RightGazeOrigin, allocator);

	// Right Eye Gaze Direction
	rapidjson::Value RightGazeDirection(rapidjson::kObjectType);
	RightGazeDirection.AddMember("name", rapidjson::Value("right eye gaze direction", allocator), allocator);
	rapidjson::Value RightGazeDirectionVal(rapidjson::kArrayType);
	RightGazeDirectionVal.PushBack(PrevGtOccupant.RightGazeDirection_inCam.X, allocator).PushBack(PrevGtOccupant.RightGazeDirection_inCam.Y, allocator).PushBack(PrevGtOccupant.RightGazeDirection_inCam.Z, allocator);
	RightGazeDirection.AddMember("val", RightGazeDirectionVal, allocator);
	Point3D.PushBack(RightGazeDirection, allocator);

	// Gaze Origin
	rapidjson::Value GazeOrigin(rapidjson::kObjectType);
	GazeOrigin.AddMember("name", rapidjson::Value("gaze origin", allocator), allocator);
	rapidjson::Value GazeOriginVal(rapidjson::kArrayType);
	GazeOriginVal.PushBack(PrevGtOccupant.GazeOrigin_inCam.X, allocator).PushBack(PrevGtOccupant.GazeOrigin_inCam.Y, allocator).PushBack(PrevGtOccupant.GazeOrigin_inCam.Z, allocator);
	GazeOrigin.AddMember("val", GazeOriginVal, allocator);
	Point3D.PushBack(GazeOrigin, allocator);

	// Gaze Direction
	rapidjson::Value GazeDirection(rapidjson::kObjectType);
	GazeDirection.AddMember("name", rapidjson::Value("gaze direction", allocator), allocator);
	rapidjson::Value GazeDirectionVal(rapidjson::kArrayType);
	GazeDirectionVal.PushBack(PrevGtOccupant.GazeDirection_inCam.X, allocator).PushBack(PrevGtOccupant.GazeDirection_inCam.Y, allocator).PushBack(PrevGtOccupant.GazeDirection_inCam.Z, allocator);
	GazeDirection.AddMember("val", GazeDirectionVal, allocator);
	Point3D.PushBack(GazeDirection, allocator);

	// head origin	
	rapidjson::Value HeadOrigin(rapidjson::kObjectType);
	HeadOrigin.AddMember("name", rapidjson::Value("head origin", allocator), allocator);
	rapidjson::Value HeadOriginVal(rapidjson::kArrayType);
	HeadOriginVal.PushBack(PrevGtOccupant.HeadOriginEarsCenter_inCam.X, allocator).PushBack(PrevGtOccupant.HeadOriginEarsCenter_inCam.Y, allocator).PushBack(PrevGtOccupant.HeadOriginEarsCenter_inCam.Z, allocator);
	HeadOrigin.AddMember("val", HeadOriginVal, allocator);
	Point3D.PushBack(HeadOrigin, allocator);

	// head direction
	rapidjson::Value HeadDirection(rapidjson::kObjectType);
	HeadDirection.AddMember("name", rapidjson::Value("head direction", allocator), allocator);
	rapidjson::Value HeadDirectionVal(rapidjson::kArrayType);
	HeadDirectionVal.PushBack(PrevGtOccupant.HeadDirection_inCam.X, allocator).PushBack(PrevGtOccupant.HeadDirection_inCam.Y, allocator).PushBack(PrevGtOccupant.HeadDirection_inCam.Z, allocator);
	HeadDirection.AddMember("val", HeadDirectionVal, allocator);
	Point3D.PushBack(HeadDirection, allocator);

	// head rotation
	rapidjson::Value HeadRotation(rapidjson::kObjectType);
	HeadRotation.AddMember("name", rapidjson::Value("head rotation roll, pitch, yaw", allocator), allocator);
	rapidjson::Value HeadRotationVal(rapidjson::kArrayType);
	HeadRotationVal.PushBack(PrevGtOccupant.HeadRotation_inCam.Roll, allocator).PushBack(PrevGtOccupant.HeadRotation_inCam.Pitch, allocator).PushBack(PrevGtOccupant.HeadRotation_inCam.Yaw, allocator);
	HeadRotation.AddMember("val", HeadRotationVal, allocator);
	Point3D.PushBack(HeadRotation, allocator);

	return Point3D;
}

static rapidjson::Value CreateJsonBooleanAttrsForOccupantOld(const FDMSSimOccupant& PrevFDMSSimOccupant, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value BooleanAttrs(rapidjson::kArrayType);

	rapidjson::Value GlassesAttr(rapidjson::kObjectType);
	GlassesAttr.AddMember("name", rapidjson::Value("glasses", allocator), allocator);
	GlassesAttr.AddMember("val", PrevFDMSSimOccupant.Glasses.Model > 0, allocator);
	BooleanAttrs.PushBack(GlassesAttr, allocator);

	rapidjson::Value HeadgearAttr(rapidjson::kObjectType);
	HeadgearAttr.AddMember("name", rapidjson::Value("headgear", allocator), allocator);
	HeadgearAttr.AddMember("val", PrevFDMSSimOccupant.Headgear > 0, allocator);
	BooleanAttrs.PushBack(HeadgearAttr, allocator);

	rapidjson::Value ScarfAttr(rapidjson::kObjectType);
	ScarfAttr.AddMember("name", rapidjson::Value("scarf", allocator), allocator);
	ScarfAttr.AddMember("val", PrevFDMSSimOccupant.Scarf > 0, allocator);
	BooleanAttrs.PushBack(ScarfAttr, allocator);

	rapidjson::Value MaskAttr(rapidjson::kObjectType);
	MaskAttr.AddMember("name", rapidjson::Value("mask", allocator), allocator);
	MaskAttr.AddMember("val", PrevFDMSSimOccupant.Mask > 0, allocator);
	BooleanAttrs.PushBack(MaskAttr, allocator);

	return BooleanAttrs;
}

static rapidjson::Value CreateJsonEyeAndSkinAttrsForOccupantOld(const FDMSSimOccupant& PrevFDMSSimOccupant, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value EyeAndSkinAttrs(rapidjson::kArrayType);

#define ADD_ATTR(name, val) \
		{ \
			rapidjson::Value attr(rapidjson::kObjectType); \
			attr.AddMember("name", rapidjson::Value(name, allocator).Move(), allocator); \
			attr.AddMember("val", val, allocator); \
			EyeAndSkinAttrs.PushBack(attr, allocator); \
		}

	ADD_ATTR("pupil_size", PrevFDMSSimOccupant.PupilSize);
	ADD_ATTR("pupil_brightness", PrevFDMSSimOccupant.PupilBrightness);
	ADD_ATTR("iris_size", PrevFDMSSimOccupant.IrisSize);
	ADD_ATTR("iris_brightness", PrevFDMSSimOccupant.IrisBrightness);
	ADD_ATTR("limbus_dark_amount", PrevFDMSSimOccupant.LimbusDarkAmount);
	ADD_ATTR("iris_color", rapidjson::Value(TCHAR_TO_UTF8(*PrevFDMSSimOccupant.IrisColor), allocator).Move());
	ADD_ATTR("sclera_brightness", PrevFDMSSimOccupant.ScleraBrightness);
	ADD_ATTR("sclera_veins", PrevFDMSSimOccupant.ScleraVeins);
	ADD_ATTR("skin_wrinkles", PrevFDMSSimOccupant.SkinWrinkles);
	ADD_ATTR("skin_roughness", PrevFDMSSimOccupant.SkinRoughness);
	ADD_ATTR("skin_specularity", PrevFDMSSimOccupant.SkinSpecularity);

#undef ADD_ATTR		

	return EyeAndSkinAttrs;
}

static rapidjson::Value CreateJsonOccupantOld(const TSharedPtr<DMSSimGroundTruthFrame>& PrevGroundTruth, const TSharedPtr<DMSSimGroundTruthFrame>& GroundTruth, const int i, rapidjson::Document::AllocatorType& allocator) {
	const uint8 Type = static_cast<uint8>(PrevGroundTruth->Common.Occupants[i].Type);
	const auto& PrevGtOccupant = PrevGroundTruth.Get()->Occupants[Type];
	const auto& GtOccupant = GroundTruth.Get()->Occupants[Type];
	const auto& PrevFDMSSimOccupant = PrevGroundTruth->Common.Occupants[i];

	rapidjson::Value Occupant(rapidjson::kObjectType);
	Occupant.AddMember("name", rapidjson::Value(TCHAR_TO_UTF8(*PrevFDMSSimOccupant.Character), allocator), allocator);
	Occupant.AddMember("type", rapidjson::Value("Metahuman", allocator), allocator);

	rapidjson::Value ObjectData(rapidjson::kObjectType);

	auto Text = CreateJsonTextForOccupantOld(Type, allocator);
	ObjectData.AddMember("text", Text, allocator);

	auto BBox = CreateJsonBBoxesOld(PrevGtOccupant, GtOccupant, PrevFDMSSimOccupant.Character, allocator);
	ObjectData.AddMember("bbox", BBox, allocator);

	auto Point2D = CreateJsonPoints2dOld(PrevGtOccupant, GtOccupant, allocator);
	ObjectData.AddMember("point2d", Point2D, allocator);

	auto Point3D = CreateJsonPoints3dOld(PrevGtOccupant, allocator);
	ObjectData.AddMember("point3d", Point3D, allocator);

	auto EyeAndSkinAttrs = CreateJsonEyeAndSkinAttrsForOccupantOld(PrevFDMSSimOccupant, allocator);
	ObjectData.AddMember("eye_and_skin", EyeAndSkinAttrs, allocator);

	auto BooleanAttrs = CreateJsonBooleanAttrsForOccupantOld(PrevFDMSSimOccupant, allocator);
	ObjectData.AddMember("boolean", BooleanAttrs, allocator);

	Occupant.AddMember("object_data", ObjectData, allocator);

	return Occupant;
}


void DMSSimImageLabelerOldImpl::AddFrame(const TSharedPtr<DMSSimGroundTruthFrame>& PrevGroundTruth, const TSharedPtr<DMSSimGroundTruthFrame>& GroundTruth, int FrameIdx) {
	std::wstringstream ss;
	ss << BaseFileName_ << L"_" << std::setw(5) << std::setfill(L'0') << FrameIdx << L"_old" << L".json";
	// Serialize JSON to string
	rapidjson::StringBuffer buffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
	CreateLabelFileOld(PrevGroundTruth, GroundTruth).Accept(writer);
	std::ofstream JsonFile(ss.str());
	if (JsonFile.is_open()) {
		JsonFile << buffer.GetString();
		JsonFile.close();
	}
	else { DMSSimLog::Info() << "fail at creating json file" << FL; }
}

rapidjson::Document DMSSimImageLabelerOldImpl::CreateLabelFileOld(const TSharedPtr<DMSSimGroundTruthFrame>& PrevGroundTruth, const TSharedPtr<DMSSimGroundTruthFrame>& GroundTruth) {
	rapidjson::Document Root;
	Root.SetObject();
	rapidjson::Document::AllocatorType& allocator = Root.GetAllocator();

	// Assign metadata to openlabel
	Root.AddMember("openlabel", rapidjson::Value(rapidjson::kObjectType).AddMember("metadata", CreateJsonMetadataOld(PrevGroundTruth->Common, allocator), allocator), allocator);

	rapidjson::Value Objects(rapidjson::kObjectType);
	for (int i = 0; i < PrevGroundTruth->Common.OccupantCount; i++) {
		auto Occupant = CreateJsonOccupantOld(PrevGroundTruth, GroundTruth, i, allocator);
		Objects.AddMember(rapidjson::Value(std::to_string(i).c_str(), allocator), Occupant, allocator);
	}
	Root["openlabel"].AddMember("objects", Objects, allocator);
	return Root;
}