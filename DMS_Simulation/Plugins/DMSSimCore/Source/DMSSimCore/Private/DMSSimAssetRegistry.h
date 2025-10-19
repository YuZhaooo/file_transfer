#pragma once


class DMSSimResourceSet {
protected:
	virtual ~DMSSimResourceSet(){}
public:
	virtual size_t GetResourceCount() const = 0;
	virtual const char* GetResourceName(size_t Index) const = 0;
	virtual const char* GetResourcePath(size_t Index) const = 0;
};

/**
 * @brief Unreal asset registry helper class, provides information about available resources,
 * such as animations, groom assets or other items like masks, glasses, scarfs and hats (BlueprintAssets).
 */
class DMSSimAssetRegistry {
public:
	virtual ~DMSSimAssetRegistry(){}

	virtual const DMSSimResourceSet& GetAnimations() const = 0;
	virtual const DMSSimResourceSet& GetGroomAssets() const = 0;
	virtual const DMSSimResourceSet& GetBlueprintAssets() const = 0;

	static DMSSimAssetRegistry* Create();
};



