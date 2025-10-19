#include "DMSSimOrchestrator.h"
#include "DMSSimConstants.h"
#include <cassert>
#include <regex>

int DMSSimOrchestrator::FindAssetIndex(const DMSSimResourceSet& ResourceSet, const char* const Name) {
	if (Name == nullptr) { return 0; }
	std::string NameStr(Name);
	if (NameStr.empty()) { return 0; }
	if (NameStr == DMSSIM_TOKEN_NONE) { return -1; }
	std::transform(NameStr.begin(), NameStr.end(), NameStr.begin(), [](unsigned char c) { return std::tolower(c); });
	const size_t AssetCount = ResourceSet.GetResourceCount();
	for (size_t i = 0; i < AssetCount; ++i) {
		const auto AssetName = ResourceSet.GetResourceName(i);
		if (strncmp(NameStr.c_str(), AssetName, NameStr.length()) == 0) {
			const std::regex IndexRegex("^_(\\d+)_");
			std::cmatch Matches;
			if (std::regex_search(AssetName + NameStr.length(), Matches, IndexRegex)) {
				const std::string IndexStr = Matches[1];
				const int Index = atoi(IndexStr.c_str());
				return Index;
			}
		}
	}
	throw std::exception((std::string("Could not find asset \"") + Name + "\"!").c_str());
	return 0;
}

template <size_t ItemCount>
int DMSSimOrchestrator::GetItemIndex(const char* ItemName, const char* const (&Items)[ItemCount]) {
	if (ItemName) {
		for (size_t i = 0; i < ItemCount; ++i) {
			const char* Item = Items[i];
			if (strcmp(Item, ItemName) == 0) { return int(i) + 1; }
		}
	}
	return 0;
}

bool DMSSimOrchestrator::Initialize(const DMSSimScenarioParser* const Parser, const DMSSimAssetRegistry* AssetRegistry, const FDMSSimOccupantType OccupantType) {
	assert(Parser);
	const DMSSimOccupant* Occupant = nullptr;
	const DMSSimOccupantScenario* OccupantScenario = nullptr;

	const size_t OccupantCount = Parser->GetOccupantCount();
	for (size_t i = 0; i < OccupantCount; ++i) {
		const auto& OccupantLoc = Parser->GetOccupant(i);
		if (OccupantLoc.GetType() == OccupantType) {
			Occupant = &OccupantLoc;
			break;
		}
	}

	const size_t OccupantScenarioCount = Parser->GetOccupantScenarioCount();
	for (size_t i = 0; i < OccupantScenarioCount; ++i) {
		const auto& OccupantScenarioLoc = Parser->GetOccupantScenario(i);
		if (OccupantScenarioLoc.GetType() == OccupantType) {
			OccupantScenario = &OccupantScenarioLoc;
			break;
		}
	}

	if (!Occupant || !OccupantScenario) { return false; }

	Character_ = Occupant->GetCharacter();
	UpperCloth_ = Occupant->GetUpperCloth() ? Occupant->GetUpperCloth() : "default";
	Headgear_ = FindAssetIndex(AssetRegistry->GetBlueprintAssets(), Occupant->GetHeadgear());
	Glasses_ = FindAssetIndex(AssetRegistry->GetBlueprintAssets(), Occupant->GetGlasses());
	GlassesColor_ = Occupant->GetGlassesColor();
	GlassesOpacity_ = Occupant->GetGlassesOpacity();
	GlassesReflective_ = Occupant->GetGlassesReflective();
	Mask_ = FindAssetIndex(AssetRegistry->GetBlueprintAssets(), Occupant->GetMask());
	Scarf_ = FindAssetIndex(AssetRegistry->GetBlueprintAssets(), Occupant->GetScarf());
	Hair_ = FindAssetIndex(AssetRegistry->GetGroomAssets(), Occupant->GetHair());
	Beard_ = FindAssetIndex(AssetRegistry->GetGroomAssets(), Occupant->GetBeard());
	Mustache_ = FindAssetIndex(AssetRegistry->GetGroomAssets(), Occupant->GetMustache());
	PupilSize_ = Occupant->GetPupilSize();
	PupilBrightness_ = Occupant->GetPupilBrightness();
	IrisSize_ = Occupant->GetIrisSize();
	IrisBrightness_ = Occupant->GetIrisBrightness();
	LimbusDarkAmount_ = Occupant->GetLimbusDarkAmount();
	IrisColor_ = Occupant->GetIrisColor();
	IrisBorderWidth_ = Occupant->GetIrisBorderWidth();
	ScleraBrightness_ = Occupant->GetScleraBrightness();
	ScleraVeins_ = Occupant->GetScleraVeins();
	SkinWrinkles_ = Occupant->GetSkinWrinkles();
	SkinRoughness_ = Occupant->GetSkinRoughness();
	SkinSpecularity_ = Occupant->GetSkinSpecularity();
	Height_ = Occupant->GetHeight() * DMSSIM_M_TO_CM;
	SeatOffset_ = Occupant->GetSeatOffset();
	SeatOffset_.X *= DMSSIM_M_TO_CM; // multiplication must be component-wise to make sure the tests compile
	SeatOffset_.Y *= DMSSIM_M_TO_CM;
	SeatOffset_.Z *= DMSSIM_M_TO_CM;
	return true;
}