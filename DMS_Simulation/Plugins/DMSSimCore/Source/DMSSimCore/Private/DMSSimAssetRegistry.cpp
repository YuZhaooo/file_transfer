#include "DMSSimAssetRegistry.h"
#include "DMSSimLog.h"
#include "DMSSimUtils.h"
#include "AssetRegistryModule.h"
#include <algorithm>

namespace {
	const FName NAME_GROOM_ASSET(TEXT("GroomAsset"));
	const FName NAME_BLUEPRINT_GENERATED_CLASS(TEXT("BlueprintGeneratedClass"));
	const FName NAME_BLUEPRINT(TEXT("Blueprint"));
	const FName NAME_ANIM_SEQUENCE(TEXT("AnimSequence"));

class DMSSimResourceSetInternal : public DMSSimResourceSet {
public:
	virtual ~DMSSimResourceSetInternal(){}
	virtual size_t GetResourceCount() const override;
	virtual const char* GetResourceName(size_t Index) const override;
	virtual const char* GetResourcePath(size_t Index) const override;

	void Update(const FAssetData& AssetData);
private:
	TArray<std::string> Names_;
	TArray<std::string> Paths_;
};

size_t DMSSimResourceSetInternal::GetResourceCount() const { return Names_.Num(); }

const char* DMSSimResourceSetInternal::GetResourceName(const size_t Index) const { return Names_[Index].c_str(); }

const char* DMSSimResourceSetInternal::GetResourcePath(const size_t Index) const { return Paths_[Index].c_str(); }

void DMSSimResourceSetInternal::Update(const FAssetData& AssetData) {
	const auto& AssetName = AssetData.AssetName;
	const auto& AssetPath = AssetData.ObjectPath;
	auto Name = WideToNarrow(FStringToWide(AssetName.ToString()).c_str());
	auto Path = WideToNarrow(FStringToWide(AssetPath.ToString()).c_str());
	const auto ToLower = [](unsigned char c) { return std::tolower(c); };
	std::transform(Name.begin(), Name.end(), Name.begin(), ToLower);
	auto PathLower = Path;
	std::transform(PathLower.begin(), PathLower.end(), PathLower.begin(), ToLower);
	if (PathLower.find("/generated/") != std::string::npos) { return; }
	Names_.Push(Name.c_str());
	Paths_.Push(Path.c_str());
}

class DMSSimAssetRegistryInternal : public DMSSimAssetRegistry {
public:
	DMSSimAssetRegistryInternal();
	virtual const DMSSimResourceSet& GetAnimations() const override;
	virtual const DMSSimResourceSet& GetGroomAssets() const override;
	virtual const DMSSimResourceSet& GetBlueprintAssets() const override;
private:
	DMSSimResourceSetInternal Animations_;
	DMSSimResourceSetInternal GroomAssets_;
	DMSSimResourceSetInternal BlueprintAssets_;
};

DMSSimAssetRegistryInternal::DMSSimAssetRegistryInternal() {
	FAssetRegistryModule& registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	auto& Registry = registry.Get();

	const TArray<FString> ResourcePaths = {
		"/Game/Animations/",
		"/Game/Models/",
		"/Game/MetaHumans/",
	};


	Registry.ScanPathsSynchronous(ResourcePaths);

	for (const auto& ResourcePath : ResourcePaths) {
		TArray<FAssetData> AssetDataSet;
		Registry.GetAssetsByPath(*ResourcePath, AssetDataSet, true, false);
		for (auto& AssetData : AssetDataSet) {
			const auto& ClassName = AssetData.AssetClass;
			if (ClassName == NAME_GROOM_ASSET) { GroomAssets_.Update(AssetData); }
			else if (ClassName == NAME_BLUEPRINT_GENERATED_CLASS || ClassName == NAME_BLUEPRINT) { BlueprintAssets_.Update(AssetData); }
			else if (ClassName == NAME_ANIM_SEQUENCE) { Animations_.Update(AssetData); }
		}
	}
}

const DMSSimResourceSet& DMSSimAssetRegistryInternal::GetAnimations() const { return Animations_; }

const DMSSimResourceSet& DMSSimAssetRegistryInternal::GetGroomAssets() const { return GroomAssets_; }

const DMSSimResourceSet& DMSSimAssetRegistryInternal::GetBlueprintAssets() const { return BlueprintAssets_; }
} // anonymous namespace

DMSSimAssetRegistry* DMSSimAssetRegistry::Create() { return new DMSSimAssetRegistryInternal; }
