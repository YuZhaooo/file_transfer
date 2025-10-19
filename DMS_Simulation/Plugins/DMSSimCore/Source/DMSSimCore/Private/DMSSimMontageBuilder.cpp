#include "DMSSimMontageBuilder.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <string>
#include <sstream>
#include <vector>

namespace DMSSimMontageBuilder
{
namespace
{
constexpr float PASSIVE_PAUSE_DELTA = 0.01f;
constexpr float INFINITE_PAUSE_DURATION = 1000000000.0f;

class DMSSimAnimationPointImpl : public DMSSimAnimationPoint {
public:
	DMSSimAnimationPointImpl(const TAnimationPoint& Point)
		: Point_(Point)
	{
	}
	virtual float GetTime() const
	{
		return Point_.Time;
	}

	virtual FVector GetPoint() const
	{
		return Point_.Point;
	}
private:
	TAnimationPoint Point_;
};

struct TAnimationInfo
{
	TAnimationInfo()
	{}

	TAnimationInfo(const char* Name, UAnimSequenceBase* const AnimSequence, const float Length, const float EndPos)
		: Name_(Name), AnimSequence_(AnimSequence), Length_(Length), EndPos_(EndPos)
	{
	}

	TAnimationInfo(const TArray<TAnimationPoint>& Points, const float EndPos)
		: EndPos_(EndPos), Points_(Points)
	{
	}
	std::string             Name_;
	UAnimSequenceBase*      AnimSequence_ = nullptr;
	float                   Length_ = 0;
	float                   EndPos_ = 0;
	TArray<TAnimationPoint> Points_;
};

class InfinitePause : public DMSSimMotion {
public:
	virtual DMSSimMotionType GetType() const override { return DMSSimMotionPausePassive; }
	virtual const char* GetAnimationName() const override { return nullptr; }
	virtual size_t GetPointCount() const { return 0; }
	virtual const DMSSimAnimationPoint& GetPoint(size_t Index) const { return *((DMSSimAnimationPoint*)nullptr); }
	virtual float GetStartPos() const override { return 0; }
	virtual float GetEndPos() const override { return 0; }
	virtual float GetDuration() const override { return INFINITE_PAUSE_DURATION; }
	virtual float GetBlendOut() const override { return 0.0f; }
};

class DMSSimAnimationChannelProxy : public DMSSimAnimationChannel {
public:
	DMSSimAnimationChannelProxy(const DMSSimAnimationChannel* const Channel)
		: Channel_(Channel) {}

	virtual DMSSimAnimationChannelType GetType() const override
	{
		return DMSSimAnimationChannelCommon;
	}

	virtual size_t GetMotionCount() const override
	{
		return (Channel_ ? Channel_->GetMotionCount() : 0) + 1;
	}

	virtual const DMSSimMotion& GetMotion(size_t MotionIndex) const override
	{
		if (Channel_ && MotionIndex < Channel_->GetMotionCount())
		{
			return Channel_->GetMotion(MotionIndex);
		}
		return Pause_;
	}
private:
	const DMSSimAnimationChannel* Channel_;
	InfinitePause                 Pause_;
};

class DMSSimCustomMotion : public DMSSimMotion {
public:
	DMSSimCustomMotion(
		const DMSSimMotionType Type,
		const char* AnimationName,
		const float StartPos,
		const float EndPos,
		const float Duration,
		const float BlendOut)
		: Type_(Type), AnimationName_(AnimationName), StartPos_(StartPos), EndPos_(EndPos), Duration_(Duration), BlendOut_(BlendOut)
	{
	}

	virtual DMSSimMotionType GetType() const override
	{
		return Type_;
	}

	virtual const char* GetAnimationName() const
	{
		return AnimationName_.c_str();
	}

	virtual size_t GetPointCount() const
	{
		return 0;
	}

	virtual const DMSSimAnimationPoint& GetPoint(size_t Index) const
	{
		return *((const DMSSimAnimationPoint*)nullptr);
	}

	virtual float GetStartPos() const override
	{
		return StartPos_;
	}

	virtual float GetEndPos() const override
	{
		return EndPos_;
	}

	virtual float GetDuration() const override
	{
		return Duration_;
	}

	virtual float GetBlendOut() const override
	{
		return BlendOut_;
	}

private:
	DMSSimMotionType                 Type_ = DMSSimMotionAnimation;
	std::string                      AnimationName_;
	float                            StartPos_ = 0;
	float                            EndPos_ = 0;
	float                            Duration_ = 0;
	float                            BlendOut_ = 0;
};

std::string StringToLower(const char* const Name)
{
	std::string NameStr(Name);
	std::transform(
		NameStr.begin(),
		NameStr.end(),
		NameStr.begin(),
		[](unsigned char c) { return std::tolower(c); });
	return NameStr;
}

const char* FindAnimationInternal(const DMSSimAssetRegistry* const  AssetRegistry, const char* const Name, const DMSSimResourceSet& AnimationSet)
{
	if (!Name)
	{
		return nullptr;
	}

	std::string NameStr(StringToLower(Name));
	if (NameStr.empty())
	{
		return nullptr;
	}

	const size_t AnimationCount = AnimationSet.GetResourceCount();
	const char* AnimationPath = nullptr;
	for (size_t i = 0; i < AnimationCount; ++i)
	{
		if (strcmp(NameStr.c_str(), AnimationSet.GetResourceName(i)) == 0)
		{
			AnimationPath = AnimationSet.GetResourcePath(i);
			break;
		}
	}
	return AnimationPath;
}

const char* FindAnimation(const DMSSimAssetRegistry* const  AssetRegistry, const char* const Name)
{
	const auto& AnimationSet = AssetRegistry->GetAnimations();
	return FindAnimationInternal(AssetRegistry, Name, AnimationSet);
}

std::string GetAnimationName(
	const DMSSimAnimationChannelType  TargetChannel,
	const DMSSimMotion& Motion,
	const DMSSimAssetRegistry* const AssetRegistry)
{
	std::string AnimName(Motion.GetAnimationName());
	const auto AnimationType = DMSSimScenarioParser::GetChannelBaseAnimationType(TargetChannel);
	if (AnimationType == DMSSimBaseAnimationHead)
	{
		AnimName += DMSSIM_HEAD_ANIMATION_POSTFIX;
		if (FindAnimation(AssetRegistry, AnimName.c_str()))
		{
			return AnimName;
		}
	}
	return Motion.GetAnimationName();
}

bool AddMontageSlotSegmentAnimation(
	TEnvironment&                     Environment,
	const DMSSimAnimationChannelType  TargetChannel,
	const DMSSimMotion&               Motion,
	const DMSSimAssetRegistry* const  AssetRegistry,
	FSlotAnimationTrack* const        Slot,
	const float                       ChannelStartPos,
	const float                       ChannelDuration,
	const bool                        FirstMontage,
	TAnimationInfo&                   PrevAnimation,
	float&                            AnimLengthAdj,
	float&                            AnimLengthFull)
{
	bool Result = true;
	const std::string AnimationName = GetAnimationName(TargetChannel, Motion, AssetRegistry);

	const char* AnimationPath = FindAnimation(AssetRegistry, AnimationName.c_str());
	if (!AnimationPath)
	{
		throw std::exception((std::string("Animation ") + AnimationName + " not found!").c_str());
	}

	bool ExactMatch = false;
	UAnimSequenceBase* const AnimSequence = Environment.LoadAnimationSequence(TargetChannel, AnimationName.c_str(), AnimationPath, &ExactMatch);
	if (!AnimSequence)
	{
		throw std::exception((std::string("Failed to load ") + AnimationName + " animation sequence!").c_str());
	}

	const float AnimLength = AnimSequence->GetPlayLength();

	float StartPos = std::min(Motion.GetStartPos(), AnimLength);
	float EndPos = AnimLength;
	if (Motion.GetEndPos() > StartPos)
	{
		EndPos = std::min(Motion.GetEndPos(), AnimLength);
	}
	float PlayRate = 1.0f;
	const float Duration = Motion.GetDuration();

	AnimLengthAdj = EndPos - StartPos;
	if (Duration > 0.0f)
	{
		if (AnimLengthAdj > 0.0f)
		{
			PlayRate = AnimLengthAdj / Duration;
		}
		AnimLengthAdj = Duration;
	}
	AnimLengthFull = AnimLengthAdj;

	if (ChannelDuration > 0)
	{
		if (ChannelStartPos < AnimLengthAdj && (ChannelStartPos + ChannelDuration) > 0)
		{
			const bool CommonAnimationLonger = (ChannelStartPos + ChannelDuration) < AnimLengthAdj;
			if (CommonAnimationLonger)
			{
				AnimLengthAdj = (ChannelStartPos + ChannelDuration);
				EndPos = StartPos + AnimLengthAdj * PlayRate;
			}
			AnimLengthFull = std::max(AnimLengthAdj, AnimLengthFull);
			if (ChannelStartPos > 0)
			{
				StartPos += ChannelStartPos * PlayRate;
				AnimLengthAdj -= ChannelStartPos;
			}
		}
		else
		{
			Result = false;
		}
	}

	if (ExactMatch)
	{
		FAnimSegment Segment;
		Segment.AnimReference = AnimSequence;
		Segment.AnimStartTime = StartPos;
		Segment.AnimEndTime = EndPos;
		Segment.AnimPlayRate = PlayRate;
		Segment.LoopingCount = 1;
		Segment.StartPos = 0;
		Slot->AnimTrack.AnimSegments.Add(Segment);
	}

	PrevAnimation = TAnimationInfo(AnimationName.c_str(), AnimSequence, AnimLength, EndPos);
	return Result;
}

const DMSSimMotion* FindParametricAnimation(const DMSSimScenarioParser* const Parser, const char* const AnimationName, DMSSimAnimationType& AnimationType)
{
	const auto AnimationCount = Parser->GetAnimationSequenceCount();
	for (size_t i = 0; i < AnimationCount; ++i)
	{
		const auto& Animation = Parser->GetAnimationSequence(i);
		if (strcmp(Animation.GetName(), AnimationName) != 0)
			continue;
		if (Animation.GetMotionCount() != 1)
			continue;
		const auto& Motion = Animation.GetMotion(0);
		if (Motion.GetType() != DMSSimMotionParametric)
			continue;
		AnimationType = Animation.GetType();
		return &Motion;
	}
	return nullptr;
}

float GetBlendOutParameter(
	const DMSSimMotion& Motion,
	const DMSSimAnimationChannelType TargetChannel,
	const DMSSimAnimationChannel* const CommonChannel,
	const DMSSimScenarioParser* const Parser)
{
	float BlendOut = Motion.GetBlendOut();
	if (BlendOut < 0.0f)
	{
		auto TargetChannelAdj = TargetChannel;
		if (!CommonChannel)
		{
			TargetChannelAdj = DMSSimAnimationChannelCommon;
		}
		BlendOut = Parser->GetDefaultBlendOut(TargetChannelAdj);
	}
	return BlendOut;
}

std::vector<std::string> SplitString(const std::string& S, const char Delim)
{
	std::vector<std::string> Result;
	std::stringstream Stream(S);
	std::string Item;

	while (std::getline(Stream, Item, Delim))
	{
		Result.push_back(Item);
	}
	return Result;
}

bool BuildParametricAnimationMontage(
	const DMSSimScenarioParser* const   Parser,
	TEnvironment&                       Environment,
	const DMSSimAnimationChannelType    TargetChannel,
	const DMSSimMotion&                 Motion,
	const DMSSimAnimationChannel* const CommonChannel,
	const float                         BlendIn,
	const float                         ChannelStartPos,
	const float                         ChannelDuration,
	const bool                          FirstMontage,
	const bool                          ParentFirstMontage,
	TAnimationInfo&                     PrevAnimation,
	TMontage&                           MontageInfo)
{
	const char* const AnimationName = Motion.GetAnimationName();
	DMSSimAnimationType AnimationType = DMSSimAnimationUnknown;
	const DMSSimMotion* const ParametricMotion = FindParametricAnimation(Parser, AnimationName, AnimationType);
	if (!ParametricMotion)
	{
		return false;
	}

	if (TargetChannel == DMSSimAnimationChannelCommon || !CommonChannel)
	{
		throw std::exception((std::string("Common channel cannot contain parametric animations. Animation - ") + AnimationName + ".").c_str());
	}

	struct
	{
		DMSSimAnimationChannelType TargetChannel;
		DMSSimAnimationType        AnimationType;
	}
	const ChannelMap[] =
	{
		{ DMSSimAnimationChannelEyeGaze,       DMSSimAnimationParametricGazeDirection },
		{ DMSSimAnimationChannelHead,          DMSSimAnimationParametricHeadRotation  },
		{ DMSSimAnimationChannelEyelids,       DMSSimAnimationParametricEyeOpening    },
		{ DMSSimAnimationChannelSteeringWheel, DMSSimAnimationParametricSteeringWheel },
	};

	bool ChannelMatched = false;
	for (const auto& MapInfo : ChannelMap)
	{
		if (MapInfo.TargetChannel == TargetChannel && MapInfo.AnimationType == AnimationType)
		{
			ChannelMatched = true;
			break;
		}
	}

	if (!ChannelMatched)
	{
		throw std::exception((std::string(AnimationName) + " does not match the target channel").c_str());
	}

	auto& Curve = MontageInfo.Curve;

	float MaxTime = 0.0f;
	const auto PointCount = ParametricMotion->GetPointCount();
	for (size_t i = 0; i < PointCount; ++i)
	{
		const auto& Point = ParametricMotion->GetPoint(i);
		const auto Time = Point.GetTime();
		TAnimationPoint AnimPoint;
		AnimPoint.Point = Point.GetPoint();
		AnimPoint.Time = Time;
		Curve.Points.Add(AnimPoint);
		MaxTime = std::max(MaxTime, Time);
	}

	auto StartPos = Motion.GetStartPos();
	auto EndPos = Motion.GetEndPos();
	if (EndPos < 0)
	{
		EndPos = MaxTime;
	}

	StartPos = std::min(StartPos, EndPos);

	auto Duration = Motion.GetDuration();
	if (Duration < 0)
	{
		Duration = EndPos - StartPos;
	}
	else
	{
		const auto OldDuration = (EndPos - StartPos);
		if (OldDuration > DMSSIM_MIN_ANIMATION_DURATION)
		{
			const auto Scale = Duration / OldDuration;

			for (size_t i = 0; i < Curve.Points.Num(); ++i)
			{
				auto& Point = Curve.Points[i];
				Point.Time *= Scale;
			}
			StartPos *= Scale;
			EndPos *= Scale;
		}
	}

	const float BlendOut = GetBlendOutParameter(Motion, TargetChannel, CommonChannel, Parser);

	Curve.Duration = Duration;
	Curve.StartPos = StartPos;
	Curve.EndPos = EndPos;
	Curve.BlendOut = BlendOut;

	MontageInfo.BlendIn = BlendIn;
	MontageInfo.BlendOut = BlendOut;

	MontageInfo.Time = Duration;
	MontageInfo.TimeFull = Duration;

	PrevAnimation = TAnimationInfo(Curve.Points, EndPos);
	return true;
}

bool BuildAnimationMontage(
	const DMSSimScenarioParser* const   Parser,
	TEnvironment&                       Environment,
	const DMSSimAnimationChannelType    TargetChannel,
	const DMSSimMotion&                 Motion,
	const DMSSimAssetRegistry* const    AssetRegistry,
	const DMSSimAnimationChannel* const CommonChannel,
	USkeleton* const                    Skeleton,
	const float                         BlendIn,
	const float                         ChannelStartPos,
	const float                         ChannelDuration,
	const bool                          FirstMontage,
	const bool                          ParentFirstMontage,
	TAnimationInfo&                     PrevAnimation,
	TMontage&                           MontageInfo)
{
	if (Motion.GetType() != DMSSimMotionAnimation)
	{
		return false;
	}

	if (TargetChannel == DMSSimAnimationChannelSteeringWheel)
	{
		throw std::exception("Steering Wheel channel doesn't support recorded animations!");
	}

	bool Result = true;
	UAnimMontage* const Montage = Environment.CreateMontage();
	Montage->SetSkeleton(Skeleton);
	FSlotAnimationTrack* const Slot = Environment.GetMontageSlot(TargetChannel, Montage);

	Environment.AddMontageSection(TargetChannel, Montage);

	float Time = 0;
	float TimeFull = 0;
	Result = AddMontageSlotSegmentAnimation(
		Environment, TargetChannel, Motion, AssetRegistry, Slot, ChannelStartPos, ChannelDuration, FirstMontage, PrevAnimation, Time, TimeFull);

	const float BlendOut = GetBlendOutParameter(Motion, TargetChannel, CommonChannel, Parser);

	Montage->BlendIn = BlendIn;
	Montage->BlendOut = BlendOut;
	Montage->PostLoad();

	MontageInfo.Montage = Montage;

	MontageInfo.BlendIn = BlendIn;
	MontageInfo.BlendOut = BlendOut;
	MontageInfo.Time = Time;
	MontageInfo.TimeFull = TimeFull;

	return Result;
}

bool BuildParametricPassivePauseMontage(
	const DMSSimScenarioParser* const   Parser,
	TEnvironment&                       Environment,
	const DMSSimAnimationChannelType    TargetChannel,
	const DMSSimMotion&                 Motion,
	const DMSSimAssetRegistry* const    AssetRegistry,
	const DMSSimAnimationChannel* const CommonChannel,
	USkeleton* const                    Skeleton,
	const float                         BlendIn,
	const float                         ChannelStartPos,
	const float                         ChannelDuration,
	const bool                          FirstMontage,
	const bool                          ParentFirstMontage,
	TAnimationInfo&                     PrevAnimation,
	TMontage&                           MontageInfo)
{
	if (Motion.GetType() != DMSSimMotionPausePassive || PrevAnimation.Points_.Num() == 0)
	{
		return false;
	}

	const auto& PrevPoints = PrevAnimation.Points_;
	float AnimationLength = 0.0f;

	for (size_t i = 0; i < PrevPoints.Num(); ++i)
	{
		AnimationLength = std::max(PrevPoints[i].Time, AnimationLength);
	}

	const float PauseDuration = Motion.GetDuration();
	float StartPos = 0.0f;
	float EndPos = PauseDuration;

	auto& Curve = MontageInfo.Curve;
	auto& Points = Curve.Points;

	if ((PrevAnimation.EndPos_ + PASSIVE_PAUSE_DELTA) > AnimationLength) {

		auto NewPoint = PrevPoints[PrevPoints.Num() - 1];
		NewPoint.Time = 0.0f;
		Points.Add(NewPoint);
		NewPoint.Time += PauseDuration;
		Points.Add(NewPoint);
	}
	else
	{
		const float Scale = PauseDuration / PASSIVE_PAUSE_DELTA;
		for (size_t i = 0; i < PrevPoints.Num(); ++i)
		{
			auto Point = PrevPoints[i];
			Point.Time *= Scale;
			Points.Add(Point);
		}
		StartPos = PrevAnimation.EndPos_ * Scale;
		EndPos = StartPos + PauseDuration;
	}

	const float BlendOut = GetBlendOutParameter(Motion, TargetChannel, CommonChannel, Parser);

	MontageInfo.Montage = nullptr;
	MontageInfo.BlendIn = BlendIn;
	MontageInfo.BlendOut = BlendOut;

	Curve.Duration = PauseDuration;
	Curve.StartPos = StartPos;
	Curve.EndPos = EndPos;
	Curve.BlendOut = BlendOut;

	MontageInfo.Time = PauseDuration;
	MontageInfo.TimeFull = PauseDuration;
	MontageInfo.Pause = true;
	return true;
}

bool BuildPurePassivePauseMontage(
	const DMSSimScenarioParser* const   Parser,
	TEnvironment&                       Environment,
	const DMSSimAnimationChannelType    TargetChannel,
	const DMSSimMotion&                 Motion,
	const DMSSimAnimationChannel* const CommonChannel,
	USkeleton* const                    Skeleton,
	const float                         BlendIn,
	const float                         ChannelStartPos,
	const float                         ChannelDuration,
	TAnimationInfo&                     PrevAnimation,
	TMontage&                           MontageInfo)
{
	if (Motion.GetType() != DMSSimMotionPausePassive || !PrevAnimation.Name_.empty())
	{
		return false;
	}

	if (TargetChannel == DMSSimAnimationChannelSteeringWheel)
	{
		throw std::exception("Steering Wheel animation channel starting with a passive pause is not supported!");
	}

	UAnimMontage* const Montage = Environment.CreateMontage();
	Montage->SetSkeleton(Skeleton);
	Environment.AddMontageSection(TargetChannel, Montage);

	bool Result = true;
	float Time = Motion.GetDuration();
	float TimeFull = 0;

	if (Time < 0.0)
	{
		throw std::exception("Negative passive pause duration!");
	}

	TimeFull = Time;
	if (ChannelDuration > 0)
	{
		if (ChannelStartPos < Time && (ChannelStartPos + ChannelDuration) > 0)
		{
			Time = std::min(Time, ChannelStartPos + ChannelDuration);
			Time -= std::max(0.0f, ChannelStartPos);
		}
		else
		{
			Result = false;
		}
	}

	const float BlendOut = GetBlendOutParameter(Motion, TargetChannel, CommonChannel, Parser);
	Montage->BlendIn = BlendIn;
	Montage->BlendOut = BlendOut;
	Montage->PostLoad();
	MontageInfo.Montage = Montage;
	MontageInfo.BlendIn = BlendIn;
	MontageInfo.BlendOut = BlendOut;
	MontageInfo.Time = Time;
	MontageInfo.TimeFull = TimeFull;
	return Result;
}

bool BuildPassivePauseMontage(
	const DMSSimScenarioParser* const   Parser,
	TEnvironment&                       Environment,
	const DMSSimAnimationChannelType    TargetChannel,
	const DMSSimMotion&                 Motion,
	const DMSSimAssetRegistry* const    AssetRegistry,
	const DMSSimAnimationChannel* const CommonChannel,
	USkeleton* const                    Skeleton,
	const float                         BlendIn,
	const float                         ChannelStartPos,
	const float                         ChannelDuration,
	const bool                          FirstMontage,
	const bool                          ParentFirstMontage,
	TAnimationInfo&                     PrevAnimation,
	TMontage&                           MontageInfo)
{
	if (Motion.GetType() != DMSSimMotionPausePassive || PrevAnimation.Points_.Num() > 0)
	{
		return false;
	}

	const auto& Name = PrevAnimation.Name_;
	if (Name.empty())
	{
		return false;
	}

	const auto AnimationLength = PrevAnimation.Length_;

	float StartPos = PrevAnimation.EndPos_ - PASSIVE_PAUSE_DELTA;
	float EndPos = PrevAnimation.EndPos_;
	if ((PrevAnimation.EndPos_ + PASSIVE_PAUSE_DELTA) < AnimationLength)
	{
		StartPos = PrevAnimation.EndPos_;
		EndPos = PrevAnimation.EndPos_ + PASSIVE_PAUSE_DELTA;
	}

	float PauseDuration = Motion.GetDuration();
	if (ChannelDuration > 0 && (ChannelStartPos + ChannelDuration) > 0)
	{
		PauseDuration = std::min(PauseDuration, ChannelStartPos + ChannelDuration);
	}

	DMSSimCustomMotion PauseMotion(DMSSimMotionAnimation, Name.c_str(), StartPos, EndPos, PauseDuration, Motion.GetBlendOut());
	if (BuildAnimationMontage(
		Parser, Environment, TargetChannel, PauseMotion, AssetRegistry, CommonChannel, Skeleton, 0,
		ChannelStartPos, ChannelDuration, FirstMontage, ParentFirstMontage, PrevAnimation, MontageInfo))
	{
		MontageInfo.Pause = true;
		return true;
	}
	return false;
}

bool BuildMontage(
	const DMSSimScenarioParser* const   Parser,
	TEnvironment& Environment,
	const DMSSimAnimationChannelType    TargetChannel,
	const DMSSimMotion& Motion,
	const DMSSimAssetRegistry* const    AssetRegistry,
	const DMSSimAnimationChannel* const CommonChannel,
	USkeleton* const                    Skeleton,
	const float                         BlendIn,
	const float                         ChannelStartPos,
	const float                         ChannelDuration,
	const bool                          FirstMontage,
	const bool                          ParentFirstMontage,
	TAnimationInfo& PrevAnimation,
	TMontage& MontageInfo)
{
	if (BuildParametricAnimationMontage(
		Parser, Environment, TargetChannel, Motion, CommonChannel,
		BlendIn, ChannelStartPos, ChannelDuration, FirstMontage, ParentFirstMontage, PrevAnimation, MontageInfo))
	{
		return true;
	}

	if (BuildPassivePauseMontage(
		Parser, Environment, TargetChannel, Motion, AssetRegistry, CommonChannel, Skeleton,
		BlendIn, ChannelStartPos, ChannelDuration, FirstMontage, ParentFirstMontage, PrevAnimation, MontageInfo))
	{
		return true;
	}

	if (BuildParametricPassivePauseMontage(
		Parser, Environment, TargetChannel, Motion, AssetRegistry, CommonChannel, Skeleton,
		BlendIn, ChannelStartPos, ChannelDuration, FirstMontage, ParentFirstMontage, PrevAnimation, MontageInfo))
	{
		return true;
	}

	if (BuildPurePassivePauseMontage(
		Parser, Environment, TargetChannel, Motion, CommonChannel, Skeleton,
		BlendIn, ChannelStartPos, ChannelDuration, PrevAnimation, MontageInfo))
	{
		return true;
	}

	if (BuildAnimationMontage(
		Parser, Environment, TargetChannel, Motion, AssetRegistry, CommonChannel, Skeleton,
		BlendIn, ChannelStartPos, ChannelDuration, FirstMontage, ParentFirstMontage, PrevAnimation, MontageInfo))
	{
		return true;
	}
	return false;
}

void AdjustPauseBlending(TArray<TMontage>& MontageList)
{
	// there should be no blending between an animation and the follewed passive pause
	for (size_t i = 1; i < MontageList.Num(); ++i)
	{
		auto& Info = MontageList[i];
		auto& PrevInfo = MontageList[i - 1];
		if (Info.Pause)
		{
			Info.Pause = false;
			Info.BlendIn = 0;
			PrevInfo.BlendOut = 0;
			PrevInfo.Curve.BlendOut = 0;
		}
	}
}

bool BuildInternal(
	const DMSSimScenarioParser* const Parser,
	TEnvironment&                     Environment,
	const DMSSimAnimationChannelType  TargetChannel,
	const DMSSimAssetRegistry* const  AssetRegistry,
	const DMSSimAnimationChannel&     Channel,
	const DMSSimAnimationChannel*     CommonChannel,
	USkeleton* const                  Skeleton,
	const float                       ChannelStartPos,
	const float                       ChannelDuration,
	const bool                        ParentFirstMontage,
	TArray<TMontage>&                 MontageList)
{
	float StartPos = 0;
	float Duration = 0;
	float BlendIn = 0;
	const size_t MotionCount = Channel.GetMotionCount();
	TAnimationInfo PrevAnimationInfo{};
	for (size_t i = 0; i < MotionCount; ++i)
	{
		const auto& Motion = Channel.GetMotion(i);
		if (Motion.GetType() == DMSSimMotionPausePassive)
		{
			BlendIn = 0;
		}
		const float BlendOut = GetBlendOutParameter(Motion, TargetChannel, CommonChannel, Parser);

		if (Motion.GetType() == DMSSimMotionPauseActive)
		{
			switch (Channel.GetType())
			{
			case DMSSimAnimationChannelCommon:
				throw std::exception("Common channel cannot contain active pauses.");
				break;

			case DMSSimAnimationChannelSteeringWheel:
				throw std::exception("Steering Wheel channel can contain only compatible animations.");
				break;
			default:
				break;
			}
		}

		StartPos = std::max(0.0f, StartPos - BlendIn);
		switch (Motion.GetType())
		{
		case DMSSimMotionAnimation:
		case DMSSimMotionPausePassive:
			{
				if (ChannelDuration > 0)
				{
					if (StartPos >= (ChannelStartPos + ChannelDuration))
					{
						break;
					}
					if (Duration > (ChannelDuration - 0.001))
					{
						break;
					}
				}
				TMontage MontageInfo{};
				if (BuildMontage(Parser, Environment, TargetChannel, Motion, AssetRegistry, CommonChannel, Skeleton, BlendIn, ChannelStartPos - StartPos, ChannelDuration, !i, ParentFirstMontage, PrevAnimationInfo, MontageInfo))
				{
					Duration += MontageInfo.Time;
					MontageList.Add(MontageInfo);
				}
				StartPos += MontageInfo.TimeFull;
			}
			break;

		case DMSSimMotionPauseActive:
			if (!CommonChannel)
			{
				throw std::exception("Active pauses require the common channel!");
			}
			else
			{
				const float PauseDuration = Motion.GetDuration();
				if (PauseDuration < 0.0)
				{
					throw std::exception("Negative active pause duration!");
				}

				TArray<TMontage> NewMontageList;
				if (!BuildInternal(Parser, Environment, TargetChannel, AssetRegistry, *CommonChannel, nullptr, Skeleton, StartPos, PauseDuration, !i, NewMontageList))
				{
					return false;
				}

				if (NewMontageList.Num() != 0)
				{
					auto& MontageInfoFirst = NewMontageList[0];
					auto& MontageInfoLast = NewMontageList[NewMontageList.Num() - 1];
					MontageInfoFirst.BlendIn = BlendIn;
					if (MontageInfoFirst.Montage)
					{
						MontageInfoFirst.Montage->BlendIn = BlendIn;
					}

					MontageInfoLast.BlendOut = BlendOut;
					if (MontageInfoLast.Montage)
					{
						MontageInfoLast.Montage->BlendOut = BlendOut;
					}

					for (size_t j = 0; j < NewMontageList.Num(); ++j)
					{
						const auto& MontageInfo = NewMontageList[j];
						Duration += MontageInfo.Time;
						MontageList.Add(MontageInfo);
						if (j)
						{
							StartPos = std::max(0.0f, StartPos - BlendIn);
						}
						StartPos += MontageInfo.Time;
					}
				}
			}
			break;
		}
		BlendIn = BlendOut;
	}

	AdjustPauseBlending(MontageList);
	return true;
}

const DMSSimAnimationChannel* FindCommonChannel(const DMSSimOccupantScenario& Scenario)
{
	const size_t ChannelCount = Scenario.GetChannelCount();
	for (size_t j = 0; j < ChannelCount; ++j)
	{
		const auto& AnimChannel = Scenario.GetChannel(j);
		if (AnimChannel.GetType() == DMSSimAnimationChannelCommon)
		{
			return &AnimChannel;
		}
	}
	return nullptr;
}
} // anonymous namespace

bool Build(
	TEnvironment&                     Environment,
	const DMSSimScenarioParser* const Parser,
	const DMSSimAssetRegistry* const  AssetRegistry,
	const FDMSSimOccupantType         Occupant,
	const DMSSimAnimationChannelType  Channel,
	USkeleton* const                  Skeleton,
	TArray<TMontage>&                 MontageList)
{
	const size_t OccupantCount = Parser->GetOccupantScenarioCount();
	for (size_t i = 0; i < OccupantCount; ++i)
	{
		const DMSSimOccupantScenario& Scenario = Parser->GetOccupantScenario(i);
		if (Scenario.GetType() == Occupant)
		{
			const DMSSimAnimationChannel* const CommonChannel = FindCommonChannel(Scenario);
			DMSSimAnimationChannelProxy ChannelProxy(CommonChannel);
			const auto* ChannelProxyPtr = &ChannelProxy;
			const DMSSimAnimationChannel* AnimChannel = nullptr;

			const size_t ChannelCount = Scenario.GetChannelCount();
			for (size_t j = 0; j < ChannelCount; ++j)
			{
				const auto& ChannelInfo = Scenario.GetChannel(j);
				if (ChannelInfo.GetType() == Channel)
				{
					AnimChannel = &ChannelInfo;
				}
			}

			if (AnimChannel == nullptr && Channel != DMSSimAnimationChannelSteeringWheel)
			{
				AnimChannel = CommonChannel;
				ChannelProxyPtr = nullptr;
			}

			if (!AnimChannel)
			{
				break;
			}

			if (BuildInternal(Parser, Environment, Channel, AssetRegistry, *AnimChannel, ChannelProxyPtr, Skeleton, -1.0f, -1.0f, true, MontageList))
			{
				if (MontageList.Num() > 0)
				{
					// don't adjust the last montage
					for (size_t k = 0; k < (MontageList.Num() - 1); ++k)
					{
						auto& MontageInfo = MontageList[k];
						auto& MontageInfo1 = MontageList[k + 1];
						if (MontageInfo.Montage || MontageInfo1.Montage)
						{
							MontageInfo.Time = std::max(0.0f, MontageInfo.Time - MontageInfo.BlendOut);
						}
					}
					// disable blendin/blendout at start/end
					auto& FirstMontageInfo = MontageList[0];
					FirstMontageInfo.BlendIn = 0;
					if (FirstMontageInfo.Montage)
					{
						FirstMontageInfo.Montage->BlendIn = 0.0f;
					}

					// DMSSIM-629: do not zero out the last montage's blend-out
					// to enable smooth transition to the default pose 
					// at the end of the scenario
				}
				return true;
			}
		}
	}
	return false;
}

} // namespace DMSSimMontageBuilder
