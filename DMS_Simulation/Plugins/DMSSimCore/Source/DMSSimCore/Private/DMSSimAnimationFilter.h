#pragma once

#include "DMSSimScenarioParser.h"

namespace DMSSimAnimationFilter
{
	/**
	 * @brief The Unreal Animation sequence resources contain some number of so-called animation curves.
	 * Each curve is responsible for animation different parts of the human model. To animate a specific
	 * part of the body we need to leave only the certain curves during filtration. The given function
	 * tells if some curve belongs to the specified animation channel.
	 *
	 * @param[in] CurveName   Name of tested curve
	 * @param[in] ChannelType Animation channel the curve is tested against
	 *
	 * @return true if the curve belongs to the channel
	 */
	bool CheckCurveChannelMatch(const wchar_t* CurveName, DMSSimAnimationChannelType ChannelType);

} // namespace DMSSimAnimationSequence
