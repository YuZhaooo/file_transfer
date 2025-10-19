#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DMSSimOccupantType.h"
#include "DMSSimScenarioBlueprint.h"
#include "DMSSimGroundTruthBlueprint.generated.h"

/**
 *
 */
UCLASS()
class UDMSSimGroundTruthBlueprint : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	/**
	 * Starts recording of the simulation video and GT-data.
	 * Basically it sets a global flag, which is polled by the renderer.
	 */
	UFUNCTION(BlueprintCallable, Category = "DMSSimCore")
	static bool StartDmsRecording();

	/**
	 * Passes Camera and Car related information to the GT-data recorder
	 */
	UFUNCTION(BlueprintCallable, Category = "DMSSimCore")
	static void StoreDmsGroundTruthCommonData(
		const FDMSScenario& Scenario,
		const FVector& CameraLocation_inCar,
		const FRotator& CameraRotation_inCar,
		const FVector& IlluminationLocation_inCar,
		const FRotator IlluminationRotation_inCar,		
		const FVector& CarPosition_inWorld,
		const FRotator& CarRotation_inWorld,
		const int& OccupantCount,
		const TArray<FDMSSimOccupant>& Occupants);

	/**
	 * Passes Occupant related information to the GT-data recorder
	 */
	UFUNCTION(BlueprintCallable, Category = "DMSSimCore")
	static void StoreDmsGroundTruthFrame(
		const FDMSSimOccupantType& OccupantType,
		const FVector& NosePoint,
		const FVector& LEarPoint,
		const FVector& REarPoint,
		const FVector& LeftEyePoint,
		const FVector& RightEyePoint,
		const FVector& LeftGazeOrigin_inCam,
		const FVector& RightGazeOrigin_inCam,
		const FVector& GazeOrigin_inCam,
		const FVector& LeftGazeDirection_inCam,
		const FVector& RightGazeDirection_inCam,
		const FVector& GazeDirection_inCam,
		const FVector& LeftGazeOrigin_inCar,
		const FVector& RightGazeOrigin_inCar,
		const FVector& GazeOrigin_inCar,
		const FVector& LeftGazeDirection_inCar,
		const FVector& RightGazeDirection_inCar,
		const FVector& GazeDirection_inCar,
		const FVector& HeadOriginEyesCenter_inCam,
		const FVector& HeadOriginEarsCenter_inCam,
		const FVector& HeadDirection_inCam,
		const FRotator& HeadRotation_inCam,
		const FVector& HeadOriginEyesCenter_inCar,
		const FVector& HeadOriginEarsCenter_inCar,
		const FVector& HeadDirection_inCar,
		const FRotator& HeadRotation_inCar,
		const FVector& LeftShoulderPoint,
		const FVector& RightShoulderPoint,
		const FVector& LeftElbowPoint,
		const FVector& RightElbowPoint,
		const FVector& LeftWristPoint,
		const FVector& RightWristPoint,
		const FVector& LeftPinkyKnucklePoint,
		const FVector& RightPinkyKnucklePoint,
		const FVector& LeftIndexKnucklePoint,
		const FVector& RightIndexKnucklePoint,
		const FVector& LeftThumbKnucklePoint,
		const FVector& RightThumbKnucklePoint,
		const FVector& LeftHipPoint,
		const FVector& RightHipPoint,
		const FVector& LeftKneePoint,
		const FVector& RightKneePoint,
		const FVector& LeftAnklePoint,
		const FVector& RightAnklePoint,
		const FVector& LeftHeelPoint,
		const FVector& RightHeelPoint,
		const FVector& LeftFootIndexPoint,
		const FVector& RightFootIndexPoint,
		const float    HorizontalMouthOpening,
		const float    VerticalMouthOpening,
		const TArray<bool>& FacialLandmarksVisible,
		const TArray<FVector>& FacialLandmarks3D_inCam,
		const TArray<FVector2D>& FacialLandmarks2D,
		const TArray<FVector>& FaceBoundingBox3D_inCam,
		const bool FaceBoundingBox3DVisible,
		const FDMSBoundingBox2D& FaceBoundingBox2D,
		const bool FaceBoundingBox2DVisible,
		const FDMSBoundingBox2D& RightEyeBoundingBox2D,
		const bool RightEyeBoundingBox2DVisible,
		const FDMSBoundingBox2D& LeftEyeBoundingBox2D,
		const bool LeftEyeBoundingBox2DVisible,
		const TArray<FVector2D>& RightEyePupilLandmarks2D,
		const TArray<FVector2D>& RightEyeIrisLandmarks2D,
		const TArray<FVector2D>& LeftEyePupilLandmarks2D,
		const TArray<FVector2D>& LeftEyeIrisLandmarks2D,
		const TArray<bool>& PupilIrisLandmarksVisible,
		const float LeftEyeLidVisibilityPerc,
		const float RightEyeLidVisibilityPerc,
		const float LeftEyePupilVisibilityPerc,
		const float RightEyePupilVisibilityPerc
	);

	/**
	 * Passes Camera and Car related information to the GT-data recorder
	 */
	UFUNCTION(BlueprintCallable, Category = "DMSSimCore")
	static void ResetGroundTruthData();

	/**
	 * Sends the signal to finish recording of video and GT-data by
	 * clearing the global "Recording" flag.
	 */
	UFUNCTION(BlueprintCallable, Category = "DMSSimCore")
	static bool StopDmsRecording();

	/**
	 * Helper function to be used in the Unreal Editor
	 */
	UFUNCTION(BlueprintCallable, Category = "DMSSimCore")
	static void GetDmsFrameTime(int32& FrameIndex, float& FrameTime);
};