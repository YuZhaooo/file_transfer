#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DMSSimCameraBlueprint.generated.h"

UCLASS()
class UDMSSimCamera : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	/**
	 * Pass camera object to the plugin, so that the plugin could apply camera parameters specified in the scenario, such as FOV, Resolution, Location, Rotation, etc.
	 */
	UFUNCTION(BlueprintCallable, Category = "DMSSimCore")
	static void SetDmsCamera(UCameraComponent* Camera);
};