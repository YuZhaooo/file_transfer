#pragma once

#include "DMSSimMontageBuilder.h"
#include "Curves/CurveVector.h"

namespace DMSSimCurveGenerator
{
	using TCurve = DMSSimMontageBuilder::TCurve;
	/**
	 * @brief Creates an Unreal curve resource from a set of curves from a scenario.
	 * It takes into account start/end positions and blend-out time of original curves.
	 *
	 * @param[in]  Curves    Array of original curves
	 * @param[out] CurveTime Total length of the result curve
	 *
	 * @return UCurveVector resource
	 */
	UCurveVector* Generate(const TArray<TCurve>& Curves, float& CurveTime);

} // namespace DMSSimCurveGenerator
