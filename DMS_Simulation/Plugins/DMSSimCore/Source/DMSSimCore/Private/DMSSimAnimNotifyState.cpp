#include "DMSSimAnimNotifyState.h"
#include "DMSSimAnimInstance.h"

namespace {
	constexpr char SOCKET_LEFT_HAND[] = "hand_l";
	constexpr char SOCKET_RIGHT_HAND[] = "hand_r";

	bool GetObjectPosition(const FDMSSimCharacter Character, const FDMSSimAttachedObject& Info, FDMSSimAttachedObjectPosition& Position) {
		static const TSet<FDMSSimCharacter> CharacterSimilaritySets[] =
		{
			{ CharacterGavin, CharacterJesse, CharacterMylen, CharacterKeiji, CharacterLucian, CharacterOmar, CharacterStephane,  CharacterTaro, CharacterTrey  },
			{ CharacterHana, CharacterAda, CharacterBernice, CharacterDanielle, CharacterGlenda, CharacterMaria, CharacterNeema, CharacterRoux, CharacterSookJa },
		};

		const auto* Result = Info.Positions.Find(Character);
		if (Result)
		{
			Position = *Result;
			return true;
		}

		for (const auto& SimilaritySet : CharacterSimilaritySets)
		{
			if (SimilaritySet.Find(Character))
			{
				for (const auto& SimilarCharacter : SimilaritySet)
				{
					Result = Info.Positions.Find(Character);
					if (Result)
					{
						Position = *Result;
						return true;
					}
				}
				break;
			}
		}

		TArray<FDMSSimAttachedObjectPosition> Positions;
		Info.Positions.GenerateValueArray(Positions);
		if (Positions.Num() == 0)
		{
			return false;
		}
		Position = Positions[0];
		return true;
	}

	FDMSSimCharacter GetCharacter(USkeletalMeshComponent* const MeshComp) {

		static const TMap<FString, FDMSSimCharacter> CharacterMap =
		{
			{ "BP_Gavin_C",    CharacterGavin    },
			{ "BP_Hana_C",     CharacterHana     },
			{ "BP_Ada_C",      CharacterAda      },
			{ "BP_Bernice_C",  CharacterBernice  },
			{ "BP_Jesse_C",    CharacterJesse    },
			{ "BP_Mylen_C",    CharacterMylen    },
			{ "BP_Danielle_C", CharacterDanielle },
			{ "BP_Glenda_C",   CharacterGlenda   },
			{ "BP_Keiji_C",    CharacterKeiji    },
			{ "BP_Lucian_C",   CharacterLucian   },
			{ "BP_Maria_C",    CharacterMaria    },
			{ "BP_Neema_C",    CharacterNeema    },
			{ "BP_Omar_C",     CharacterOmar     },
			{ "BP_Roux_C",     CharacterRoux     },
			{ "BP_Sook-ja_C",  CharacterSookJa   },
			{ "BP_Stephane_C", CharacterStephane },
			{ "BP_Taro_C",     CharacterTaro     },
			{ "BP_Trey_C",     CharacterTrey     },
		};

		const auto* const Actor = MeshComp->GetOwner();
		if (Actor)
		{
			const auto* const CharacterClass = Actor->GetClass();
			const auto Name = CharacterClass->GetName();
			const auto* const Result = CharacterMap.Find(Name);
			if (Result)
			{
				return *Result;
			}
		}
		return CharacterUnknow;
	}

	bool SpawnObject(TMap<UAnimSequenceBase*, FDMSSimHandheldObject>& ObjectMap, const FDMSSimAttachedObject& Info, const char* const Socket, USkeletalMeshComponent* const MeshComp, UAnimSequenceBase* const Animation)
	{
		static TAtomic<int> Index = 1;

		auto* const ObjInfo = ObjectMap.Find(Animation);
		if (ObjInfo)
		{
			ObjInfo->RefCount++;
			return false;
		}

		auto* const ObjClass = Info.ObjectClass.Get();
		if (ObjClass == nullptr || MeshComp == nullptr)
		{
			return false;
		}

		const FDMSSimCharacter Character = GetCharacter(MeshComp);

		FDMSSimAttachedObjectPosition Position = {};
		if (!GetObjectPosition(Character, Info, Position)) {
			return false;
		}

		FActorSpawnParameters SpawnParameters = {};
		FString Name(ObjClass->GetFName().ToString());
		Name += "_";
		Name += FString::FromInt(Index++);
		SpawnParameters.Name = *Name;
		SpawnParameters.Owner = MeshComp->GetAttachmentRootActor();

		const auto Actor = MeshComp->GetWorld()->SpawnActor<AActor>(ObjClass, SpawnParameters);
		if (!Actor)
		{
			return false;
		}

		Actor->AttachToComponent(MeshComp, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, true), Socket);
		Actor->SetActorRelativeLocation(Position.Location);
		Actor->SetActorRelativeRotation(Position.Rotation);
		Actor->SetActorRelativeScale3D(Position.Scale);

		FDMSSimHandheldObject NewObjInfo;
		NewObjInfo.Object_ = Actor;
		ObjectMap.Add(Animation, NewObjInfo);
		return true;
	}

	bool DestroyObject(TMap<UAnimSequenceBase*, FDMSSimHandheldObject>& ObjectMap, UAnimSequenceBase* const Animation)
	{
		auto* const ObjInfo = ObjectMap.Find(Animation);
		if (!ObjInfo)
		{
			return false;
		}

		ObjInfo->RefCount--;
		if (ObjInfo->RefCount > 0)
		{
			return false;
		}

		ObjInfo->Object_->Destroy();
		ObjectMap.Remove(Animation);

		return ObjectMap.Num() == 0;
	}

	UDMSSimAnimInstance* GetAnimInstance(USkeletalMeshComponent* MeshComp)
	{
		if (!MeshComp)
		{
			return nullptr;
		}
		auto* AnimInstance = Cast<UDMSSimAnimInstance>(MeshComp->GetAnimInstance());
		return AnimInstance;
	}
} // anonymous namespace

void UDMSSimAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	auto* const AnimInstance = GetAnimInstance(MeshComp);
	// In Animation Preview/Editor the AnimInstance is not present, so we keep track of handheld objects in the Notify State itself
	if (AnimInstance)
	{
		auto& LeftHandObject = AnimInstance->LeftHandObject_;
		auto& RightHandObject = AnimInstance->RightHandObject_;

		if (SpawnObject(LeftHandObject, LeftHand, SOCKET_LEFT_HAND, MeshComp, Animation))
		{			
			AnimInstance->SteeringIKLeftHandBlendOp  = UDMSSimAnimInstance::IKBlend_Out;
			AnimInstance->SteeringIKLeftHandBlendOut = LeftHand.BlendInTime;
		}

		if (SpawnObject(RightHandObject, RightHand, SOCKET_RIGHT_HAND, MeshComp, Animation))
		{			
			AnimInstance->SteeringIKRightHandBlendOp  = UDMSSimAnimInstance::IKBlend_Out;
			AnimInstance->SteeringIKRightHandBlendOut = RightHand.BlendInTime;
		}
	}
	else
	{
		SpawnObject(LeftHandObject_, LeftHand, SOCKET_LEFT_HAND, MeshComp, Animation);
		SpawnObject(RightHandObject_, RightHand, SOCKET_RIGHT_HAND, MeshComp, Animation);
	}
}

void UDMSSimAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	auto* const AnimInstance = GetAnimInstance(MeshComp);
	// In Animation Preview/Editor the AnimInstance is not present, so we keep track of handheld objects in the Notify State itself
	if (AnimInstance)
	{
		auto& LeftHandObject = AnimInstance->LeftHandObject_;
		auto& RightHandObject = AnimInstance->RightHandObject_;

		if ( DestroyObject(LeftHandObject, Animation) != false )
		{
			AnimInstance->SteeringIKLeftHandBlendOp = UDMSSimAnimInstance::IKBlend_In;
			AnimInstance->SteeringIKLeftHandBlendIn = LeftHand.BlendOutTime;
		}

		if ( DestroyObject(RightHandObject, Animation) != false )
		{
			AnimInstance->SteeringIKRightHandBlendOp = UDMSSimAnimInstance::IKBlend_In;
			AnimInstance->SteeringIKRightHandBlendIn = RightHand.BlendOutTime;
		}
	}
	else
	{
		DestroyObject(LeftHandObject_, Animation);
		DestroyObject(RightHandObject_, Animation);
	}
}

void UDMSSimAnimNotifyAffectorState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	auto* const AnimInstance = GetAnimInstance(MeshComp);
	if (!AnimInstance)
	{
		return;
	}

	switch (EffectorUse)
	{
	case EffectorUseLeftHand:
		AnimInstance->LeftHandEffector = Effector;
		AnimInstance->LeftEffectorUseObject = UseObjectPosition;
		break;

	case EffectorUseRightHand:
		AnimInstance->RightHandEffector = Effector;
		AnimInstance->RightEffectorUseObject = UseObjectPosition;
		break;
	}
}

void UDMSSimAnimNotifyAffectorState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	auto* const AnimInstance = GetAnimInstance(MeshComp);
	if (!AnimInstance)
	{
		return;
	}

	switch (EffectorUse)
	{
	case EffectorUseLeftHand:
		AnimInstance->LeftHandEffector = EffectorNone;
		break;

	case EffectorUseRightHand:
		AnimInstance->RightHandEffector = EffectorNone;
		break;
	}
}

FString UDMSSimAnimNotifyAffectorState::GetNotifyName_Implementation() const
{
	return TEXT("DMS Hand Effector");
}
