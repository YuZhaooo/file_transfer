#pragma once

#include "CoreMinimal.h"
#include "DMSSimGazeUtils.generated.h"

/**
 * @enum FDMSSimEyeType
 * @brief left or right eye
 */
UENUM(BlueprintType)
enum FDMSSimEyeType
{
	FDMSSimEyeLeft = 0        UMETA(DisplayName = "Left Eye"),
	FDMSSimEyeRight           UMETA(DisplayName = "Right Eye"),
};

UCLASS()
class UDMSSimGazeUtilsBlueprint : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	/**
	 * Calculate eye direction curve parameters based on eye/head positions, as well as the head rotation.
	 */
	UFUNCTION(BlueprintCallable, Category = "DMSSimCore")
	static void ComputeGazeParameters(
		const FDMSSimEyeType eye,
		const FVector& ViewTarget,
		const FVector& EyeLocation,
		const FRotator& HeadRotation,
		float& AngleLeft,
		float& AngleRight,
		float& AngleUp,
		float& AngleDown);
};
