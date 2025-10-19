#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DMSSimAnimationBuilderBlueprint.generated.h"

UCLASS()
class UDMSSimAnimationBuilderBlueprint : public UBlueprintFunctionLibrary
{

GENERATED_BODY()
	
	/**
	 * Generates filtered animations for each channel.
	 * The generated animations are stored in Content\Animations\Generated.
	 * For each original animation BuildDmsAnimations generates versions for EyeGaze, Eyelids, Face, Head, UpperBody, LeftHand, RightHand channels.
	 * The function is supposed to be called in the Unreal Editor, manually.
	 */
	UFUNCTION(BlueprintCallable, Category = "DMSSimCore")
	static void BuildDmsAnimations();

	/**
	 * @brief Iterates over a folder, finding all animations for the 'Face_Archetype_Skeleton' and applies pitch correction to them.
	 * As we have no way to tell Iphone animations apart from Rokoko, this needs to be applied with caution, and only once.
	 * Intended use: once you have imported some iphone animations,
	 * trigger this function from console using python script for the import folder.
	 * @note Destructive operation: will not relent if the additive curve "head" is already present.
	 * @param Folders list of folders in '/Game/XXX' format.
	 * @param Pitch head pitch correction desired.
	 */
	UFUNCTION(BlueprintCallable, Category="DMSSimCore")
	static void ApplyIphonePitchCorrectionRecursively(const TArray<FString> Folders, float DesiredHeadPitchOffset = -18.9f);
};
