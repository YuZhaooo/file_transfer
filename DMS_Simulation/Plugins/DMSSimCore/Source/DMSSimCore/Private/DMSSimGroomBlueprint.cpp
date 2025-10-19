#include "DMSSimGroomBlueprint.h"
#include "DMSSimGroomBindingBuilder.h"
#include "GroomBindingBuilder.h"


UGroomBindingAsset* UDMSSimGroomBlueprint::CreateDmsGroomBindingAsset(
	UGroomAsset* const   GroomAsset,
	USkeletalMesh* const SourceMesh,
	USkeletalMesh* const TargetMesh)
{
	UGroomBindingAsset* const BindingAsset = NewObject<UGroomBindingAsset>(UGroomBindingAsset::StaticClass());
	BindingAsset->Groom = GroomAsset;
	BindingAsset->SourceSkeletalMesh = SourceMesh;
	BindingAsset->TargetSkeletalMesh = TargetMesh;

#if !WITH_EDITORONLY_DATA
	BindingAsset->bIsValid = DMSSimGroomBindingBuilder::BuildBinding_CPU(BindingAsset, true);
#else
	BindingAsset->bIsValid = FGroomBindingBuilder::BuildBinding(BindingAsset, false, true);
#endif

	BindingAsset->QueryStatus = UGroomBindingAsset::EQueryStatus::None;
	if (BindingAsset->IsValid())
	{
		return BindingAsset;
	}
	return nullptr;
}