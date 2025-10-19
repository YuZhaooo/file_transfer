#pragma once

#include "DMSSimConfigParser.h"
#include "DMSSimOccupantType.h"

constexpr unsigned DMSSIM_DEFAULT_FRAME_WIDTH = 1312;
constexpr unsigned DMSSIM_DEFAULT_FRAME_HEIGHT = 1008;
constexpr int      DMSSIM_MAX_RESOLUTION_DIMENSION = 8192;

constexpr float DMSSIM_DEFAULT_FOV = 43.6f;
constexpr float DMSSIM_MIN_FOV = 5.0f;
constexpr float DMSSIM_MAX_FOV = 165.0f;
constexpr int   DMSSIM_DEFAULT_FRAME_RATE = 60;
constexpr int   DMSSIM_MAX_FRAME_RATE = 120;

constexpr float DMSSIM_MIN_NOISE = 0.0f;
constexpr float DMSSIM_MAX_NOISE = 100.0f;
constexpr float DMSSIM_DEFAULT_NOISE = 0.0f;

constexpr float DMSSIM_MIN_BLUR = 0.0f;
constexpr float DMSSIM_MAX_BLUR = 100.0f;
constexpr float DMSSIM_DEFAULT_BLUR = 0.0f;

constexpr float DMSSIM_MIN_FSTOP = 1.0f;
constexpr float DMSSIM_MAX_FSTOP = 100.0f;

constexpr float DMSSIM_MIN_FOCAL_DISTANCE = 0.0f;
constexpr float DMSSIM_MAX_FOCAL_DISTANCE = 10.0f;

constexpr int DMSSIM_MIN_DIAPHRAGM_BLADE_COUNT = 1;
constexpr int DMSSIM_MAX_DIAPHRAGM_BLADE_COUNT = 20;
constexpr int DMSSIM_DEFAULT_DIAPHRAGM_BLADE_COUNT = 5;

constexpr float DMSSIM_MIN_ANIMATION_DURATION = 0.001f;
constexpr float DMSSIM_MAX_ANIMATION_DURATION = 10000.0f;

constexpr float DMSSIM_MIN_BLEND_OUT = 0.0f;
constexpr float DMSSIM_MAX_BLEND_OUT = 20.0f;

constexpr float DMSSIM_MIN_CURVE_VALUE = -1000.0f;
constexpr float DMSSIM_MAX_CURVE_VALUE = 1000.0f;

constexpr float DMSSIM_MIN_ILLUMINATION_INTENSITY = 0.0f;
constexpr float DMSSIM_MAX_ILLUMINATION_INTENSITY = 100000.0f;

constexpr float DMSSIM_MIN_ILLUMINATION_TEMPERATURE = 0.0f;
constexpr float DMSSIM_MAX_ILLUMINATION_TEMPERATURE = 12000.0f;

constexpr float DMSSIM_MIN_ILLUMINATION_OUTER_CONE_ANGLE = 0.0f;
constexpr float DMSSIM_MAX_ILLUMINATION_OUTER_CONE_ANGLE = 80.0f;

constexpr float DMSSIM_MIN_ILLUMINATION_ATTENUATION_RADIUS = 0.0f;
constexpr float DMSSIM_MAX_ILLUMINATION_ATTENUATION_RADIUS = 10000.0f;

constexpr float DMSSIM_MIN_STEERING_WHEEL_COLUMN_PITCH_ANGLE = -20.0f;
constexpr float DMSSIM_MAX_STEERING_WHEEL_COLUMN_PITCH_ANGLE = 20.0f;

constexpr float DMSSIM_MIN_GLASSES_OPACITY = 0.0f;
constexpr float DMSSIM_MAX_GLASSES_OPACITY = 1.0f;
constexpr float DMSSIM_DEFAULT_GLASSES_OPACITY = 0.2f;


constexpr char DMSSIM_TOKEN_NONE[] = "none";

constexpr char DMSSIM_PARAMETER_GAZE_DIRECTION_POINT[] = "gaze_direction_point";
constexpr char DMSSIM_PARAMETER_HEAD_DIRECTION_POINT[] = "head_direction_point";
constexpr char DMSSIM_PARAMETER_EYE_OPENING_REL_VECTOR[] = "eye_opening_relative_vector";
constexpr char DMSSIM_PARAMETER_STEERING_WHEEL[] = "steering_wheel";

constexpr char DMSSIM_HEAD_ANIMATION_POSTFIX[] = "_0";

enum DMSSimAnimationChannelType {
	DMSSimAnimationChannelCommon,
	DMSSimAnimationChannelEyeGaze,
	DMSSimAnimationChannelEyelids,
	DMSSimAnimationChannelFace,
	DMSSimAnimationChannelHead,
	DMSSimAnimationChannelUpperBody,
	DMSSimAnimationChannelLeftHand,
	DMSSimAnimationChannelRightHand,
	DMSSimAnimationChannelSteeringWheel,
	DMSSimAnimationChannelCount,
};

enum DMSSimBaseAnimationType {
	DMSSimBaseAnimationUnknown,
	DMSSimBaseAnimationHead,
	DMSSimBaseAnimationBody,
};

enum DMSSimMotionType {
	DMSSimMotionAnimation,
	DMSSimMotionPauseActive,
	DMSSimMotionPausePassive,
	DMSSimMotionParametric,
};

enum DMSSimAnimationType {
	DMSSimAnimationUnknown,
	DMSSimAnimationParametricGazeDirection,
	DMSSimAnimationParametricHeadRotation,
	DMSSimAnimationParametricEyeOpening,
	DMSSimAnimationParametricSteeringWheel,
};

class DMSSimOccupant {
protected:
	virtual ~DMSSimOccupant(){}
public:
	virtual FDMSSimOccupantType GetType() const = 0;
	virtual const char* GetCharacter() const = 0;
	virtual const char* GetHeadgear() const = 0;
	virtual const char* GetGlasses() const = 0;
	virtual const char* GetUpperCloth() const = 0;
	virtual FVector     GetGlassesColor() const = 0;
	virtual float       GetGlassesOpacity() const = 0;
	virtual bool       GetGlassesReflective() const = 0;
	virtual const char* GetMask() const = 0;
	virtual const char* GetScarf() const = 0;
	virtual const char* GetHair() const = 0;
	virtual const char* GetBeard() const = 0;
	virtual const char* GetMustache() const = 0;
	virtual float GetPupilSize() const = 0;
	virtual float GetPupilBrightness() const = 0;
	virtual float GetIrisSize() const = 0;
	virtual float GetIrisBrightness() const = 0;
	virtual float GetIrisBorderWidth() const = 0;
	virtual float GetLimbusDarkAmount() const = 0;
	virtual const char* GetIrisColor() const = 0;
	virtual float GetScleraBrightness() const = 0;
	virtual float GetScleraVeins() const = 0;
	virtual float GetSkinWrinkles() const = 0;
	virtual float GetSkinRoughness() const = 0;
	virtual float GetSkinSpecularity() const = 0;
	virtual float GetHeight() const = 0;
	virtual FVector GetSeatOffset() const = 0;
};

class DMSSimAnimationPoint {
protected:
	virtual ~DMSSimAnimationPoint(){}
public:
	virtual float GetTime() const = 0;
	virtual FVector GetPoint() const = 0;
};

class DMSSimMotion {
protected:
	virtual ~DMSSimMotion(){}
public:
	virtual DMSSimMotionType GetType() const = 0;
	virtual const char* GetAnimationName() const = 0;
	virtual size_t GetPointCount() const = 0;
	virtual const DMSSimAnimationPoint& GetPoint(size_t Index) const = 0;

	virtual float GetStartPos() const = 0;
	virtual float GetEndPos() const = 0;
	virtual float GetDuration() const = 0;
	virtual float GetBlendOut() const = 0;
};

class DMSSimAnimationChannel {
protected:
	virtual ~DMSSimAnimationChannel() {}
public:
	virtual DMSSimAnimationChannelType GetType() const = 0;
	virtual size_t GetMotionCount() const = 0;
	virtual const DMSSimMotion& GetMotion(size_t MotionIndex) const = 0;
};

class DMSSimAnimationSequence {
protected:
	virtual ~DMSSimAnimationSequence(){}
public:
	virtual const char* GetName() const = 0;
	virtual DMSSimAnimationType GetType() const = 0;
	virtual size_t GetMotionCount() const = 0;
	virtual const DMSSimMotion& GetMotion(size_t MotionIndex) const = 0;
};

class DMSSimOccupantScenario {
protected:
	virtual ~DMSSimOccupantScenario(){}
public:
	virtual FDMSSimOccupantType GetType() const = 0;
	virtual size_t GetMotionCount() const = 0;
	virtual const DMSSimMotion& GetMotion(size_t MotionIndex) const = 0;
	virtual size_t GetChannelCount() const = 0;
	virtual const DMSSimAnimationChannel& GetChannel(size_t ChannelIndex) const = 0;
};

class DMSSimIllumination {
protected:
	virtual ~DMSSimIllumination() {}
public:
	virtual float GetIntensity() const = 0;
	virtual float GetAttenuationRadius() const = 0;
	virtual float GetSourceRadius() const = 0;
	virtual float InnerConeAngle() const = 0;
	virtual float OuterConeAngle() const = 0;
	virtual FVector  GetPosition() const = 0;
	virtual FRotator GetRotation() const = 0;
};

class DMSSimSteeringWheelColumn {
protected:
	virtual ~DMSSimSteeringWheelColumn() {}
public:
	virtual float GetPitchAngle() const = 0;
	virtual bool GetIsCameraIntegrated() const = 0;
};

class DMSSimGroundTruthSettings {
protected:
	virtual ~DMSSimGroundTruthSettings() {}
public:
	virtual float					GetBoundingBoxPaddingFactorFace() const = 0;
	virtual float					GetEyeBoundingBoxWidthFactor() const = 0;
	virtual float					GetEyeBoundingBoxHeightFactor() const = 0;
	virtual float					GetEyeBoundingBoxDepth() const = 0;
};


class DMSSimCamera {
protected:
	virtual ~DMSSimCamera(){}
public:
	virtual bool					GetNIR() const = 0;
	virtual bool                    GetVideoOut() const = 0;
	virtual bool                    GetCsvOut() const = 0;
	virtual bool					GetDepth16Bit() const = 0;
	virtual unsigned				GetFrameWidth() const = 0;
	virtual unsigned				GetFrameHeight() const = 0;
	virtual unsigned				GetFrameRate() const = 0;

	virtual FVector					GetPosition() const = 0;
	virtual FRotator				GetRotation() const = 0;
	virtual bool					GetMirrored() const = 0;
	virtual float					GetFOV() const = 0;

	virtual float					GetNoise() const = 0;
	virtual float					GetBlur() const = 0;

	virtual float					GetFocalDistance() const = 0;
	virtual int						GetDiaphragmBladeCount() const = 0;
	virtual float					GetMinFStop() const = 0;
	virtual float					GetMaxFStop() const = 0;

	virtual float					GetGrainIntensity() const = 0;
	virtual float					GetGrainJitter() const = 0;
	virtual float					GetSaturation() const = 0;
	virtual float					GetGamma() const = 0;
	virtual float					GetContrast() const = 0;
	virtual float					GetBloomIntensity() const = 0;
	virtual float					GetFocusOffset() const = 0;
};

/**
 * @class DMSSimScenarioParser
 * @brief DMSSimScenarioParser is a tree-like representation of a scenario yaml file.
 * The DMSSimScenarioParser object can provide singular properties,
 * as well as references to complex subobjects, such as DMSSimCamera, for example.
 */
class DMSSimScenarioParser{
public:
	virtual ~DMSSimScenarioParser(){}

	virtual unsigned GetVersionMajor() const = 0;
	virtual unsigned GetVersionMinor() const = 0;
	virtual const char* GetDescription() const = 0;
	virtual const char* GetEnvironment() const = 0;
	virtual bool GetRandomBlinking() const = 0;
	virtual bool GetRandomSmiling() const = 0;
	virtual bool GetRandomHeadMovements() const = 0;
	virtual bool GetRandomBodyMovements() const = 0;
	virtual bool GetRandomGaze() const = 0;
	virtual const char* GetCarModel() const = 0;
	virtual float GetCarSpeed() const = 0;
	virtual FRotator GetSunRotation() const = 0;
	virtual float GetSunIntensity() const = 0;
	virtual float GetSunTemperature() const = 0;

	virtual const DMSSimCoordinateSpace& GetCoordinateSpace() const = 0;

	virtual const DMSSimCamera& GetCamera() const = 0;
	virtual const DMSSimSteeringWheelColumn& GetSteeringWheelColumn() const = 0;
	virtual const DMSSimGroundTruthSettings& GetGroundTruthSettings() const = 0;

	virtual const DMSSimIllumination& GetCameraIllumination() const = 0;


	virtual float GetDefaultBlendOut(DMSSimAnimationChannelType Channel) const = 0;

	virtual size_t GetOccupantCount() const = 0;
	virtual const DMSSimOccupant& GetOccupant(size_t OccupantIndex) const = 0;

	virtual size_t GetAnimationSequenceCount() const = 0;
	virtual const DMSSimAnimationSequence& GetAnimationSequence(size_t Index) const = 0;

	virtual size_t GetOccupantScenarioCount() const = 0;
	virtual const DMSSimOccupantScenario& GetOccupantScenario(size_t Index) const = 0;

	/**
	 * Create Scenario Parser Object
	 *
	 * @param[in] FilePath Path to a scenario file
	 * @param[in] Config   Coordinate space configurations for different car models
	 *
	 * @return Parser object
	 */
	static DMSSimScenarioParser* Create(const wchar_t* const FilePath, const DMSSimConfigParser& Config);

	static const DMSSimCoordinateSpace& GetDefaultCoordinateSpace();
	static const DMSSimCamera& GetDefaultCamera();
	static DMSSimBaseAnimationType GetChannelBaseAnimationType(DMSSimAnimationChannelType Channel);
};
