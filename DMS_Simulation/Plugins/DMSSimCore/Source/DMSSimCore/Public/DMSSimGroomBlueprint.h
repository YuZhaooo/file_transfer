#pragma once
#include "CoreMinimal.h"
#include "GroomAsset.h"
#include "GroomBindingAsset.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DMSSimGroomBlueprint.generated.h"

UCLASS()
class UDMSSimGroomBlueprint : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	/**
	 * @brief To be able to apply hair of one character onto another character,
	 * we need to have a special resource, Groom Binding Asset.
	 * The groom binding assets are created on the fly.
	 * The hair/beard/moustache, as well as the characted, are selected in the scenario.
	 * CreateDmsGroomBindingAsset generates a new grrom binding asset.
	 *
	 * @param[in] GroomAsset Resource containing hair, beard or mustache
	 * @param[in] SourceMesh Head mesh of the character, the hair (beard, mustache) was originally made for
	 * @param[in] TargetMesh Head mesh of the target character
	 *
	 * @return New groom binding for the specified hair and the character.
	 */
	UFUNCTION(BlueprintCallable, Category = "DMSSimCore")
	static UGroomBindingAsset* CreateDmsGroomBindingAsset(
		UGroomAsset*   GroomAsset,
		USkeletalMesh* SourceMesh,
		USkeletalMesh* TargetMesh
	);
};
