#pragma once

#include "Animation/AnimInstance.h"
#include "DMSSimAnimInstance.generated.h"

/**
 * @struct FDMSSimHandheldObject
 * @brief Contains the pointer to an object, as well as the number of animations that use it at the moment.
 */
USTRUCT(BlueprintType)
struct DMSSIMCORE_API FDMSSimHandheldObject
{
	GENERATED_BODY()

	UPROPERTY()
	AActor* Object_ = nullptr;

	UPROPERTY()
	int RefCount = 1;
};

/**
 * @enum FDMSSimEffector
 * @brief Supported effectors for IK
 */
UENUM(BlueprintType)
enum FDMSSimEffector
{
	EffectorNone           UMETA(DisplayName = "None"),
	EffectorLeftEar        UMETA(DisplayName = "LeftEar"),
	EffectorRightEar       UMETA(DisplayName = "RightEar"),
	EffectorMouth          UMETA(DisplayName = "Mouth"),
	EffectorNose           UMETA(DisplayName = "Nose"),
	EffectorLeftEye        UMETA(DisplayName = "Left Eye"),
	EffectorRightEye       UMETA(DisplayName = "Right Eye"),
	EffectorMask           UMETA(DisplayName = "Mask"),
	EffectorLeftEarDirect  UMETA(DisplayName = "LeftEarDirect"),
	EffectorRightEarDirect UMETA(DisplayName = "RightEarDirect"),
};

/**
 * @class UDMSSimAnimInstance
 * @brief Left and Right hand weights in relation to IK steering wheel animation (for the driver).
 * 1.0 - the corresponding hand is on the wheel, 0.0 - overwise.
 * As well as, references to handheld objects.
 */
UCLASS(BlueprintType, Blueprintable)
class DMSSIMCORE_API UDMSSimAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:

	enum EDMSIKBlendOp : uint32
	{
		IKBlend_None = 0,
		IKBlend_In,
		IKBlend_Out
	};

public:

	/** Steering wheel IK weight for left hand */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=DMS)
	float											SteeringWeightLeftHand = 1.0f;

	/** Blend in time for steering IK left hand */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=DMS)
	float											SteeringIKLeftHandBlendIn = 0.3f;

	/** Blend out time for steering IK left hand */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=DMS)
	float											SteeringIKLeftHandBlendOut = 0.3f;

	EDMSIKBlendOp									SteeringIKLeftHandBlendOp = IKBlend_None;

	/** Steering wheel IK weight for right hand */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=DMS)
	float											SteeringWeightRightHand = 1.0f;

	/** Blend in time for steering IK right hand */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=DMS)
	float											SteeringIKRightHandBlendIn = 0.3f;

	/** Blend out time for steering IK right hand */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=DMS)
	float											SteeringIKRightHandBlendOut = 0.3f;

	EDMSIKBlendOp									SteeringIKRightHandBlendOp = IKBlend_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DMS)
	TEnumAsByte<FDMSSimEffector>					LeftHandEffector;

	/** Use object offset for left hand effector */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DMS)
	bool											LeftEffectorUseObject = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DMS)
	TEnumAsByte<FDMSSimEffector>					RightHandEffector;

	/** Use object offset for right hand effector */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DMS)
	bool											RightEffectorUseObject = false;

	UFUNCTION(BlueprintCallable, Category = DMS)
	AActor										   *GetRightHandAttachedObject() const;

	UFUNCTION(BlueprintCallable, Category = DMS)
	AActor										   *GetLeftHandAttachedObject() const;

	UPROPERTY()
	TMap<UAnimSequenceBase*, FDMSSimHandheldObject> LeftHandObject_;
	UPROPERTY()
	TMap<UAnimSequenceBase*, FDMSSimHandheldObject> RightHandObject_;

public:

	virtual void									NativeUpdateAnimation(float DeltaSeconds) override;
};
