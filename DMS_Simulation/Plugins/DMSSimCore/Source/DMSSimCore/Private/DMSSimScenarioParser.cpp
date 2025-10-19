#include "DMSSimScenarioParser.h"
#include "DMSSimConstants.h"
#include "DMSSimParserBase.h"
#include "DMSSimYamlObj.h"
#include "DMSSimLog.h"
#include <cassert>
#include <fstream>
#include <functional>
#include <regex>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

using namespace DMSSimParserBaseHelpers;

namespace {

	constexpr char DMSSIM_TOKEN_DEFAULT[] = "default";
	constexpr char DMSSIM_CHANNEL_NAME_COMMON[] = "common";
	constexpr char DMSSIM_CHANNEL_NAME_EYE_GAZE[] = "eye_gaze";
	constexpr char DMSSIM_CHANNEL_NAME_EYELIDS[] = "eyelids";
	constexpr char DMSSIM_CHANNEL_NAME_FACE[] = "face";
	constexpr char DMSSIM_CHANNEL_NAME_HEAD[] = "head";
	constexpr char DMSSIM_CHANNEL_NAME_UPPER_BODY[] = "upper_body";
	constexpr char DMSSIM_CHANNEL_NAME_LEFT_HAND[] = "left_hand";
	constexpr char DMSSIM_CHANNEL_NAME_RIGHT_HAND[] = "right_hand";
	constexpr char DMSSIM_CHANNEL_NAME_STEERING_WHEEL[] = "steering_wheel";

	bool IsCurveInCarSpace(const char* const MotionName) {
		const char* const CameraSpaceParameters[] = {
			DMSSIM_PARAMETER_GAZE_DIRECTION_POINT,
			DMSSIM_PARAMETER_HEAD_DIRECTION_POINT,
		};

		for (const auto parameter : CameraSpaceParameters) { if (strcmp(parameter, MotionName) == 0) { return true; } }
		return false;
	}

	FVector ConvertPointToCarSpace(const DMSSimCoordinateSpace& CoordinateSpace, const FVector& Point) {
		FVector NewPoint = {};
		const FRotator Rotation = CoordinateSpace.GetRotation();
		const FVector Translation = CoordinateSpace.GetTranslation();
		const FVector Scale = CoordinateSpace.GetScale();
		NewPoint = Point + Translation;
		NewPoint = Rotation.RotateVector(NewPoint);
		NewPoint = NewPoint * Scale;
		NewPoint = NewPoint * DMSSIM_M_TO_CM;
		return NewPoint;
	}

	FVector ConvertVectorToCarSpace(const DMSSimCoordinateSpace& CoordinateSpace, const FVector& Point) {
		FVector NewPoint(Point);
		const FRotator Rotation = CoordinateSpace.GetRotation();
		const FVector Scale = CoordinateSpace.GetScale();
		NewPoint = Rotation.RotateVector(NewPoint);
		NewPoint = NewPoint * Scale;
		return NewPoint;
	}

	FRotator ConvertRotationToCarSpace(const DMSSimCoordinateSpace& CoordinateSpace, const FRotator& Rotator){
		FRotator RotatorAdj = Rotator * static_cast<float>(180.0f / PI);
		FVector Forward{ 1.0f, 0.0f, 0.0f };
		FVector Right{ 0.0f, -1.0f, 0.0f };
		FVector Up{ 0.0f, 0.0f, 1.0f };
		Forward = ConvertVectorToCarSpace(CoordinateSpace, RotatorAdj.RotateVector(Forward));
		Right = ConvertVectorToCarSpace(CoordinateSpace, RotatorAdj.RotateVector(Right));
		Up = ConvertVectorToCarSpace(CoordinateSpace, RotatorAdj.RotateVector(Up));
		Forward.Normalize();
		Right.Normalize();
		Up.Normalize();

		// The implementation could be done in a more simple way, but the tests wouldn't compile in this case.
		// The alternative would be:
		// FRotator NewRotator = FTransform(Forward, Right, Up, FVector(0, 0, 0)).Rotator();
		FRotator NewRotator{
			FMath::RadiansToDegrees(std::atan2(Forward.Z, std::sqrt(Forward.X * Forward.X + Forward.Y * Forward.Y))),
			FMath::RadiansToDegrees(std::atan2(Forward.Y, Forward.X)),
			0
		};

		FRotationMatrix RotMat(NewRotator);
		FVector SYAxis{ RotMat.M[1][0], RotMat.M[1][1], RotMat.M[1][2] };
		NewRotator.Roll = FMath::RadiansToDegrees(std::atan2(Up | SYAxis, Right | SYAxis));
		return NewRotator;
	}

	const char* GetChannelName(const DMSSimAnimationChannelType Channel) {
		struct {
			DMSSimAnimationChannelType Channel;
			const char* Name;
		}
		const ChannelNameMap[] = {
			{ DMSSimAnimationChannelCommon,        DMSSIM_CHANNEL_NAME_COMMON     },
			{ DMSSimAnimationChannelEyeGaze,       DMSSIM_CHANNEL_NAME_EYE_GAZE   },
			{ DMSSimAnimationChannelEyelids,       DMSSIM_CHANNEL_NAME_EYELIDS    },
			{ DMSSimAnimationChannelFace,          DMSSIM_CHANNEL_NAME_FACE       },
			{ DMSSimAnimationChannelHead,          DMSSIM_CHANNEL_NAME_HEAD       },
			{ DMSSimAnimationChannelUpperBody,     DMSSIM_CHANNEL_NAME_UPPER_BODY },
			{ DMSSimAnimationChannelLeftHand,      DMSSIM_CHANNEL_NAME_LEFT_HAND  },
			{ DMSSimAnimationChannelRightHand,     DMSSIM_CHANNEL_NAME_RIGHT_HAND },
			{ DMSSimAnimationChannelSteeringWheel, DMSSIM_CHANNEL_NAME_STEERING_WHEEL },
		};

		for (const auto& Info : ChannelNameMap) {
			if (Info.Channel == Channel) { return Info.Name; }
		}
		assert(0);
		return "";
	}

	bool IsOrientationObj(YamlObj* const Obj) {
		if (!Obj) { return false; }
		switch (Obj->GetYamlType()) {
		case YamlObjTypeSun:
		case YamlObjTypeCamera:
		case YamlObjTypeSteeringWheelColumn:
		case YamlObjTypeIllumination:
			return true;
		default:
			break;
		}
		return false;
	}

	class YamlCar : public YamlObj {
	public:
		YamlCar() : YamlObj(YamlObjTypeCar) {}
		virtual ~YamlCar() {}
		std::string Model_;
		float       Speed_ = 0.0f;
	};

	class YamlSeat : public YamlObj {
	public:
		YamlSeat() : YamlObj(YamlObjTypeSeat) {}
		virtual ~YamlSeat() {}
		FVector Offset_ = { 0.0f, 0.0f, 0.0f };
	};

	class YamlOrientationObj : public YamlObj {
	public:
		YamlOrientationObj(YamlObjType Type) : YamlObj(Type) {}
		void Recompute(const DMSSimCoordinateSpace& CoordinateSpace);
		std::vector<float> Location_;
		std::vector<float> Rotation_;
		yaml_mark_t        LocationMark_ = {};
		yaml_mark_t        RotationMark_ = {};
		FVector            PositionFinal_ = {};
		FRotator           RotationFinal_ = {};
	};

	void YamlOrientationObj::Recompute(const DMSSimCoordinateSpace& CoordinateSpace) {
		if (Location_.size() != 3) { ThrowExceptionWithLineN_Internal("Invalid Location", LocationMark_); }
		if (Rotation_.size() != 3) { ThrowExceptionWithLineN_Internal("Invalid Rotation", RotationMark_); }
		FVector Position{ Location_[0], Location_[1], Location_[2] };
		PositionFinal_ = ConvertPointToCarSpace(CoordinateSpace, Position);
		FRotator Rotation;
		Rotation.Roll = -Rotation_.at(0);
		Rotation.Pitch = -Rotation_.at(1);
		Rotation.Yaw = -Rotation_.at(2);
		RotationFinal_ = ConvertRotationToCarSpace(CoordinateSpace, Rotation);
	}

	class YamlGroundTruthSettings : public YamlObj, public DMSSimGroundTruthSettings {
	public:
		YamlGroundTruthSettings() : YamlObj(YamlObjTypeGroundTruthSettings) {}
		virtual ~YamlGroundTruthSettings() {}
		virtual float					GetBoundingBoxPaddingFactorFace() const override { return BoundingBoxPaddingFactor_Face_; };
		virtual float					GetEyeBoundingBoxWidthFactor() const override { return EyeBoundingBoxWidthFactor_; };
		virtual float					GetEyeBoundingBoxHeightFactor() const override { return EyeBoundingBoxHeightFactor_; };
		virtual float					GetEyeBoundingBoxDepth() const override { return EyeBoundingBoxDepth_; };

		float							BoundingBoxPaddingFactor_Face_ = 0.0;
		float							EyeBoundingBoxWidthFactor_ = 1.0;
		float							EyeBoundingBoxHeightFactor_ = 1.0;
		float							EyeBoundingBoxDepth_ = 1.0;
	};

	class YamlSun : public YamlOrientationObj {
	public:
		YamlSun(): YamlOrientationObj(YamlObjTypeSun) {}
		virtual ~YamlSun() {}
		float Intensity_ = 4.0f;
		float Temperature_ = 5000.0f;
	};

	class YamlCamera : public YamlOrientationObj, public DMSSimCamera {
	public:
		YamlCamera() : YamlOrientationObj(YamlObjTypeCamera) {}
		virtual ~YamlCamera() {}
		void Recompute(const DMSSimCoordinateSpace& CoordinateSpace);
		bool					GetNIR() const override { return NIR_; };
		bool					GetDepth16Bit() const override { return Depth16Bit_; };
		bool                    GetVideoOut() const override { return VideoOut_; };
		bool                    GetCsvOut() const override { return CsvOut_; }
		unsigned				GetFrameWidth() const override;
		unsigned				GetFrameHeight() const override;
		unsigned				GetFrameRate() const override { return FrameRate_; };
		FVector					GetPosition() const override { return PositionFinal_; };
		FRotator				GetRotation() const override { return RotationFinal_; };
		bool					GetMirrored() const override { return Mirrored_; };
		float					GetFOV() const override { return FOV_; };
		float					GetNoise() const override { return Noise_; };
		float					GetBlur() const override { return Blur_; };
		float					GetFocalDistance() const override { return FocalDistance_ * DMSSIM_M_TO_CM; };
		int						GetDiaphragmBladeCount() const override { return DiaphragmBladeCount_; };
		float					GetMinFStop() const override { return MinFStop_; };
		float					GetMaxFStop() const override { return MaxFStop_; };
		float					GetGrainIntensity() const override { return GrainIntensity_; };
		float					GetGrainJitter() const override { return GrainJitter_; };
		float					GetSaturation() const override { return Saturation_; };
		float					GetGamma() const override { return Gamma_; };
		float					GetContrast() const override { return Contrast_; };
		float					GetBloomIntensity() const override { return BloomIntensity_; };
		float					GetFocusOffset() const override { return FocusOffset_; };

		std::vector<unsigned>	Resolution_;
		yaml_mark_t				ResolutionMark_ = {};
		unsigned				FrameRate_ = DMSSIM_DEFAULT_FRAME_RATE;
		bool					Depth16Bit_ = false;
		bool					NIR_ = false;
		bool					Mirrored_ = false;
		bool                    VideoOut_ = false;
		bool                    CsvOut_ = false;
		float					FOV_ = DMSSIM_DEFAULT_FOV;
		float					Noise_ = DMSSIM_DEFAULT_NOISE;
		float					Blur_ = DMSSIM_DEFAULT_BLUR;
		float					FocalDistance_ = -1.0f;
		float					MinFStop_ = 1.4f;
		float					MaxFStop_ = 4.0f;
		int						DiaphragmBladeCount_ = DMSSIM_DEFAULT_DIAPHRAGM_BLADE_COUNT;
		float					GrainIntensity_ = 0.0f;
		float					GrainJitter_ = 0.0f;
		float					Saturation_ = 1.0f;
		float					Gamma_ = 1.0f;
		float					Contrast_ = 1.0f;
		float					BloomIntensity_ = 0.0f;
		float					FocusOffset_ = 0.0f;
	};

	void YamlCamera::Recompute(const DMSSimCoordinateSpace& CoordinateSpace) {
		if (!Resolution_.empty() && Resolution_.size() != 2) { ThrowExceptionWithLineN_Internal("Invalid number of resolution parameters. Must be 2, width and height.", ResolutionMark_); }
		YamlOrientationObj::Recompute(CoordinateSpace);
	}

	unsigned YamlCamera::GetFrameWidth() const {
		if (!Resolution_.empty()) {
			assert(Resolution_.size() == 2);
			return Resolution_[0];
		}
		return DMSSIM_DEFAULT_FRAME_WIDTH;
	}

	unsigned YamlCamera::GetFrameHeight() const {
		if (!Resolution_.empty()) {
			assert(Resolution_.size() == 2);
			return Resolution_[1];
		}
		return DMSSIM_DEFAULT_FRAME_HEIGHT;
	}

	class DefaultYamlCamera : public YamlCamera {
	public:
		DefaultYamlCamera() : YamlCamera() {}
		unsigned GetFrameWidth() const override { return 1344; }
		unsigned GetFrameHeight() const override { return 1024; }
		unsigned GetFrameRate() const override { return 60; }
		bool GetDepth16Bit() const override { return false; }
		FVector GetPosition() const override { return { 95.2343674f, 37.2700005f, 65.0299988f }; }
		FRotator GetRotation() const override { return { 17.7800007f,  2.64599991f, 0.808000028f }; }
	};

	DefaultYamlCamera DefaultCamera;

	class YamlIllumination : public YamlOrientationObj, public DMSSimIllumination {
	public:
		YamlIllumination() : YamlOrientationObj(YamlObjTypeIllumination) {}
		virtual ~YamlIllumination() {}
		virtual float GetIntensity() const override { return Intensity_; };
		virtual float GetAttenuationRadius() const override { return AttenuationRadius_; };
		virtual float GetSourceRadius() const override { return SourceRadius_; };
		virtual float InnerConeAngle() const override { return InnerConeAngle_; };
		virtual float OuterConeAngle() const override { return OuterConeAngle_; };
		virtual FVector  GetPosition() const override { return PositionFinal_; };
		virtual FRotator GetRotation() const override { return RotationFinal_; };

		DMSSimScenarioParser* Parser_ = nullptr;
		float                 Intensity_ = 0.0f;
		float                 AttenuationRadius_ = 150.0f;
		float				  SourceRadius_ = 2.0f;
		float                 InnerConeAngle_ = 0.0;
		float                 OuterConeAngle_ = 40.0f;
	};

	class YamlSteeringWheelColumn : public YamlOrientationObj, public DMSSimSteeringWheelColumn {
	public:
		YamlSteeringWheelColumn() : YamlOrientationObj(YamlObjTypeSteeringWheelColumn) {}
		virtual ~YamlSteeringWheelColumn() {}
		virtual float GetPitchAngle() const override { return PitchAngle_; };
		virtual bool GetIsCameraIntegrated() const override { return IsCameraIntegrated_; }
		bool				  IsCameraIntegrated_ = false;
		float				  PitchAngle_ = 0.0f;
	};

	class YamlBlendOutParameters : public YamlObj {
	public:
		YamlBlendOutParameters() : YamlObj(YamlObjTypeDefaultBlendOutParameters) {
			std::fill(Parameters_, Parameters_ + DMSSimAnimationChannelCount, DMSSIM_MIN_BLEND_OUT - 1.0f);
			Parameters_[DMSSimAnimationChannelSteeringWheel] = 0;
		}
		virtual ~YamlBlendOutParameters() {}
		virtual void Validate() const override;

		yaml_mark_t StartMark_ = {};
		bool        Defined_ = false;
		float       Parameters_[DMSSimAnimationChannelCount] = {};
	};

	void YamlBlendOutParameters::Validate() const {
		if (!Defined_) { throw std::exception("Default blend out parameters not set"); }
		for (int i = 0; i < DMSSimAnimationChannelCount; ++i) {
			const auto Parameter = Parameters_[i];
			if (Parameter < DMSSIM_MIN_BLEND_OUT) {
				const auto ChannelName = GetChannelName(static_cast<DMSSimAnimationChannelType>(i));
				ThrowExceptionWithLineN_Internal((std::string(ChannelName) + " has no defaut blend_out parameter set").c_str(), StartMark_);
			}
		}
	}

	class YamlChannelsParameters : public YamlObj {
	public:
		YamlChannelsParameters() : YamlObj(YamlObjTypeChannelsParameters) {}
		virtual ~YamlChannelsParameters() {}
		virtual void Validate() const override { BlendOutParameters_.Validate(); };
		YamlBlendOutParameters BlendOutParameters_;
	};

	class YamlOccupant : public YamlObj, public DMSSimOccupant {
	public:
		YamlOccupant(FDMSSimOccupantType Type) : YamlObj(YamlObjTypeOccupant), Type_(Type) {}
		virtual ~YamlOccupant() {}
		void Validate() const override { if (!GlassesColor_.empty() && GlassesColor_.size() != 3) { ThrowExceptionWithLineN_Internal("Glasses Color must consist of 3 components.", GlassesColorMark_); } };
		FDMSSimOccupantType GetType() const override { return Type_; };
		const char* GetCharacter() const override { return Character_.c_str(); };
		const char* GetHeadgear() const override { return Headgear_.empty() ? nullptr : Headgear_.c_str(); };
		const char* GetUpperCloth() const override { return UpperCloth_.empty() ? nullptr : UpperCloth_.c_str(); };
		const char* GetGlasses() const override { return Glasses_.empty() ? nullptr : Glasses_.c_str(); };
		FVector     GetGlassesColor() const override { return GlassesColor_.size() == 3 ? FVector{ GlassesColor_[0], GlassesColor_[1], GlassesColor_[2] } : FVector{ 0, 0, 0 }; };
		float       GetGlassesOpacity() const override { return GlassesOpacity_; };
		bool       GetGlassesReflective() const override { return GlassesReflective_; };
		virtual const char* GetMask() const override { return Mask_.empty() ? nullptr : Mask_.c_str(); };
		const char* GetScarf() const override { return Scarf_.empty() ? nullptr: Scarf_.c_str(); };
		const char* GetHair() const override { return Hair_.empty() ? nullptr : Hair_.c_str(); };
		const char* GetBeard() const override { return Beard_.empty() ? nullptr : Beard_.c_str(); };
		const char* GetMustache() const override { return Mustache_.empty() ? nullptr : Mustache_.c_str(); };
		float GetPupilSize() const override { return PupilSize_; };
		float GetPupilBrightness() const override { return PupilBrightness_; };
		float GetIrisSize() const override { return IrisSize_; };
		float GetIrisBrightness() const override { return IrisBrightness_; };
		float GetIrisBorderWidth() const override { return IrisBorderWidth_; };
		float GetLimbusDarkAmount() const override { return LimbusDarkAmount_; };
		const char* GetIrisColor() const override { return IrisColor_.c_str(); };
		float GetScleraBrightness() const override { return ScleraBrightness_; };
		float GetScleraVeins() const override { return ScleraVeins_; };
		float GetSkinWrinkles() const override { return SkinWrinkles_; };
		float GetSkinRoughness() const override { return SkinRoughness_; };
		float GetSkinSpecularity() const override { return SkinSpecularity_; };
		float GetHeight() const override { return Height_; };
		FVector GetSeatOffset() const override { return Seat_.Offset_; };
		FDMSSimOccupantType Type_;
		std::string        Character_;
		std::string        Headgear_;
		std::string		   UpperCloth_;
		std::string        Glasses_;
		yaml_mark_t        GlassesColorMark_ = {};
		std::vector<float> GlassesColor_;
		float              GlassesOpacity_ = DMSSIM_DEFAULT_GLASSES_OPACITY;
		bool			   GlassesReflective_ = true;
		std::string        Mask_;
		std::string        Scarf_;
		std::string        Hair_;
		std::string        Beard_;
		std::string        Mustache_;
		float              PupilSize_ = 1.12f;
		float              PupilBrightness_ = 0.0f;
		float              IrisSize_ = 0.53f;
		float              IrisBrightness_ = 1.f;
		float			   IrisBorderWidth_ = 0.04f;
		float              LimbusDarkAmount_ = 0.5;
		std::string        IrisColor_ ="";
		float              ScleraBrightness_ = 0.5f;
		float              ScleraVeins_ = 0.5f;
		float              SkinWrinkles_ = 1.f;
		float              SkinRoughness_ = 1.f;
		float              SkinSpecularity_ = 1.f;
		float              Height_ = 0.0f;
		YamlSeat           Seat_;
	};

	class YamlOccupants : public YamlObj {
	public:
		YamlOccupants() : YamlObj(YamlObjTypeOccupants) {}
		std::vector<YamlOccupant> Occupants_;
	};

	class YamlRandomMovements : public YamlObj{
	public:
		YamlRandomMovements() : YamlObj(YamlObjTypeRandomMovements) {}
		virtual ~YamlRandomMovements() {}
		
		bool Blinking_ = false;
		bool Smiling_ = false;
		bool Head_ = false;
		bool Body_ = false;
		bool Gaze_ = false;
	};

	class YamlAnimationPoint : public YamlObj, public DMSSimAnimationPoint {
	public:
		YamlAnimationPoint() : YamlObj(YamlObjTypeAnimationPoint) {}
		virtual ~YamlAnimationPoint() {}
		virtual void Validate() const override { assert(0); }
		virtual void ValidatePoint(DMSSimAnimationType AnimationType) const;
		virtual float GetTime() const { return Time_; }
		virtual FVector GetPoint() const { return Values_.size() == 3 ? FVector{ Values_.at(0), Values_.at(1), Values_.at(2) }: FVector{ Values_.at(0), 0, 0 }; };

		yaml_mark_t StartMark_ = {};
		float Time_ = 0;
		std::vector<float> Values_;
	};

	void YamlAnimationPoint::ValidatePoint(const DMSSimAnimationType AnimationType) const {
		const size_t ExpectedValueCount = (AnimationType == DMSSimAnimationParametricSteeringWheel)? 1 : 3;
		if (Values_.size() != ExpectedValueCount) {
			std::stringstream Msg;
			Msg << "Invalid animation curve point. Must be " << ExpectedValueCount << " values.";
			ThrowExceptionWithLineN_Internal(Msg.str().c_str(), StartMark_);
		}
	}

	class YamlMotion : public YamlObj, public DMSSimMotion {
	public:
		YamlMotion() : YamlObj(YamlObjTypeMotion) {}
		virtual ~YamlMotion() {}
		virtual bool IsComplexSequence() const override { return Type_ == DMSSimMotionParametric; }
		virtual YamlObj* StartMapping(const yaml_event_t& Event) override;
		virtual void Validate() const override { assert(0); }
		virtual void ValidateMotion(DMSSimAnimationType AnimationType) const;
		virtual DMSSimMotionType GetType() const override { return Type_; };
		virtual const char* GetAnimationName() const override { return Animation_.c_str(); };
		virtual size_t GetPointCount() const override { return Points_.size(); };
		virtual const DMSSimAnimationPoint& GetPoint(size_t Index) const override { return Points_.at(Index); };
		virtual float GetStartPos() const override { return StartPos_; };
		virtual float GetEndPos() const override { return EndPos_; };
		virtual float GetDuration() const override { return Duration_; };
		virtual float GetBlendOut() const override { return BlendOut_; };
		void RecomputePoints(const DMSSimCoordinateSpace& CoordinateSpace);

		std::string            Animation_;
		DMSSimMotionType       Type_ = DMSSimMotionAnimation;
		std::vector<YamlAnimationPoint> Points_;
		float                  StartPos_ = 0.0f;
		float                  EndPos_ = -1.0f;
		float                  Duration_ = -1.0f;
		float                  BlendOut_ = -1.0f;
		yaml_mark_t            StartMark_ = {};
	};

	YamlObj* YamlMotion::StartMapping(const yaml_event_t& Event) {
		if (Type_ == DMSSimMotionParametric && Sequence_) {
			YamlAnimationPoint Point;
			Point.StartMark_ = Event.start_mark;
			Points_.push_back(Point);
			return &Points_.back();
		}
		return this;
	}

	void YamlMotion::ValidateMotion(const DMSSimAnimationType AnimationType) const {
		if (Duration_ < DMSSIM_MIN_ANIMATION_DURATION) {
			switch (GetType()) {
			case DMSSimMotionPauseActive:
				ThrowExceptionWithLineN_Internal("Active pause requires duration!", StartMark_);
				break;
			case DMSSimMotionPausePassive:
				ThrowExceptionWithLineN_Internal("Passive pause requires duration!", StartMark_);
				break;
			}
		}

		if (Type_ != DMSSimMotionParametric) return;

		if (Points_.empty()) { ThrowExceptionWithLineN_Internal("No parametric animation points defined", StartMark_); }

		const auto& Point0 = Points_.front();
		if (Point0.GetTime() >= DMSSIM_MIN_ANIMATION_DURATION) { ThrowExceptionWithLineN_Internal("Time of the first point must be 0.0", Point0.StartMark_); }

		float PrevTime = -1.0f;
		for (const auto& Point : Points_) {
			Point.ValidatePoint(AnimationType);
			if (PrevTime > Point.GetTime()) { ThrowExceptionWithLineN_Internal("Points must be time-ordered", Point.StartMark_); }
			PrevTime = Point.GetTime();
		}
	}

	void YamlMotion::RecomputePoints(const DMSSimCoordinateSpace& CoordinateSpace) {
		if (Type_ != DMSSimMotionParametric) { return; }
		if (IsCurveInCarSpace(Animation_.c_str())) {
			for (auto& PointInfo: Points_) {
				auto Point = PointInfo.GetPoint();
				Point = ConvertPointToCarSpace(CoordinateSpace, Point);
				PointInfo.Values_ = { Point.X, Point.Y, Point.Z };
			}
		}
		else if (Animation_ == DMSSIM_PARAMETER_STEERING_WHEEL) {
			for (auto& PointInfo : Points_) {
				for (auto& Value: PointInfo.Values_) { Value = FMath::RadiansToDegrees(Value); }
			}
		}
	}

	class YamlAnimationChannel : public YamlObj, public DMSSimAnimationChannel {
	public:
		YamlAnimationChannel() : YamlObj(YamlObjTypeAnimationChannel) {}
		virtual ~YamlAnimationChannel() {}
		virtual YamlObj* StartMapping(const yaml_event_t& Event) override;
		virtual void Validate() const override { for(const auto& Motion : Motions_) Motion.ValidateMotion(DMSSimAnimationUnknown); };
		virtual DMSSimAnimationChannelType GetType() const override { return Type_; };
		virtual size_t GetMotionCount() const override { return Motions_.size(); };
		virtual const DMSSimMotion& GetMotion(size_t MotionIndex) const override { return Motions_.at(MotionIndex); };

		DMSSimAnimationChannelType Type_ = DMSSimAnimationChannelCommon;
		std::vector<YamlMotion>    Motions_;
	};

	YamlObj* YamlAnimationChannel::StartMapping(const yaml_event_t& Event) {
		if (Sequence_) {
			YamlMotion Motion;
			Motion.StartMark_ = Event.start_mark;
			Motions_.push_back(Motion);
			return &Motions_.back();
		}
		return nullptr;
	}

	class YamlAnimationSequence : public YamlObj, public DMSSimAnimationSequence {
	public:
		YamlAnimationSequence() : YamlObj(YamlObjTypeAnimationSequence) {}
		virtual ~YamlAnimationSequence() {}
		virtual bool IsComplexSequence() const override { return true; }
		virtual void Validate() const override;
		virtual const char* GetName() const override { return Name_.c_str(); };
		virtual DMSSimAnimationType GetType() const override { return Type_; };
		virtual size_t GetMotionCount() const override { return Motions_.size(); };
		virtual const DMSSimMotion& GetMotion(size_t MotionIndex) const override { return Motions_.at(MotionIndex); };

		void RecomputePoints(const DMSSimCoordinateSpace& CoordinateSpace){ for(auto& Motion : Motions_) Motion.RecomputePoints(CoordinateSpace); };

		std::string                        Name_;
		DMSSimAnimationType                Type_ = DMSSimAnimationUnknown;
		std::vector<YamlMotion>            Motions_;
		bool                               WithParameters_ = false;
		yaml_mark_t                        StartMark_ = {};
	};

	void YamlAnimationSequence::Validate() const {
		if (Motions_.empty()) { ThrowExceptionWithLineN_Internal((std::string("Empty animation sequence ") + Name_).c_str(), StartMark_); }
		for (const auto& Motion : Motions_) { Motion.ValidateMotion(Type_); }
	}

	class YamlAnimations : public YamlObj {
	public:
		YamlAnimations() : YamlObj(YamlObjTypeAnimations) {}
		virtual ~YamlAnimations() {}
		virtual void Validate() const override { for(const auto& Sequence : Sequences_) Sequence.Validate(); };
		void RecomputePoints(const DMSSimCoordinateSpace& CoordinateSpace) { for(auto& Sequence : Sequences_) Sequence.RecomputePoints(CoordinateSpace); };
		std::vector<YamlAnimationSequence> Sequences_;
	};	

	class YamlOccupantScenario : public YamlObj, public DMSSimOccupantScenario {
	public:
		YamlOccupantScenario(FDMSSimOccupantType Type) : YamlObj(YamlObjTypeOccupantScenario), Type_(Type) {}
		virtual ~YamlOccupantScenario() {}
		virtual void Validate() const override { for (const auto& Channel : Channels_) Channel.Validate(); };
		virtual FDMSSimOccupantType GetType() const override { return Type_; };
		virtual size_t GetMotionCount() const override { return Motions_.size(); };
		virtual const DMSSimMotion& GetMotion(size_t MotionIndex) const override { return Motions_.at(MotionIndex); };
		virtual size_t GetChannelCount() const override { return Channels_.size(); };
		virtual const DMSSimAnimationChannel& GetChannel(size_t ChannelIndex) const override { return Channels_.at(ChannelIndex); };

		FDMSSimOccupantType                Type_;
		std::vector<YamlMotion>           Motions_;
		std::vector<YamlAnimationChannel> Channels_;
	};

	class YamlScenario : public YamlObj {
	public:
		YamlScenario(): YamlObj(YamlObjTypeScenario){}
		virtual ~YamlScenario(){}
		std::vector<YamlOccupantScenario> Occupants_;
	};

	class DMSSimCoordinateSpaceObj: public YamlObj, public DMSSimCoordinateSpace {
	public:
		DMSSimCoordinateSpaceObj(): YamlObj(YamlObjTypeCoordinateSpace) {}
		virtual ~DMSSimCoordinateSpaceObj() {}
		virtual const char* GetCarModel() const override { return CarModel_.c_str(); }
		virtual FRotator GetRotation() const override { return Rotation_; }
		virtual FVector  GetTranslation() const override { return Translation_; }
		virtual FVector  GetScale() const override { return Scale_; }
		void Recompute();
		virtual void Validate() const override;

		std::string         CarModel_;
		FRotator            Rotation_ = {};
		FVector             Translation_ = { -1.411584f, 0.0f, -0.65192f };
		FVector             Scale_ = { 1.0f, -1.0f, 1.0f };
		bool                Custom_ = false;
		DMSSimParserVector  CustomRotation_;
		DMSSimParserVector  CustomTranslation_;
		DMSSimParserVector  CustomScale_;
	};

	void DMSSimCoordinateSpaceObj::Validate() const {
		if (CustomRotation_.Values_.size() != 3) { ThrowExceptionWithLineN_Internal("Coordinate Space - Invalid Rotation", CustomTranslation_.Mark_); }
		if (CustomTranslation_.Values_.size() != 3) { ThrowExceptionWithLineN_Internal("Coordinate Space - Invalid Translation", CustomTranslation_.Mark_); } 
		if (CustomScale_.Values_.size() != 3) { ThrowExceptionWithLineN_Internal("Coordinate Space - Invalid Scale", CustomScale_.Mark_); }
	}

	void DMSSimCoordinateSpaceObj::Recompute() {
		if (!Custom_) { return; }
		Validate();
		Rotation_ = ValuesToRotator(CustomRotation_.Values_);
		Translation_ = ValuesToVector(CustomTranslation_.Values_);
		Scale_ = ValuesToVector(CustomScale_.Values_);
		Scale_.Y = -Scale_.Y;
	}

	DMSSimCoordinateSpaceObj DefaultCoordinateSpaceObj;

	class DMSSimScenarioParserImpl: public DMSSimScenarioParser, public DMSSimParserBase {
	public:
		DMSSimScenarioParserImpl() {}
		virtual ~DMSSimScenarioParserImpl(){}

		bool Initialize(const wchar_t* const FilePath, const DMSSimConfigParser& Config);

		//getter helper methods
		unsigned GetVersionMajor() const override { return MajorVersion_; }
		unsigned GetVersionMinor() const override { return MinorVersion_; }
		const char* GetDescription() const override { return Description_.empty() ? nullptr : Description_.c_str(); }
		const char* GetEnvironment() const override { return Environment_.c_str(); }
		bool GetRandomBlinking() const override { return RandomMovements_.Blinking_; }
		bool GetRandomSmiling() const override { return RandomMovements_.Smiling_; }
		bool GetRandomHeadMovements() const override { return RandomMovements_.Head_; }
		bool GetRandomBodyMovements() const override { return RandomMovements_.Body_; }
		bool GetRandomGaze() const override { return RandomMovements_.Gaze_; }
		const char* GetCarModel() const override { return Car_.Model_.empty() ? nullptr : Car_.Model_.c_str(); }
		float GetCarSpeed() const override { return Car_.Speed_; }
		FRotator GetSunRotation() const override { return Sun_.RotationFinal_; }
		float GetSunIntensity() const override { return Sun_.Intensity_; }
		float GetSunTemperature() const override { return Sun_.Temperature_; }
		const DMSSimCoordinateSpace& GetCoordinateSpace() const override { return CoordinateSpace_; }
		const DMSSimCamera& GetCamera() const override { return Camera_; }
		const DMSSimSteeringWheelColumn& GetSteeringWheelColumn() const override { return SteeringWheelColumn_; };
		const DMSSimGroundTruthSettings& GetGroundTruthSettings() const override { return GroundTruthSettings_; };
		
		const DMSSimIllumination& GetCameraIllumination() const override { return Illumination_; }
		float GetDefaultBlendOut(DMSSimAnimationChannelType Channel) const override;
		size_t GetOccupantCount() const override { return Occupants_.Occupants_.size(); }
		const DMSSimOccupant& GetOccupant(size_t OccupantIndex) const override { return Occupants_.Occupants_.at(OccupantIndex); }
		size_t GetAnimationSequenceCount() const override { return Animations_.Sequences_.size(); };
		const DMSSimAnimationSequence& GetAnimationSequence(size_t Index) const override { return Animations_.Sequences_.at(Index); }
		size_t GetOccupantScenarioCount() const override { return Scenario_.Occupants_.size(); }
		const DMSSimOccupantScenario& GetOccupantScenario(size_t Index) const override { return Scenario_.Occupants_.at(Index); }

		//yaml event handlers
		YamlObj* EventHandler_description(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_environment(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_random_movements(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_random_smiling(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_random_blinking(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_random_head(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_random_body(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_random_gaze(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_car(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_model(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_speed(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_sun(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_sun_intensity(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_sun_temperature(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_camera(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_spectrum(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_resolution(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_transform_base_to_project(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_framerate(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_bit_depth(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_location(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_rotation(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_translation(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_scale(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_mirrored(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_fov(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_video_out(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_csv_out(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_min_fstop(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_max_fstop(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_focal_distance(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_diaphragm_blade_count(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_illumination(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_intensity(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_outer_cone_angle(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_attenuation_radius(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_source_radius(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_noise(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_blur(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_grain_intensity(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_grain_jitter(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_saturation(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_gamma(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_contrast(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_bloom_intensity(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_focus_offset(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_ground_truth_settings(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_bounding_box_padding_factor_face(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_eye_bounding_box_width_factor(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_eye_bounding_box_height_factor(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_eye_bounding_box_depth(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_steering_wheel_column(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_is_camera_integrated(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_pitch_angle(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_channels_parameters(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_channels_blendout_defaults(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_occupants(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_driver(const yaml_event_t* Event, YamlObj* Obj, bool Enter) { return EventHandler_OccupantInternal(Event, Obj, Enter, FDMSSimOccupantType::Driver, "driver"); };
		YamlObj* EventHandler_passenger_front(const yaml_event_t* Event, YamlObj* Obj, bool Enter) { return EventHandler_OccupantInternal(Event, Obj, Enter, FDMSSimOccupantType::PassengerFront, "front passenger"); };
		YamlObj* EventHandler_passenger_rear_left(const yaml_event_t* Event, YamlObj* Obj, bool Enter) { return EventHandler_OccupantInternal(Event, Obj, Enter, FDMSSimOccupantType::PassengerRearLeft, "rear left passenger"); };
		YamlObj* EventHandler_passenger_rear_middle(const yaml_event_t* Event, YamlObj* Obj, bool Enter) { return EventHandler_OccupantInternal(Event, Obj, Enter, FDMSSimOccupantType::PassengerRearMiddle, "rear middle passenger"); };
		YamlObj* EventHandler_passenger_rear_right(const yaml_event_t* Event, YamlObj* Obj, bool Enter) { return EventHandler_OccupantInternal(Event, Obj, Enter, FDMSSimOccupantType::PassengerRearRight, "rear right passenger"); }
		YamlObj* EventHandler_character(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_headgear(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_upper_cloth(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_glasses(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_glasses_color(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_glasses_opacity(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_glasses_reflective(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_mask(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_scarf(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_hair(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_beard(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_mustache(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_pupil_size(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_pupil_brightness(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_iris_size(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_iris_brightness(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_limbus_dark_amount(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_iris_color(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_iris_border_width(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_sclera_brightness(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_sclera_veins(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_skin_wrinkles(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_skin_roughness(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_skin_specularity(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_height(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_seat(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_x_offset(const yaml_event_t* Event, YamlObj* Obj, bool Enter) { return EventHandler_offset_internal(Event, Obj, Enter, "x_offset", [](FVector& V) -> float& { return V.X; }); };
		YamlObj* EventHandler_y_offset(const yaml_event_t* Event, YamlObj* Obj, bool Enter) { return EventHandler_offset_internal(Event, Obj, Enter, "y_offset", [](FVector& V) -> float& { return V.Y; }); };
		YamlObj* EventHandler_z_offset(const yaml_event_t* Event, YamlObj* Obj, bool Enter) { return EventHandler_offset_internal(Event, Obj, Enter, "z_offset", [](FVector& V) -> float& { return V.Z; }); };
		YamlObj* EventHandler_animations(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_name(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_type(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_parameter(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_parameters(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_points(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_time(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_value(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_sequence(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_motion(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_animation(const yaml_event_t* Event, YamlObj* Obj, bool Enter) { return EventHandler_animation_internal(Event, Obj, Enter, false); };
		YamlObj* EventHandler_animation_internal(const yaml_event_t* Event, YamlObj* Obj, bool Enter, bool Pause);
		YamlObj* EventHandler_pause(const yaml_event_t* Event, YamlObj* Obj, bool Enter) { return EventHandler_animation_internal(Event, Obj, Enter, true); };
		YamlObj* EventHandler_duration(const yaml_event_t* Event, YamlObj* Obj, bool Enter) { return EventHandler_animation_parameter(Event, Obj, Enter, "duration", false, [](YamlMotion* Motion, float Value) { Motion->Duration_ = Value; }, DMSSIM_MIN_ANIMATION_DURATION, DMSSIM_MAX_ANIMATION_DURATION); };
		YamlObj* EventHandler_start_pos(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_end_pos(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_blend_out(const yaml_event_t* Event, YamlObj* Obj, bool Enter) { return EventHandler_animation_parameter(Event, Obj, Enter, "blend_out", false, [](YamlMotion* Motion, float Value) { Motion->BlendOut_ = Value; }, DMSSIM_MIN_BLEND_OUT, DMSSIM_MAX_BLEND_OUT); };
		YamlObj* EventHandler_animation_channel(const yaml_event_t* Event, YamlObj* Obj, bool Enter, DMSSimAnimationChannelType Type);
		YamlObj* EventHandler_common(const yaml_event_t* Event, YamlObj* Obj, bool Enter) { return EventHandler_animation_channel(Event, Obj, Enter, DMSSimAnimationChannelCommon); };
		YamlObj* EventHandler_eye_gaze(const yaml_event_t* Event, YamlObj* Obj, bool Enter) { return EventHandler_animation_channel(Event, Obj, Enter, DMSSimAnimationChannelEyeGaze); };
		YamlObj* EventHandler_eyelids(const yaml_event_t* Event, YamlObj* Obj, bool Enter) { return EventHandler_animation_channel(Event, Obj, Enter, DMSSimAnimationChannelEyelids); };
		YamlObj* EventHandler_face(const yaml_event_t* Event, YamlObj* Obj, bool Enter) { return EventHandler_animation_channel(Event, Obj, Enter, DMSSimAnimationChannelFace); };
		YamlObj* EventHandler_head(const yaml_event_t* Event, YamlObj* Obj, bool Enter) { return EventHandler_animation_channel(Event, Obj, Enter, DMSSimAnimationChannelHead); };
		YamlObj* EventHandler_upper_body(const yaml_event_t* Event, YamlObj* Obj, bool Enter) { return EventHandler_animation_channel(Event, Obj, Enter, DMSSimAnimationChannelUpperBody); };
		YamlObj* EventHandler_left_hand(const yaml_event_t* Event, YamlObj* Obj, bool Enter) { return EventHandler_animation_channel(Event, Obj, Enter, DMSSimAnimationChannelLeftHand); };
		YamlObj* EventHandler_right_hand(const yaml_event_t* Event, YamlObj* Obj, bool Enter) { return EventHandler_animation_channel(Event, Obj, Enter, DMSSimAnimationChannelRightHand); };
		YamlObj* EventHandler_steering_wheel(const yaml_event_t* Event, YamlObj* Obj, bool Enter) { return EventHandler_animation_channel(Event, Obj, Enter, DMSSimAnimationChannelSteeringWheel); };
		YamlObj* EventHandler_scenario(const yaml_event_t* Event, YamlObj* Obj, bool Enter);

	protected:
		const char* FindFunction(const yaml_event_t& Event) const override;
		YamlObj* ProcessEvent(const char* FuncName, const yaml_event_t& Event, YamlObj* Obj, bool Enter) override;

	private:
		std::string               Description_;
		std::string               Environment_ = "default";
		YamlRandomMovements       RandomMovements_;
		DMSSimCoordinateSpaceObj  CoordinateSpace_;
		YamlSun                   Sun_;
		YamlCar                   Car_;
		YamlCamera                Camera_;
		YamlGroundTruthSettings	  GroundTruthSettings_;
		YamlSteeringWheelColumn	  SteeringWheelColumn_;
		YamlIllumination          Illumination_;
		YamlChannelsParameters    ChannelsParameters_;
		YamlOccupants             Occupants_;
		YamlAnimations            Animations_;
		YamlScenario              Scenario_;

		bool IsOccupantExits(FDMSSimOccupantType Type);
		bool IsOccupantScenarioExits(FDMSSimOccupantType Type);
		void PopObjStack(std::stack<YamlObj*>& Stack) { if (!Stack.empty()) Stack.pop(); };
		void CheckAnimationRange(const yaml_event_t* const Event, const YamlObj* const Obj, bool Enter);

		YamlObj* EventHandler_fov_internal(const yaml_event_t* Event, YamlObj* Obj, bool Enter, const char* PropertyName, const std::function<void(float)>& Setter);

		template <class TSelector>
		YamlObj* EventHandler_offset_internal(const yaml_event_t* Event, YamlObj* Obj, bool Enter, const char* ParamName, TSelector Selector);

		YamlObj* EventHandler_animation_parameter(const yaml_event_t* Event, YamlObj* Obj, bool Enter, const char* const Parameter, bool NoPause, const std::function<void(YamlMotion*, float)>& Setter, float MinValue, float MaxValue);
		YamlObj* EventHandler_OccupantInternal(const yaml_event_t* Event, YamlObj* Obj, bool Enter, FDMSSimOccupantType Type, const char* Name);

		void ValidateParameters(const DMSSimConfigParser& Config);
		const DMSSimCoordinateSpace* GetCoordinateSpace(const DMSSimConfigParser& Config) const;
	};

	namespace {
		typedef YamlObj* (DMSSimScenarioParserImpl::* ProcessYamlEventFunc)(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
#define DMSSIM_DEFINE_YAML_EVENT_HANDLER(name) { #name, &DMSSimScenarioParserImpl::EventHandler_##name },

		/**
		 * Table of handlers for supported node types.
		 * Every new node type needs to be added to this table.
		 */
		struct {
			const char* const    Name;
			ProcessYamlEventFunc Func;
		}
		const EventHandlers[] = {
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(version)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(description)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(environment)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(random_movements)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(random_smiling)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(random_blinking)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(random_head)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(random_body)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(random_gaze)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(car)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(model)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(speed)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(sun)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(sun_intensity)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(sun_temperature)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(camera)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(spectrum)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(resolution)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(framerate)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(bit_depth)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(transform_base_to_project)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(location)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(rotation)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(translation)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(scale)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(mirrored)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(fov)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(video_out)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(csv_out)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(min_fstop)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(max_fstop)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(focal_distance)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(diaphragm_blade_count)

			DMSSIM_DEFINE_YAML_EVENT_HANDLER(noise)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(blur)

			DMSSIM_DEFINE_YAML_EVENT_HANDLER(illumination)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(intensity)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(outer_cone_angle)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(attenuation_radius)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(source_radius)

			DMSSIM_DEFINE_YAML_EVENT_HANDLER(steering_wheel_column)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(is_camera_integrated)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(pitch_angle)			
			
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(ground_truth_settings)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(bounding_box_padding_factor_face)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(eye_bounding_box_width_factor)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(eye_bounding_box_height_factor)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(eye_bounding_box_depth)

			DMSSIM_DEFINE_YAML_EVENT_HANDLER(grain_intensity)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(grain_jitter)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(saturation)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(gamma)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(contrast)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(bloom_intensity)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(focus_offset)

			DMSSIM_DEFINE_YAML_EVENT_HANDLER(channels_parameters)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(channels_blendout_defaults)

			DMSSIM_DEFINE_YAML_EVENT_HANDLER(occupants)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(driver)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(passenger_front)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(passenger_rear_left)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(passenger_rear_middle)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(passenger_rear_right)

			DMSSIM_DEFINE_YAML_EVENT_HANDLER(character)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(headgear)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(upper_cloth)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(glasses)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(glasses_color)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(glasses_opacity)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(glasses_reflective)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(mask)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(scarf)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(hair)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(beard)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(mustache)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(pupil_size)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(pupil_brightness)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(iris_size)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(iris_brightness)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(limbus_dark_amount)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(iris_color)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(iris_border_width)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(sclera_brightness)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(sclera_veins)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(skin_wrinkles)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(skin_roughness)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(skin_specularity)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(height)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(seat)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(x_offset)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(y_offset)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(z_offset)

			DMSSIM_DEFINE_YAML_EVENT_HANDLER(animations)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(name)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(type)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(parameter)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(parameters)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(points)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(time)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(value)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(sequence)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(motion)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(animation)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(pause)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(duration)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(blend_out)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(start_pos)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(end_pos)

			DMSSIM_DEFINE_YAML_EVENT_HANDLER(common)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(eye_gaze)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(eyelids)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(face)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(head)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(upper_body)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(left_hand)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(right_hand)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(steering_wheel)

			DMSSIM_DEFINE_YAML_EVENT_HANDLER(scenario)
		};
#undef DMSSIM_DEFINE_YAML_EVENT_HANDLER
	} // anonymous namespace

	bool DMSSimScenarioParserImpl::Initialize(const wchar_t* const FilePath, const DMSSimConfigParser& Config) {
		const bool Result = InitializeInternal(FilePath);
		if (Result) { ValidateParameters(Config); }
		return Result;
	}

	const char* DMSSimScenarioParserImpl::FindFunction(const yaml_event_t& Event) const {
		const auto* const Info = FindEventHandlerEx(Event, EventHandlers);
		if (Info) { return Info->Name; }
		return nullptr;
	}

	YamlObj* DMSSimScenarioParserImpl::ProcessEvent(const char* FuncName, const yaml_event_t& Event, YamlObj* Obj, bool Enter) {
		const auto* const Info = FindEventHandler(FuncName, EventHandlers);
		if (Info) { return (this->*Info->Func)(&Event, Obj, Enter); }
		return nullptr;
	}

	bool DMSSimScenarioParserImpl::IsOccupantExits(FDMSSimOccupantType Type){
		for (const auto& Occupant : Occupants_.Occupants_) { if (Occupant.Type_ == Type) { return true; } }
		return false;
	}

	bool DMSSimScenarioParserImpl::IsOccupantScenarioExits(FDMSSimOccupantType Type) {
		for (const auto& Occupant : Scenario_.Occupants_) { if (Occupant.Type_ == Type) { return true; } }
		return false;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_description(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (Obj) { ThrowExceptionWithLineN("Description block must be on the base level", Event); }
		if (!Enter) { Description_.assign(reinterpret_cast<const char*>(Event->data.scalar.value)); }
		return nullptr;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_environment(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (Obj) { ThrowExceptionWithLineN("Environment block must be on the base level", Event); }
		if (!Enter) {
			Environment_.assign(reinterpret_cast<const char*>(Event->data.scalar.value));
			if (Environment_.empty()) { ThrowExceptionWithLineN("Empty environment name", Event); }
		}
		return nullptr;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_random_movements(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (Obj) { ThrowExceptionWithLineN("Random movements block must be on the base level", Event); }
		if (Enter) { return &RandomMovements_; }
		return nullptr;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_random_smiling(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeRandomMovements) { ThrowExceptionWithLineN("The smiling property belongs only to the random movements object", Event); }
		if (!Enter) {
			auto RandomMovements = static_cast<YamlRandomMovements*>(Obj);
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strcmp(Value, "no") == 0) { RandomMovements->Smiling_ = false; }
			else if (strcmp(Value, "yes") == 0) { RandomMovements->Smiling_ = true; }
			else { ThrowExceptionWithLineN("Random movements, smiling must be \"yes\" or \"no\"", Event); }
		}
		return Obj;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_random_blinking(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeRandomMovements) { ThrowExceptionWithLineN("The blinking property belongs only to the random movements object", Event); }
		if (!Enter) {
			auto RandomMovements = static_cast<YamlRandomMovements*>(Obj);
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strcmp(Value, "no") == 0) { RandomMovements->Blinking_ = false; }
			else if (strcmp(Value, "yes") == 0) { RandomMovements->Blinking_ = true; }
			else { ThrowExceptionWithLineN("Random movements, blinking must be \"yes\" or \"no\"", Event); }
		}
		return Obj;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_random_head(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeRandomMovements) { ThrowExceptionWithLineN("The random head movement property belongs only to the random movements object", Event); }
		if (!Enter) {
			auto RandomMovements = static_cast<YamlRandomMovements*>(Obj);
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strcmp(Value, "no") == 0) { RandomMovements->Head_ = false; }
			else if (strcmp(Value, "yes") == 0) { RandomMovements->Head_ = true; }
			else { ThrowExceptionWithLineN("Random movements, random_head must be \"yes\" or \"no\"", Event); }
		}
		return Obj;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_random_body(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeRandomMovements) { ThrowExceptionWithLineN("The random body movement property belongs only to the random movements object", Event); }
		if (!Enter) {
			auto RandomMovements = static_cast<YamlRandomMovements*>(Obj);
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strcmp(Value, "no") == 0) { RandomMovements->Body_ = false; }
			else if (strcmp(Value, "yes") == 0) { RandomMovements->Body_ = true; }
			else { ThrowExceptionWithLineN("Random movements, random_body must be \"yes\" or \"no\"", Event); }
		}
		return Obj;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_random_gaze(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeRandomMovements) { ThrowExceptionWithLineN("The random gaze property belongs only to the random movements object", Event); }
		if (!Enter) {
			auto RandomMovements = static_cast<YamlRandomMovements*>(Obj);
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strcmp(Value, "no") == 0) { RandomMovements->Gaze_ = false; }
			else if (strcmp(Value, "yes") == 0) { RandomMovements->Gaze_= true; }
			else { ThrowExceptionWithLineN("Random movements, random gaze must be \"yes\" or \"no\"", Event); }
		}
		return Obj;
	}
	YamlObj* DMSSimScenarioParserImpl::EventHandler_car(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (Obj) { ThrowExceptionWithLineN("car block must be on the base level", Event); }
		if (Enter) { return &Car_; }
		return nullptr;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_model(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Enter) {
			if (!Obj || Obj->GetYamlType() != YamlObjTypeCar) { ThrowExceptionWithLineN("The model property belongs only to the car object", Event); }
			auto Car = static_cast<YamlCar*>(Obj);
			Car->Model_.assign(reinterpret_cast<const char*>(Event->data.scalar.value));
		}
		return Obj;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_speed(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCar) { ThrowExceptionWithLineN("The speed property belongs only to the car object", Event); }
		auto Car = static_cast<YamlCar*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty speed property", Event); }
			const float Speed = ParseFloat(Event, Event->data.scalar.value, "speed");
			Car->Speed_ = Speed;
		}
		return Car;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_sun(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (Obj) { ThrowExceptionWithLineN("sun block must be on the base level", Event); }
		return &Sun_;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_sun_intensity(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeSun) { ThrowExceptionWithLineN("sun_intensity property belongs to sun block", Event); }
		const auto Sun = static_cast<YamlSun*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty intensity property", Event); }
			Sun->Intensity_ = ParseFloatEx(Event, Event->data.scalar.value, "sun_intensity", DMSSIM_MIN_ILLUMINATION_INTENSITY, DMSSIM_MAX_ILLUMINATION_INTENSITY);
		}
		return Sun;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_sun_temperature(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeSun) { ThrowExceptionWithLineN("sun_temperature property belongs to sun block", Event); }
		const auto Sun = static_cast<YamlSun*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty temperature property", Event); }
			Sun->Temperature_ = ParseFloatEx(Event, Event->data.scalar.value, "sun_temperature", DMSSIM_MIN_ILLUMINATION_TEMPERATURE, DMSSIM_MAX_ILLUMINATION_TEMPERATURE);
		}
		return Sun;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_camera(const yaml_event_t* Event, YamlObj* Obj, bool Enter){
		if (Obj) { ThrowExceptionWithLineN("Camera block must be on the base level", Event); }
		return &Camera_;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_spectrum(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN("spectrum property belongs to camera block", Event); }
		const auto Camera = static_cast<YamlCamera*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			bool NIR = false;
			if (strcmp(Value, "NIR") == 0) { NIR = true; }
			else if (strcmp(Value, "RGB") != 0) { ThrowExceptionWithLineN("Invalid spectrum option value of the camera. Must be \"NIR\" or \"RGB\"", Event); }
			Camera->NIR_ = NIR;
		}
		return Camera;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_resolution(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN("resolution property belongs to camera block", Event); }
		const auto Camera = static_cast<YamlCamera*>(Obj);
		if (!Enter) {
			Camera->ResolutionMark_ = Event->start_mark;
			const char* const ValueStr = reinterpret_cast<const char*>(Event->data.scalar.value);
			const int Dimension = atoi(ValueStr);
			if (Dimension <= 0 || Dimension > DMSSIM_MAX_RESOLUTION_DIMENSION) { ThrowExceptionWithLineN((std::string("Invalid frame resolution: ") + ValueStr).c_str(), Event); }
			Camera->Resolution_.push_back(Dimension);
		}
		return Camera;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_framerate(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN("framerate property belongs to camera block", Event); }
		const auto Camera = static_cast<YamlCamera*>(Obj);
		if (!Enter) {
			const char* const ValueStr = reinterpret_cast<const char*>(Event->data.scalar.value);
			const int FrameRate = atoi(ValueStr);
			if (FrameRate <= 0 || FrameRate > DMSSIM_MAX_FRAME_RATE) { ThrowExceptionWithLineN((std::string("Invalid frame rate: ") + ValueStr).c_str(), Event); }
			Camera_.FrameRate_ = FrameRate;
		}
		return Camera;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_bit_depth(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN("bit depth property belongs to camera block", Event); }
		const auto Camera = static_cast<YamlCamera*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			bool Depth16bit = false;
			if (strcmp(Value, "16bit") == 0) { Depth16bit = true; }
			else if (strcmp(Value, "8bit") != 0) { ThrowExceptionWithLineN("Invalid bit depth option value of the camera. Must be \"16bit\" or \"8bit\"", Event); }
			Camera_.Depth16Bit_ = Depth16bit;
		}
		return Camera;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_transform_base_to_project(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (Obj) { ThrowExceptionWithLineN("transform_base_to_project block must be on the base level", Event); }
		if (Enter) {
			CoordinateSpace_.Custom_ = true;
			return &CoordinateSpace_;
		}
		return nullptr;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_location(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!IsOrientationObj(Obj)) { ThrowExceptionWithLineN("location property does not belong to this block", Event); }
		const auto Orientation = static_cast<YamlOrientationObj*>(Obj);
		if (!Enter) {
			Orientation->LocationMark_ = Event->start_mark;
			Orientation->Location_.push_back(ParseFloat(Event, Event->data.scalar.value, "location"));
		}
		return Orientation;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_rotation(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!IsOrientationObj(Obj) && Obj->GetYamlType() != YamlObjTypeCoordinateSpace) { ThrowExceptionWithLineN("rotation property does not belong to this block", Event); }
		if (!Enter) {
			if (IsOrientationObj(Obj)) {
				const auto Orientation = static_cast<YamlOrientationObj*>(Obj);
				Orientation->RotationMark_ = Event->start_mark;
				Orientation->Rotation_.push_back(ParseFloat(Event, Event->data.scalar.value, "rotation"));
			}
			else {
				const auto CoordinateSpace = static_cast<DMSSimCoordinateSpaceObj*>(Obj);
				auto& Value = CoordinateSpace->CustomRotation_;
				Value.Mark_ = Event->start_mark;
				Value.Values_.push_back(ParseFloat(Event, Event->data.scalar.value, "rotation"));
			}
		}
		return Obj;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_translation(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCoordinateSpace) { ThrowExceptionWithLineN("translation property belongs to transform_base_to_project", Event); }
		const auto CoordinateSpace = static_cast<DMSSimCoordinateSpaceObj*>(Obj);
		if (!Enter) {
			auto& Value = CoordinateSpace->CustomTranslation_;
			Value.Mark_ = Event->start_mark;
			Value.Values_.push_back(ParseFloat(Event, Event->data.scalar.value, "translation"));
		}
		return CoordinateSpace;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_scale(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCoordinateSpace) { ThrowExceptionWithLineN("scale property belongs to transform_base_to_project", Event); }
		const auto CoordinateSpace = static_cast<DMSSimCoordinateSpaceObj*>(Obj);
		if (!Enter) {
			auto& Value = CoordinateSpace->CustomScale_;
			Value.Mark_ = Event->start_mark;
			const auto ValueFloat = ParseFloat(Event, Event->data.scalar.value, "scale");
			if (std::abs(ValueFloat) < DMSSIM_MIN_SCALE_VALUE) { ThrowExceptionWithLineN((std::string("scale modulo values must be greater than ") + std::to_string(DMSSIM_MIN_SCALE_VALUE)).c_str(), Event); }
			Value.Values_.push_back(ValueFloat);
		}
		return CoordinateSpace;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_mirrored(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN("mirrored property belongs to camera block", Event); }
		const auto Camera = static_cast<YamlCamera*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			bool Mirrored = false;
			if (strcmp(Value, "yes") == 0) { Mirrored = true; }
			else if (strcmp(Value, "no") != 0){ ThrowExceptionWithLineN("Invalid mirrored option value of the camera. Must be \"yes\" or \"no\"", Event); }
			Camera->Mirrored_ = Mirrored;
		}
		return Camera;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_fov_internal(const yaml_event_t* Event, YamlObj* Obj, bool Enter, const char* PropertyName, const std::function<void (float)>& Setter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN((std::string(PropertyName) + " property belongs to camera block").c_str(), Event); }
		const auto Camera = static_cast<YamlCamera*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN((std::string("Empty") + PropertyName + " property").c_str(), Event); }
			Setter(ParseFloatEx(Event, Event->data.scalar.value, PropertyName, DMSSIM_MIN_FOV, DMSSIM_MAX_FOV));
		}
		return Camera;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_fov(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		return EventHandler_fov_internal(Event, Obj, Enter, "fov", [Obj](float Value) { static_cast<YamlCamera*>(Obj)->FOV_ = Value; });
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_video_out(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN("video out property belongs to camera block", Event); }
		const auto Camera = static_cast<YamlCamera*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			bool VideoOut = false;
			if (strcmp(Value, "yes") == 0) { VideoOut = true; }
			else if (strcmp(Value, "no") != 0) { ThrowExceptionWithLineN("Invalid video out option value of the camera. Must be \"yes\" or \"no\"", Event); }
			Camera->VideoOut_ = VideoOut;
		}
		return Camera;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_csv_out(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN("csv out property belongs to camera block", Event); }
		const auto Camera = static_cast<YamlCamera*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			bool CsvOut = false;
			if (strcmp(Value, "yes") == 0) { CsvOut = true; }
			else if (strcmp(Value, "no") != 0) { ThrowExceptionWithLineN("Invalid csv out option value of the camera. Must be \"yes\" or \"no\"", Event); }
			Camera->CsvOut_ = CsvOut;
		}
		return Camera;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_noise(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN("noise property belongs to camera block", Event); }
		const auto Camera = static_cast<YamlCamera*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty noise property", Event); }
			Camera->Noise_ = ParseFloatEx(Event, Event->data.scalar.value, "noise", DMSSIM_MIN_NOISE, DMSSIM_MAX_NOISE);
		}
		return Camera;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_blur(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN("blur property belongs to camera block", Event); }
		const auto Camera = static_cast<YamlCamera*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty blur property", Event); }
			Camera->Blur_ = ParseFloatEx(Event, Event->data.scalar.value, "blur", DMSSIM_MIN_BLUR, DMSSIM_MAX_BLUR);
		}
		return Camera;
	}


	YamlObj* DMSSimScenarioParserImpl::EventHandler_grain_intensity(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN("grain intensity property belongs to camera block", Event); }
		const auto Camera = static_cast<YamlCamera*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty grain intensity", Event); }
			Camera->GrainIntensity_ = ParseFloatEx(Event, Event->data.scalar.value, "grain_intensity", 0.0, 10.0);
		}
		return Camera;
	}
	YamlObj* DMSSimScenarioParserImpl::EventHandler_grain_jitter(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN("grain jitter property belongs to camera block", Event); }
		const auto Camera = static_cast<YamlCamera*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty grain jitter", Event); }
			Camera->GrainJitter_ = ParseFloatEx(Event, Event->data.scalar.value, "grain_jitter", 0.0, 10.0);
		}
		return Camera;
	}
	YamlObj* DMSSimScenarioParserImpl::EventHandler_saturation(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN("saturation property belongs to camera block", Event); }
		const auto Camera = static_cast<YamlCamera*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty saturation", Event); }
			Camera->Saturation_ = ParseFloatEx(Event, Event->data.scalar.value, "saturation", 0.0, 10.0);
		}
		return Camera;
	}
	YamlObj* DMSSimScenarioParserImpl::EventHandler_gamma(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN("gamma property belongs to camera block", Event); }
		const auto Camera = static_cast<YamlCamera*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty gamma", Event); }
			Camera->Gamma_ = ParseFloatEx(Event, Event->data.scalar.value, "gamma", 0.0, 10.0);
		}
		return Camera;
	}
	YamlObj* DMSSimScenarioParserImpl::EventHandler_contrast(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN("contrast property belongs to camera block", Event); }
		const auto Camera = static_cast<YamlCamera*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty contrast", Event); }
			Camera->Contrast_ = ParseFloatEx(Event, Event->data.scalar.value, "contrast", 0.0, 10.0);
		}
		return Camera;
	}
	YamlObj* DMSSimScenarioParserImpl::EventHandler_bloom_intensity(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN("bloom intensity property belongs to camera block", Event); }
		const auto Camera = static_cast<YamlCamera*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty bloom intensity", Event); }
			Camera->BloomIntensity_ = ParseFloatEx(Event, Event->data.scalar.value, "bloom_intensity", 0.0, 10.0);
		}
		return Camera;
	}
	YamlObj* DMSSimScenarioParserImpl::EventHandler_focus_offset(const yaml_event_t * Event, YamlObj * Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN("focus offset property belongs to camera block", Event); }
		const auto Camera = static_cast<YamlCamera*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty focus offset", Event); }
			Camera->FocusOffset_ = ParseFloatEx(Event, Event->data.scalar.value, "focus_offset", -100.0, 100.0);
		}
		return Camera;
	}


	YamlObj* DMSSimScenarioParserImpl::EventHandler_min_fstop(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN("min fstop property belongs to camera block", Event); }
		const auto Camera = static_cast<YamlCamera*>(Obj);
		if (!Enter) { Camera->MinFStop_ = ParseFloatEx(Event, Event->data.scalar.value, "min fstop", DMSSIM_MIN_FSTOP, DMSSIM_MAX_FSTOP); }
		return Camera;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_max_fstop(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN("max fstop property belongs to camera block", Event); }
		const auto Camera = static_cast<YamlCamera*>(Obj);
		if (!Enter) { Camera->MaxFStop_ = ParseFloatEx(Event, Event->data.scalar.value, "max fstop", DMSSIM_MIN_FSTOP, DMSSIM_MAX_FSTOP); }
		return Camera;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_focal_distance(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN("focal distance property belongs to camera block", Event); }
		const auto Camera = static_cast<YamlCamera*>(Obj);
		if (!Enter) { Camera->FocalDistance_ = ParseFloatEx(Event, Event->data.scalar.value, "focal distance", DMSSIM_MIN_FOCAL_DISTANCE, DMSSIM_MAX_FOCAL_DISTANCE); }
		return Camera;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_ground_truth_settings(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (Obj) { ThrowExceptionWithLineN("ground truth settings block must be on the base level", Event); }
		return &GroundTruthSettings_;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_bounding_box_padding_factor_face(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeGroundTruthSettings) { ThrowExceptionWithLineN("face bounding box padding factor property belongs to ground truth settings block", Event); }
		const auto GroundTruthSettings = static_cast<YamlGroundTruthSettings*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty bounding box padding factor face", Event); }
			GroundTruthSettings->BoundingBoxPaddingFactor_Face_ = ParseFloatEx(Event, Event->data.scalar.value, "bounding_box_padding_factor_face", 0.0, 10.0);
		}
		return GroundTruthSettings;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_eye_bounding_box_width_factor(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeGroundTruthSettings) { ThrowExceptionWithLineN("eye_bounding_box_width_factor factor property belongs to ground truth settings block", Event); }
		const auto GroundTruthSettings = static_cast<YamlGroundTruthSettings*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty bounding box padding factor eyes", Event); }
			GroundTruthSettings->EyeBoundingBoxWidthFactor_ = ParseFloatEx(Event, Event->data.scalar.value, "eye_bounding_box_width_factor", 0.0, 10.0);
		}
		return GroundTruthSettings;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_eye_bounding_box_height_factor(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeGroundTruthSettings) { ThrowExceptionWithLineN("eye_bounding_box_height_factor factor property belongs to ground truth settings block", Event); }
		const auto GroundTruthSettings = static_cast<YamlGroundTruthSettings*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty bounding box padding factor eyes", Event); }
			GroundTruthSettings->EyeBoundingBoxHeightFactor_ = ParseFloatEx(Event, Event->data.scalar.value, "eye_bounding_box_height_factor", 0.0, 10.0);
		}
		return GroundTruthSettings;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_eye_bounding_box_depth(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeGroundTruthSettings) { ThrowExceptionWithLineN("eye_bounding_box_depth property belongs to ground truth settings block", Event); }
		const auto GroundTruthSettings = static_cast<YamlGroundTruthSettings*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty bounding box padding factor eyes", Event); }
			GroundTruthSettings->EyeBoundingBoxDepth_ = ParseFloatEx(Event, Event->data.scalar.value, "eye_bounding_box_depth", 0.0, 10.0);
		}
		return GroundTruthSettings;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_diaphragm_blade_count(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN("diaphragm blade count property belongs to camera block", Event); }
		const auto Camera = static_cast<YamlCamera*>(Obj);
		if (!Enter) { Camera->DiaphragmBladeCount_ = ParseIntEx(Event, Event->data.scalar.value, "diaphragm blade count", DMSSIM_MIN_DIAPHRAGM_BLADE_COUNT, DMSSIM_MAX_DIAPHRAGM_BLADE_COUNT); }
		return Camera;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_illumination(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN("illumination block belongs to camera block", Event); }
		return &Illumination_;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_intensity(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeIllumination) { ThrowExceptionWithLineN("intensity property belongs to illumination block", Event); }
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty intensity property", Event); }
			Illumination_.Intensity_ = ParseFloatEx(Event, Event->data.scalar.value, "intensity", DMSSIM_MIN_ILLUMINATION_INTENSITY, DMSSIM_MAX_ILLUMINATION_INTENSITY);
		}
		return &Illumination_;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_outer_cone_angle(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeIllumination) { ThrowExceptionWithLineN("outer_cone_angle property belongs to illumination block", Event); }
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty outer_cone_angle property", Event); }
			Illumination_.OuterConeAngle_ = ParseFloatEx(Event, Event->data.scalar.value, "outer_cone_angle", DMSSIM_MIN_ILLUMINATION_OUTER_CONE_ANGLE, DMSSIM_MAX_ILLUMINATION_OUTER_CONE_ANGLE);
		}
		return &Illumination_;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_attenuation_radius(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeIllumination) { ThrowExceptionWithLineN("attenuation_radius property belongs to illumination block", Event); }
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty attenuation_radius property", Event); }
			Illumination_.AttenuationRadius_ = ParseFloatEx(Event, Event->data.scalar.value, "attenuation_radius", DMSSIM_MIN_ILLUMINATION_ATTENUATION_RADIUS, DMSSIM_MAX_ILLUMINATION_ATTENUATION_RADIUS);
		}
		return &Illumination_;
	}
	YamlObj* DMSSimScenarioParserImpl::EventHandler_source_radius(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeIllumination) { ThrowExceptionWithLineN("source_radius property belongs to illumination block", Event); }
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty source_radius property", Event); }
			Illumination_.SourceRadius_ = ParseFloatEx(Event, Event->data.scalar.value, "source_radius", 0, 100);
		}
		return &Illumination_;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_steering_wheel_column(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCamera) { ThrowExceptionWithLineN("SteeringWheelColumn block belongs to camera block", Event); }
		return &SteeringWheelColumn_;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_is_camera_integrated(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeSteeringWheelColumn) { ThrowExceptionWithLineN("isCameraIntegrated property belongs to SteeringWheelColumn block", Event); }
		const auto SteeringWheelColumn = static_cast<YamlSteeringWheelColumn*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strcmp(Value, "yes") == 0) { SteeringWheelColumn->IsCameraIntegrated_ = true; }
			else if (strcmp(Value, "no") == 0) { SteeringWheelColumn->IsCameraIntegrated_ = false; }
			else { ThrowExceptionWithLineN("IsCameraIntegrated must be \"yes\" or \"no\"", Event); }
		}
		return SteeringWheelColumn;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_pitch_angle(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeSteeringWheelColumn) { ThrowExceptionWithLineN("PitchAngle property belongs to SteeringWheelColumn block", Event); }
		const auto SteeringWheelColumn = static_cast<YamlSteeringWheelColumn*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty column pitch angle", Event); }
			SteeringWheelColumn->PitchAngle_ = ParseFloatEx(Event, Event->data.scalar.value, "pitch_angle", DMSSIM_MIN_STEERING_WHEEL_COLUMN_PITCH_ANGLE, DMSSIM_MAX_STEERING_WHEEL_COLUMN_PITCH_ANGLE);
		}
		return SteeringWheelColumn;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_channels_parameters(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (Obj) { ThrowExceptionWithLineN("Channels parameters block must be on the base level", Event); }
		return &ChannelsParameters_;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_channels_blendout_defaults(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeChannelsParameters) { ThrowExceptionWithLineN("Blendout defaults block must be in the channels parameters block", Event); }
		if (Enter) {
			ChannelsParameters_.BlendOutParameters_.Defined_ = true;
			ChannelsParameters_.BlendOutParameters_.StartMark_ = Event->start_mark;
		}
		return &ChannelsParameters_.BlendOutParameters_;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_occupants(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (Obj) { ThrowExceptionWithLineN("occupants block must be on the base level", Event); }
		return &Occupants_;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_OccupantInternal(const yaml_event_t* Event, YamlObj* Obj, bool Enter, FDMSSimOccupantType Type, const char* Name) {
		if (Obj) {
			switch (Obj->GetYamlType()) {
			case YamlObjTypeOccupants:
				{
					const auto Occupants = static_cast<YamlOccupants*>(Obj);
					if (Enter) {
						if (IsOccupantExits(Type)) { ThrowExceptionWithLineN((std::string("There can only be one ") + Name + " occupant in the car").c_str(), Event); }
						YamlOccupant Occupant(Type);
						Occupants->Occupants_.push_back(Occupant);
					}
					return &Occupants->Occupants_.back();
				}
				break;

			case YamlObjTypeScenario:
				{
					const auto Scenario = static_cast<YamlScenario*>(Obj);
					if (Enter) {
						if (IsOccupantScenarioExits(Type)) { ThrowExceptionWithLineN((std::string("There can only be one ") + Name + " occupant in the scenario").c_str(), Event); }
						YamlOccupantScenario Occupant(Type);
						Scenario->Occupants_.push_back(Occupant);
					}
					return &Scenario->Occupants_.back();
				}
				break;

			case YamlObjTypeOccupantScenario:
				return nullptr;
			}
		}

		ThrowExceptionWithLineN((std::string("[") + Name + "] property belongs to the \"occupants\" or \"scenario\" sections").c_str(), Event);
		return nullptr;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_character(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("character property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty character property", Event); }
			Occupant->Character_.assign(Value);
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_headgear(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("headgear property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty headgear property", Event); }
			if (strcmp(Value, DMSSIM_TOKEN_NONE) != 0) { Occupant->Headgear_.assign(Value); }
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_upper_cloth(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("upper cloth property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty upper cloth property", Event); }
			if (strcmp(Value, DMSSIM_TOKEN_NONE) != 0) { Occupant->UpperCloth_.assign(Value); }
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_glasses(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("glasses property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty glasses property", Event); }
			if (strcmp(Value, DMSSIM_TOKEN_NONE) != 0) { Occupant->Glasses_.assign(Value); }
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_glasses_color(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("glasses color property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty glasses color property", Event); }
			Occupant->GlassesColorMark_ = Event->start_mark;
			Occupant->GlassesColor_.push_back(ParseFloatEx(Event, Event->data.scalar.value, "glasses color", 0.0f, 1.0f));
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_glasses_opacity(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("glasses opacity property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty glasses opacity property", Event); }
			Occupant->GlassesOpacity_ = ParseFloatEx(Event, Event->data.scalar.value, "glasses opacity", DMSSIM_MIN_GLASSES_OPACITY, DMSSIM_MAX_GLASSES_OPACITY);
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_glasses_reflective(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("glasses reflective property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			Occupant->GlassesReflective_ = true;
			if (strcmp(Value, "no") == 0) { Occupant->GlassesReflective_ = false; }
			else if (strcmp(Value, "yes") != 0) { ThrowExceptionWithLineN("glasses reflective must be \"yes\" or \"no\"", Event); }
		}
		return Occupant;
	}
	YamlObj* DMSSimScenarioParserImpl::EventHandler_mask(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("mask property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty mask property", Event); }
			if (strcmp(Value, DMSSIM_TOKEN_NONE) != 0) { Occupant->Mask_.assign(Value); }
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_scarf(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("scarf property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty scarf property", Event); }
			if (strcmp(Value, DMSSIM_TOKEN_NONE) != 0) { Occupant->Scarf_.assign(Value); }
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_hair(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("hair property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty hair property", Event); }
			if (strcmp(Value, DMSSIM_TOKEN_DEFAULT) != 0) { Occupant->Hair_.assign(Value); }
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_beard(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("beard property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty beard property", Event); }
			if (strcmp(Value, DMSSIM_TOKEN_NONE) != 0) { Occupant->Beard_.assign(Value); }
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_mustache(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("mustache property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty mustache property", Event); }
			if (strcmp(Value, DMSSIM_TOKEN_NONE) != 0) { Occupant->Mustache_.assign(Value); }
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_pupil_size(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("pupil size property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty pupil size property", Event); }
			const float PupilSize = ParseFloat(Event, Event->data.scalar.value, "pupil_size");
			Occupant->PupilSize_ = PupilSize;
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_pupil_brightness(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("pupil brightness property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty pupil brightness property", Event); }
			const float PupilBrightness = ParseFloat(Event, Event->data.scalar.value, "pupil_brightness");
			Occupant->PupilBrightness_ = PupilBrightness;
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_iris_size(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("iris size property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty iris size property", Event); }
			const float IrisSize = ParseFloat(Event, Event->data.scalar.value, "iris_size");
			Occupant->IrisSize_ = IrisSize;
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_iris_brightness(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("iris brightness property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty iris brightness property", Event); }
			const float IrisBrightness = ParseFloat(Event, Event->data.scalar.value, "iris_brightness");
			Occupant->IrisBrightness_ = IrisBrightness;
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_limbus_dark_amount(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("limbus dark amount property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty limbus dark amount property", Event); }
			const float LimbusDarkAmount = ParseFloat(Event, Event->data.scalar.value, "limbus_dark_amount");
			Occupant->LimbusDarkAmount_ = LimbusDarkAmount;
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_iris_color(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("iris color property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty iris color property", Event); }
			if (strcmp(Value, DMSSIM_TOKEN_NONE) != 0) { Occupant->IrisColor_.assign(Value); }
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_iris_border_width(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("iris border width property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty iris border width property", Event); }
			const float IrisBorderWidth = ParseFloat(Event, Event->data.scalar.value, "iris_border_width");
			Occupant->IrisBorderWidth_ = IrisBorderWidth;
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_sclera_brightness(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("sclera brightness property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty sclera brightness property", Event); }
			const float ScleraBrightness = ParseFloat(Event, Event->data.scalar.value, "sclera_brightness");
			Occupant->ScleraBrightness_ = ScleraBrightness;
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_sclera_veins(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("sclera veins property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty sclera veins property", Event); }
			const float ScleraVeius = ParseFloat(Event, Event->data.scalar.value, "sclera_veins");
			Occupant->ScleraVeins_ = ScleraVeius;
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_skin_wrinkles(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("skin wrinkles property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty skin wrinkles property", Event); }
			const float SkinWrinkles = ParseFloat(Event, Event->data.scalar.value, "skin_wrinkles ");
			Occupant->SkinWrinkles_ = SkinWrinkles;
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_skin_roughness(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("skin roughness property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty skin roughness property", Event); }
			const float SkinRoughness = ParseFloat(Event, Event->data.scalar.value, "skin_roughness ");
			Occupant->SkinRoughness_ = SkinRoughness;
		}
		return Occupant;
	}


	YamlObj* DMSSimScenarioParserImpl::EventHandler_skin_specularity(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("skin specularity property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty skin specularity property", Event); }
			const float SkinSpecularity = ParseFloat(Event, Event->data.scalar.value, "skin_specularity ");
			Occupant->SkinSpecularity_ = SkinSpecularity;
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_height(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("height property belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty height property", Event); }
			Occupant->Height_ = ParseFloat(Event, Event->data.scalar.value, "height");
		}
		return Occupant;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_seat(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeOccupant) { ThrowExceptionWithLineN("seat sub-section belongs to the \"driver/passenger\" sections", Event); }
		const auto Occupant = static_cast<YamlOccupant*>(Obj);
		return &Occupant->Seat_;
	}

	template <class TSelector>
	YamlObj* DMSSimScenarioParserImpl::EventHandler_offset_internal(const yaml_event_t* Event, YamlObj* Obj, bool Enter, const char* ParamName, TSelector Selector){
		if (!Obj || Obj->GetYamlType() != YamlObjTypeSeat) { ThrowExceptionWithLineN((std::string(ParamName) + " property belongs to the \"seat\" sections").c_str(), Event); }
		const auto Seat = static_cast<YamlSeat*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN((std::string("Empty ") + ParamName + " property").c_str(), Event); }
			const float Offset = ParseFloat(Event, Event->data.scalar.value, ParamName);
			Selector(Seat->Offset_) = Offset;
		}
		return Seat;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_animations(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (Obj) { ThrowExceptionWithLineN("animations block must be on the base level", Event); }
		return &Animations_;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_name(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Enter) {
			if (!Obj || Obj->GetYamlType() != YamlObjTypeAnimationSequence) { ThrowExceptionWithLineN("name property belongs to animation sequence block", Event); }
			const auto Sequence = static_cast<YamlAnimationSequence*>(Obj);
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty animation sequence name property", Event); }
			Sequence->Name_.assign(Value);
			return Sequence;
		}
		else {
			if (!Obj || Obj->GetYamlType() != YamlObjTypeAnimations) { ThrowExceptionWithLineN("name property belongs to animations block", Event); }
			const auto Animations = static_cast<YamlAnimations*>(Obj);
			YamlAnimationSequence Sequence;
			Sequence.StartMark_ = Event->start_mark;
			Animations->Sequences_.push_back(Sequence);
			return &Animations->Sequences_.back();
		}
		return nullptr;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_type(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (Obj) {
			switch (Obj->GetYamlType()) {
			case YamlObjTypeAnimations:
				if (Enter) {
					const auto Animations = static_cast<YamlAnimations*>(Obj);
					if (Animations->Sequences_.empty()) { ThrowExceptionWithLineN("[type] belongs to some animation", Event); }
					return &Animations->Sequences_.back();
				}
				break;
			case YamlObjTypeAnimationSequence:
				if (!Enter) {
					const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
					if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty animation type property", Event); }
					const auto Sequence = static_cast<YamlAnimationSequence*>(Obj);
					if (Sequence->GetType() != DMSSimAnimationUnknown) { ThrowExceptionWithLineN("[type] already set", Event); }
					const struct AnimationTypes {
						const char*         Name;
						DMSSimAnimationType Type;
					}
					Types[] = {
						{ "gaze_direction", DMSSimAnimationParametricGazeDirection },
						{ "head_rotation",  DMSSimAnimationParametricHeadRotation  },
						{ "eye_opening",    DMSSimAnimationParametricEyeOpening    },
						{ "steering_wheel", DMSSimAnimationParametricSteeringWheel },
					};

					for (const auto& Type : Types) {
						if (strcmp(Type.Name, Value) == 0) {
							Sequence->Type_ = Type.Type;
							break;
						}
					}

					if (Sequence->GetType() == DMSSimAnimationUnknown) { ThrowExceptionWithLineN((std::string("Unknown animation type: ") + Value).c_str(), Event); }
					return Obj;
				}
				break;
			}
		}
		ThrowExceptionWithLineN("[type] block belongs to animations block", Event);
		return nullptr;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_parameter(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (Obj) {
			switch (Obj->GetYamlType()) {
			case YamlObjTypeAnimationSequence:
				if (Enter) {
					const auto Sequence = static_cast<YamlAnimationSequence*>(Obj);
					if (Sequence->WithParameters_) {
						YamlMotion Motion;
						Motion.Type_ = DMSSimMotionParametric;
						Motion.StartMark_ = Event->start_mark;
						Sequence->Motions_.push_back(Motion);
						return &Sequence->Motions_.back();
					}
				}
				break;

			case YamlObjTypeMotion:
				if (!Enter) {
					const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
					if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty animation parameter property", Event); }
					const char* const Parameters[] = {
						DMSSIM_PARAMETER_GAZE_DIRECTION_POINT,
						DMSSIM_PARAMETER_HEAD_DIRECTION_POINT,
						DMSSIM_PARAMETER_EYE_OPENING_REL_VECTOR,
						DMSSIM_PARAMETER_STEERING_WHEEL,
					};

					for (const auto Parameter : Parameters) {
						if (strcmp(Parameter, Value) == 0) {
							const auto Motion = static_cast<YamlMotion*>(Obj);
							Motion->Animation_ = Value;
							return Motion;
						}
					}
					ThrowExceptionWithLineN((std::string("Unsupported parameter: ") + Value).c_str(), Event);
				}
				break;
			}
		}
		ThrowExceptionWithLineN("[parameter] block belongs to parameters block", Event);
		return nullptr;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_parameters(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (Obj) {
			switch (Obj->GetYamlType()) {
			case YamlObjTypeAnimations:
				if (Enter) {
					const auto Animations = static_cast<YamlAnimations*>(Obj);
					if (Animations->Sequences_.empty()) { ThrowExceptionWithLineN("[parameters] belongs to some animation", Event); }
					auto& Animation = Animations->Sequences_.back();
					if (Animation.WithParameters_) { ThrowExceptionWithLineN("Recursive [parameters] block", Event); }
					if (Animation.GetType() == DMSSimAnimationUnknown) { ThrowExceptionWithLineN("Animation type not set", Event); }
					Animation.WithParameters_ = true;
					return &Animation;
				}
				break;
			}
		}
		ThrowExceptionWithLineN("[parameters] block belongs to animations block", Event);
		return nullptr;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_points(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (Obj) {
			switch (Obj->GetYamlType()) {
			case YamlObjTypeAnimationSequence:
				const auto Sequence = static_cast<YamlAnimationSequence*>(Obj);
				return &Sequence->Motions_.back();
			}
		}
		ThrowExceptionWithLineN("[points] block belongs to animations block", Event);
		return nullptr;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_time(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeAnimationPoint) { ThrowExceptionWithLineN("time property belongs to a point block", Event); }
		if (!Enter) {
			const auto AnimationPoint = static_cast<YamlAnimationPoint*>(Obj);
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty time property", Event); }
			AnimationPoint->Time_ = ParseFloatEx(Event, Event->data.scalar.value, "time", 0.0f, DMSSIM_MAX_ANIMATION_DURATION);
		}
		return Obj;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_value(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeAnimationPoint) { ThrowExceptionWithLineN("value property belongs to a point block", Event); }
		if (!Enter) {
			const auto AnimationPoint = static_cast<YamlAnimationPoint*>(Obj);
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN("Empty value property", Event); }
			AnimationPoint->Values_.push_back(ParseFloatEx(Event, Event->data.scalar.value, "point", DMSSIM_MIN_CURVE_VALUE, DMSSIM_MAX_CURVE_VALUE));
		}
		return Obj;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_sequence(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (Obj) {
			switch (Obj->GetYamlType()) {
			case YamlObjTypeAnimations:
				return &static_cast<YamlAnimations*>(Obj)->Sequences_.back();
			case YamlObjTypeOccupantScenario:
				return static_cast<YamlOccupantScenario*>(Obj);
				break;
			}
		}
		ThrowExceptionWithLineN("sequence property belongs to animations or driver/passenger blocks", Event);
		return nullptr;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_motion(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (Obj) {
			switch (Obj->GetYamlType()) {
			case YamlObjTypeAnimationSequence:
				if (Enter) {
					const auto Sequence = static_cast<YamlAnimationSequence*>(Obj);
					YamlMotion Motion;
					Motion.StartMark_ = Event->start_mark;
					Sequence->Motions_.push_back(Motion);
					return &Sequence->Motions_.back();
				}
				break;
			case YamlObjTypeOccupantScenario:
				if (Enter) {
					const auto OccupantScenario = static_cast<YamlOccupantScenario*>(Obj);
					YamlMotion Motion;
					Motion.StartMark_ = Event->start_mark;
					OccupantScenario->Motions_.push_back(Motion);
					return &OccupantScenario->Motions_.back();
				}
				break;
			case YamlObjTypeMotion:
				if (!Enter) { return Obj; }
				break;
			}
		}
		ThrowExceptionWithLineN("[motion] block belongs to sequence/scenario block", Event);
		return nullptr;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_animation_internal(const yaml_event_t* Event, YamlObj* Obj, bool Enter, bool Pause) {
		const char* const Parameter = Pause ? "pause" : "animation";
		const auto ParentType = Obj->GetYamlType();
		if (!Obj || ParentType != YamlObjTypeMotion) {
			if (ParentType != YamlObjTypeAnimationChannel || !Enter) { ThrowExceptionWithLineN((std::string(Parameter) + " block belongs to a motion block or an animation channel").c_str(), Event); }
		}
		if (ParentType == YamlObjTypeAnimationChannel && Enter) {
			const auto Channel = static_cast<YamlAnimationChannel*>(Obj);
			return &Channel->Motions_.back();
		}
		if (ParentType != YamlObjTypeMotion) { ThrowExceptionWithLineN((std::string(Parameter) + " block belongs to a motion block").c_str(), Event); }

		const auto Motion = static_cast<YamlMotion*>(Obj);
		if (!Enter) {
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN((std::string("Empty ") + Parameter + " property").c_str(), Event); }
			if (Pause) {
				if (strcmp(Value, "active") == 0) { Motion->Type_ = DMSSimMotionPauseActive; }
				else if (strcmp(Value, "passive") == 0) { Motion->Type_ = DMSSimMotionPausePassive; }
				else { ThrowExceptionWithLineN((std::string("Unexpected pause type: ") + Value).c_str(), Event); }
			}
			else { Motion->Animation_ = Value; }
		}
		return Motion;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_animation_parameter(const yaml_event_t* Event, YamlObj* Obj, bool Enter, const char* const Parameter, bool NoPause, const std::function<void(YamlMotion*, float)>& Setter, const float MinValue, const float MaxValue) {
		if (Obj && Obj->GetYamlType() == YamlObjTypeAnimationChannel && Enter) {
			const auto Channel = static_cast<YamlAnimationChannel*>(Obj);
			auto& Motions = Channel->Motions_;
			if (Motions.empty()) { ThrowExceptionWithLineN((std::string(Parameter) + " block parsing unexpected error").c_str(), Event); }
			return &Motions.back();
		}
		if (!Obj || Obj->GetYamlType() != YamlObjTypeMotion) { ThrowExceptionWithLineN((std::string(Parameter) + " block belongs to motion or animation block").c_str(), Event); }
		const auto Motion = static_cast<YamlMotion*>(Obj);
		if (!Enter) {
			if (Motion->GetType() != DMSSimMotionAnimation && NoPause) { ThrowExceptionWithLineN((std::string("Parameter ") + Parameter + " is not supported on active/passive pause").c_str(), Event); }
			const auto Value = reinterpret_cast<const char*>(Event->data.scalar.value);
			if (strlen(Value) == 0) { ThrowExceptionWithLineN((std::string("Empty ") + Parameter + " property").c_str(), Event); }
			Setter(Motion, ParseFloatEx(Event, Event->data.scalar.value, Parameter, MinValue, MaxValue));
		}
		return Motion;
	}

	void DMSSimScenarioParserImpl::CheckAnimationRange(const yaml_event_t* const Event, const YamlObj* const Obj, bool Enter) {
		if (!Enter && Obj && Obj->GetYamlType() == YamlObjTypeMotion) {
			const auto Motion = static_cast<const YamlMotion*>(Obj);
			const float StartPos = Motion->GetStartPos();
			const float EndPos = Motion->GetEndPos();
			if (EndPos >= 0 && (StartPos - EndPos) > -DMSSIM_MIN_ANIMATION_DURATION) { ThrowExceptionWithLineN("Animation Start Position greater than End Position", Event); }
		}
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_start_pos(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		const auto NewObj = EventHandler_animation_parameter(Event, Obj, Enter, "start_pos", true, [](YamlMotion* Motion, float Value) { Motion->StartPos_ = Value; }, 0, DMSSIM_MAX_ANIMATION_DURATION);
		CheckAnimationRange(Event, NewObj, Enter);
		return NewObj;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_end_pos(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		const auto NewObj = EventHandler_animation_parameter(Event, Obj, Enter, "end_pos", true, [](YamlMotion* Motion, float Value) { Motion->EndPos_ = Value; }, DMSSIM_MIN_ANIMATION_DURATION, DMSSIM_MAX_ANIMATION_DURATION);
		CheckAnimationRange(Event, NewObj, Enter);
		return NewObj;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_animation_channel(const yaml_event_t* Event, YamlObj* Obj, bool Enter, DMSSimAnimationChannelType Type) {
		const char* const ChannelName = GetChannelName(Type);
		if (!Obj || (Obj->GetYamlType() != YamlObjTypeOccupantScenario && Obj->GetYamlType() != YamlObjTypeDefaultBlendOutParameters)) { ThrowExceptionWithLineN((std::string(ChannelName) + " block belongs to sequence / channels_blendout_defaults block").c_str(), Event); }
		const auto ObjType = Obj->GetYamlType();
		switch (ObjType) {
		case YamlObjTypeOccupantScenario: {
			const auto Scenario = static_cast<YamlOccupantScenario*>(Obj);
			if (Enter) {
				if (Scenario->GetType() != FDMSSimOccupantType::Driver && Type == DMSSimAnimationChannelSteeringWheel) { ThrowExceptionWithLineN("Steering Wheel channel is supported only for the driver.", Event); }
				for (const auto& C : Scenario->Channels_) { if (C.GetType() == Type) { ThrowExceptionWithLineN((std::string("Duplicate animation channel: ") + ChannelName).c_str(), Event); } }
				YamlAnimationChannel Channel;
				Channel.Type_ = Type;
				Scenario->Channels_.push_back(Channel);
			}
			return &Scenario->Channels_.back();
		}
		break;
		case YamlObjTypeDefaultBlendOutParameters: {
			if (!Enter) {
				const auto Parameters = static_cast<YamlBlendOutParameters*>(Obj);
				const float Value = ParseFloatEx(Event, Event->data.scalar.value, ChannelName, DMSSIM_MIN_BLEND_OUT, DMSSIM_MAX_BLEND_OUT);
				Parameters->Parameters_[Type] = Value;
			}
		}
		break;
		}
		return Obj;
	}

	YamlObj* DMSSimScenarioParserImpl::EventHandler_scenario(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (Obj) { ThrowExceptionWithLineN("scenario block must be on the base level", Event); }
		return &Scenario_;
	}

	void DMSSimScenarioParserImpl::ValidateParameters(const DMSSimConfigParser& Config) {
		const auto CoordinateSpace = GetCoordinateSpace(Config);
		CoordinateSpace_.Recompute();
		if (!CoordinateSpace_.Custom_) {
			CoordinateSpace_.CarModel_ = CoordinateSpace->GetCarModel();
			CoordinateSpace_.Rotation_ = CoordinateSpace->GetRotation();
			CoordinateSpace_.Translation_ = CoordinateSpace->GetTranslation();
			CoordinateSpace_.Scale_ = CoordinateSpace->GetScale();
		}
		CoordinateSpace_.Rotation_ = CoordinateSpace_.Rotation_ * static_cast<float>(180.0f / PI);
		Sun_.Recompute(CoordinateSpace_);
		Camera_.Recompute(CoordinateSpace_);
		if (Illumination_.Location_.empty()) { Illumination_.Location_ = Camera_.Location_; }
		if (Illumination_.Rotation_.empty()) { Illumination_.Rotation_ = Camera_.Rotation_; }
		Illumination_.Recompute(CoordinateSpace_);
		for (const auto& Occupant : Occupants_.Occupants_) { Occupant.Validate(); }
		ChannelsParameters_.Validate();
		Animations_.Validate();
		Animations_.RecomputePoints(CoordinateSpace_);
		for (const auto& Occupant : Scenario_.Occupants_) { Occupant.Validate(); }
	}

	float DMSSimScenarioParserImpl::GetDefaultBlendOut(DMSSimAnimationChannelType Channel) const {
		assert(Channel < DMSSimAnimationChannelCount);
		return ChannelsParameters_.BlendOutParameters_.Parameters_[Channel];
	}

	const DMSSimCoordinateSpace* DMSSimScenarioParserImpl::GetCoordinateSpace(const DMSSimConfigParser& Config) const {
		const DMSSimCoordinateSpace* DefaultCoordinateSpace = nullptr;
		const size_t CoordinateSpaceCount = Config.GetCoordinateSpaceCount();
		for (size_t i = 0; i < CoordinateSpaceCount; ++i) {
			const auto& CoordinateSpace = Config.GetCoordinateSpace(i);
			const auto CarName = CoordinateSpace.GetCarModel();
			if (strcmp(CarName, Car_.Model_.c_str()) == 0) { return &CoordinateSpace; }
			else if (strcmp(CarName, DMSSIM_DEFAULT_COORDINATE_SPACE_NAME) == 0) { DefaultCoordinateSpace = &CoordinateSpace; }
		}
		if (DefaultCoordinateSpace == nullptr) { throw std::exception((std::string("Unsupported coordinate space ") + Car_.Model_).c_str()); }
		return DefaultCoordinateSpace;
	}
} // anonymous namespace

DMSSimScenarioParser* DMSSimScenarioParser::Create(const wchar_t* const FilePath, const DMSSimConfigParser& Config) {
	auto Parser = std::make_unique<DMSSimScenarioParserImpl>();
	if (Parser->Initialize(FilePath, Config)) { return Parser.release(); }
	return 	nullptr;
}

const DMSSimCoordinateSpace& DMSSimScenarioParser::GetDefaultCoordinateSpace() { return DefaultCoordinateSpaceObj; }

const DMSSimCamera& DMSSimScenarioParser::GetDefaultCamera() { return DefaultCamera; }

DMSSimBaseAnimationType DMSSimScenarioParser::GetChannelBaseAnimationType(DMSSimAnimationChannelType Channel) {
	switch (Channel) {
	case DMSSimAnimationChannelEyeGaze:
	case DMSSimAnimationChannelEyelids:
	case DMSSimAnimationChannelFace:
	case DMSSimAnimationChannelHead:
		return DMSSimBaseAnimationHead;
	case DMSSimAnimationChannelUpperBody:
	case DMSSimAnimationChannelLeftHand:
	case DMSSimAnimationChannelRightHand:
		return DMSSimBaseAnimationBody;
	};
	return DMSSimBaseAnimationUnknown;
}