#pragma once

#include "DMSSimAssetRegistry.h"
#include "DMSSimScenarioParser.h"
#include <Animation/AnimMontage.h>
#include <string>
#include <vector>

namespace DMSSimMontageBuilder
{
	struct TAnimationPoint
	{
		float   Time;
		FVector Point;
	};

	struct TCurve
	{
		TArray<TAnimationPoint> Points;
		float StartPos;
		float EndPos;
		float Duration;
		float BlendOut;
	};

	struct TMontage
	{
		UAnimMontage*             Montage;
		TCurve                    Curve;
		float                     BlendIn;
		float                     BlendOut;
		float                     Time;
		float                     TimeFull;
		bool                      Pause;
	};

	struct TEnvironment {
	protected:
		virtual ~TEnvironment(){}
	public:
		virtual UAnimMontage* CreateMontage() = 0;
		virtual UAnimSequenceBase* LoadAnimationSequence(DMSSimAnimationChannelType TargetChannel, const char* Name, const char* Path, bool* ExactMatch) = 0;
		virtual FSlotAnimationTrack* GetMontageSlot(DMSSimAnimationChannelType TargetChannel, UAnimMontage* Montage) = 0;
		virtual void AddMontageSection(DMSSimAnimationChannelType TargetChannel, UAnimMontage* Montage) = 0;
		virtual std::vector<std::string> GetNotifyEvents(UAnimSequenceBase* Sequence) = 0;
	};


	bool Build(
		TEnvironment&                    Environment,
		const DMSSimScenarioParser*      Parser,
		const DMSSimAssetRegistry*       AssetRegistry,
		const FDMSSimOccupantType         Occupant,
		const DMSSimAnimationChannelType Channel,
		USkeleton*                       Skeleton,
		TArray<TMontage>&                MontageList);
} // namespace DMSSimMontageBuilder
