#pragma once

#include <string>
#include "DMSSimAssetRegistry.h"
#include "DMSSimScenarioParser.h"

/**
 * Helper class.
 * I does mapping of names of some items into indices (Resources like hats, masks etc. are currently referenced in the Editor by index).
 * I also converts seet offsets to CM, like it's used in the Editor.
 */
class DMSSimOrchestrator {
public:
	DMSSimOrchestrator() {}
	~DMSSimOrchestrator() {}

	bool Initialize(const class DMSSimScenarioParser* Parser, const DMSSimAssetRegistry* AssetRegistry, FDMSSimOccupantType Occupant);
	const char* GetCharacter() const { return Character_.c_str(); };
	const char* GetUppercloth() const { return UpperCloth_.c_str(); };
	int GetHeadgear() const { return Headgear_; };
	int GetGlasses() const { return Glasses_; };
	FVector GetGlassesColor() const { return GlassesColor_; };
	float GetGlassesOpacity() const { return GlassesOpacity_; };
	bool GetGlassesReflective() const { return GlassesReflective_; };
	int GetMask() const { return Mask_; };
	int GetScarf() const { return Scarf_; };
	int GetHair() const { return Hair_; };
	int GetBeard() const { return Beard_; };
	int GetMustache() const { return Mustache_; };
	float GetPupilSize() const { return PupilSize_; };
	float GetPupilBrightness() const { return PupilBrightness_; }
	float GetIrisSize() const { return IrisSize_; }
	float GetIrisBrightness() const { return IrisBrightness_; }
	float GetIrisBorderWidth() const { return IrisBorderWidth_; }
	float GetLimbusDarkAmount() const { return LimbusDarkAmount_; }
	const char* GetIrisColor() const { return IrisColor_.c_str(); }
	float GetScleraBrightness() const { return ScleraBrightness_; }
	float GetScleraVeins() const { return ScleraVeins_; }
	float GetSkinWrinkles() const { return SkinWrinkles_; }
	float GetSkinRoughness() const { return SkinRoughness_; }
	float GetSkinSpecularity() const { return SkinSpecularity_; }
	float GetHeight() const { return Height_; };
	FVector GetSeatOffset() const { return SeatOffset_; };

private:
	std::string                Character_;
	std::string                UpperCloth_;
	int                        Headgear_ = 0;
	int                        Glasses_ = 0;
	FVector                    GlassesColor_ = {};
	float                      GlassesOpacity_ = 0;
	bool					   GlassesReflective_ = false;
	int                        Mask_ = 0;
	int                        Scarf_ = 0;
	int                        Hair_ = 0;
	int                        Beard_ = 0;
	int                        Mustache_ = 0;
	float                      PupilSize_ = 0.0f;
	float                      PupilBrightness_ = 0.0f;
	float                      IrisSize_ = 0.0f;
	float                      IrisBrightness_ = 0.0f;
	float					   IrisBorderWidth_ = 0.0f;
	float                      LimbusDarkAmount_ = 0.0f;
	std::string                IrisColor_ = "";
	float                      ScleraBrightness_ = 0.0f;
	float                      ScleraVeins_ = 0.0f;
	float                      SkinWrinkles_ = 0.0f;
	float                      SkinRoughness_ = 0.0f;
	float                      SkinSpecularity_ = 0.0f;
	float                      Height_ = 0.0f;
	FVector                    SeatOffset_ = { 0, 0, 0 };

	int FindAssetIndex(const DMSSimResourceSet& ResourceSet, const char* AssetName);

	template <size_t ItemCount>
	int GetItemIndex(const char* ItemName, const char* const (&Items)[ItemCount]);
};