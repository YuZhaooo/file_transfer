#include "DMSSimAnimationFilter.h"
#include <regex>

namespace DMSSimAnimationFilter
{
bool CheckCurveChannelMatch(const wchar_t* const CurveName, const DMSSimAnimationChannelType ChannelType)
{
	static const struct {
		DMSSimAnimationChannelType Type;
		std::wregex                Pattern;
		bool					   bIsExactMatch = false;
	} Channels [] = {
		{ DMSSimAnimationChannelEyeGaze,    std::wregex(L"eyelook(Up|Down|Left|Right)[lr]",  std::regex_constants::icase) },
		{ DMSSimAnimationChannelEyelids,    std::wregex(L"eyeblink(L|R|Left|Right)", std::regex_constants::icase) },
		{ DMSSimAnimationChannelFace,       std::wregex(L"face|eyelid|blink_|look(In|Out)|eyerelax|lowerlid|upperlid|eyelashes|eyepupil|eyeparallellook|brow|squint|eyecheek|cheek|eyewide|nose|mouth|teeth|tongue|jawopen|jawleft|jawright|jawfwd|jawforward|jawback|jawclench|chincompress|ear|smile|lip|purse", std::regex_constants::icase) },
		{ DMSSimAnimationChannelUpperBody,  std::wregex(L"EffectorWeightLeftHand|EffectorWeightRightHand|^spine_|^neck_|^pelvis", std::regex_constants::icase) },
		{ DMSSimAnimationChannelHead,       std::wregex(L"head"), true },
		
	};

	for (const auto& Channel : Channels)
	{
		if (Channel.Type == ChannelType)
		{
			bool bMatches = Channel.bIsExactMatch 
				? std::regex_match(CurveName, Channel.Pattern) 
				: std::regex_search(CurveName, Channel.Pattern);
			if (bMatches)
			{
				return true;
			}
		}
	}
	return false;
}
} // namespace DMSSimAnimationSequence