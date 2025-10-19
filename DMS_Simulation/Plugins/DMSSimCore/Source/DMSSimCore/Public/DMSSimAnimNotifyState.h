#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "DMSSimAnimInstance.h"
#include "DMSSimAnimNotifyState.generated.h"

/**
 * @enum FDMSSimCharacter
 * @brief Supported characters
 */
UENUM(BlueprintType)
enum FDMSSimCharacter
{
	CharacterGavin      UMETA(DisplayName = "Gavin"),
	CharacterHana       UMETA(DisplayName = "Hana"),
	CharacterAda        UMETA(DisplayName = "Ada"),
	CharacterBernice    UMETA(DisplayName = "Bernice"),
	CharacterJesse      UMETA(DisplayName = "Jesse"),
	CharacterMylen      UMETA(DisplayName = "Mylen"),
	CharacterDanielle   UMETA(DisplayName = "Danielle"),
	CharacterGlenda     UMETA(DisplayName = "Glenda"),
	CharacterKeiji      UMETA(DisplayName = "Keiji"),
	CharacterLucian     UMETA(DisplayName = "Lucian"),
	CharacterMaria      UMETA(DisplayName = "Maria"),
	CharacterNeema      UMETA(DisplayName = "Neema"),
	CharacterOmar       UMETA(DisplayName = "Omar"),
	CharacterRoux       UMETA(DisplayName = "Roux"),
	CharacterSookJa     UMETA(DisplayName = "Sook-Ja"),
	CharacterStephane   UMETA(DisplayName = "Stephane"),
	CharacterTaro       UMETA(DisplayName = "Taro"),
	CharacterTrey       UMETA(DisplayName = "Trey"),
	CharacterUnknow,
};

/**
 * @enum FDMSSimEffectorUse
 * @brief Effector purpose (for the left hand or the right hand)
 */
UENUM(BlueprintType)
enum FDMSSimEffectorUse
{
	EffectorUseLeftHand    UMETA(DisplayName = "LeftHand"),
	EffectorUseRightHand   UMETA(DisplayName = "RightHand"),
};

/**
 * @enum FDMSSimAttachedObjectPosition
 * @brief Position of a hendheld object in its respect to the hand it's attached to
 */
USTRUCT(BlueprintType)
struct DMSSIMCORE_API FDMSSimAttachedObjectPosition
{
	GENERATED_BODY()
public:
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		FVector Location = {};
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		FRotator Rotation = {};
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		FVector Scale = { 1, 1, 1 };
};

/**
 * @enum FDMSSimAttachedObject
 * @brief Attached object's information. It includes the objects's class and positions for different characters.
 */
USTRUCT(BlueprintType)
struct DMSSIMCORE_API FDMSSimAttachedObject
{
	GENERATED_BODY()
public:

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	TSubclassOf<AActor> ObjectClass;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	TMap<TEnumAsByte<FDMSSimCharacter>, FDMSSimAttachedObjectPosition>  Positions;

	/** Blend in time from IK to Object */
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float BlendInTime = 0.3f;

	/** Blend out time from Object to steering IK */
    UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
    float BlendOutTime = 0.3f;
};

/**
 * @enum UDMSSimAnimNotifyState
 * @brief Attached object's information. It includes the objects's class and positions for different characters.
 */
UCLASS(Blueprintable, hidecategories = Object, collapsecategories)
class DMSSIMCORE_API UDMSSimAnimNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		FDMSSimAttachedObject LeftHand;
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		FDMSSimAttachedObject RightHand;

	UPROPERTY()
		TMap<UAnimSequenceBase*, FDMSSimHandheldObject> LeftHandObject_;
	UPROPERTY()
		TMap<UAnimSequenceBase*, FDMSSimHandheldObject> RightHandObject_;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};

UCLASS(Blueprintable, hidecategories = Object, collapsecategories,DisplayName="DMS Hand Effector")
class DMSSIMCORE_API UDMSSimAnimNotifyAffectorState : public UAnimNotifyState
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DMS, DisplayName="Use On")
	TEnumAsByte<FDMSSimEffectorUse> EffectorUse;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DMS)
	TEnumAsByte<FDMSSimEffector>	Effector;

	/** Get Position IK from attached object and use it as offset for more precise position. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DMS)
	bool							UseObjectPosition = false;

public:

	virtual void					NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;
	virtual void					NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	FString							GetNotifyName_Implementation() const;
};