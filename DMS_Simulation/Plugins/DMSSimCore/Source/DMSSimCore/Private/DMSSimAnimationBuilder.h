#pragma once

#include "CoreMinimal.h"
#include "DMSSimScenarioParser.h"

namespace DMSSimAnimationBuilder
{
	/**
	 * Loads animation sequence resource version for the specied channel.
	 */
	UAnimSequenceBase* LoadAnimationSequence(const DMSSimAnimationChannelType TargetChannel, const char* const Name, const char* const Path, bool* ExactMatch);
} // namespace DMSSimAnimationBuilder
