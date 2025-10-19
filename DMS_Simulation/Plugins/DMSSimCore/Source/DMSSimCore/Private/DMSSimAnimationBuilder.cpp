#include "DMSSimAnimationBuilder.h"
#include "DMSSimAnimationBuilderBlueprint.h"
#include "DMSSimAnimationFilter.h"
#include "DMSSimAssetRegistry.h"
#include "DMSSimScenarioParser.h"
#include "AssetRegistryModule.h"
#include "DMSSimLog.h"

namespace
{
	constexpr char GENERATED_ANIMATIONS_LOCATION[] = "/Game/Animations/Generated/";

	const FName NAME_HEAD_BONE(TEXT("head"));

	struct DMSSimChannelInfo
	{
		DMSSimAnimationChannelType Type = DMSSimAnimationChannelCommon;
		const char*                Postfix = nullptr;
		bool                       SkipFiltering = false;
	};

	DMSSimChannelInfo ChannelInfoList[] =
	{
		{ DMSSimAnimationChannelEyeGaze,   "EyeGaze"    },
		{ DMSSimAnimationChannelEyelids,   "Eyelids"    },
		{ DMSSimAnimationChannelFace,      "Face"       },
		{ DMSSimAnimationChannelHead,      "Head"       },
		{ DMSSimAnimationChannelUpperBody, "UpperBody"  },
		{ DMSSimAnimationChannelLeftHand,  "LeftHand",  true },
		{ DMSSimAnimationChannelRightHand, "RightHand", true },
	};

	const char* GetChannelPostfix(const DMSSimAnimationChannelType Type)
	{
		for (const auto& Info : ChannelInfoList)
		{
			if (Info.Type == Type)
			{
				return Info.Postfix;
			}
		}
		return nullptr;
	}

	FString MakeAnimationName(const char* const Name, const char* const Postfix)
	{
		FString AnimationName;
		AnimationName += Name;
		if (Postfix)
		{
			AnimationName += "_";
			AnimationName += Postfix;
		}
		return AnimationName;
	}

	FString MakePackageName(const FString& AnimationName)
	{
		FString PackageName(GENERATED_ANIMATIONS_LOCATION);
		PackageName += AnimationName;
		return PackageName;
	}

	void BuildFilteredAnimation(const char* const Name, const char* const Path, const DMSSimAnimationChannelType Channel, const char* const Postfix)
	{
		FString AnimationName(MakeAnimationName(Name, Postfix));
		FString PackageName(MakePackageName(AnimationName));

		const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
		if (FPaths::FileExists(PackageFileName))
		{
			return;
		}

		FStringAssetReference ref(Path);
		UAnimSequence* const AnimSequence = Cast<UAnimSequence>(ref.TryLoad());
		const auto& RawCurves = AnimSequence->RawCurveData;

		const bool WithHiddenData = (Channel == DMSSimAnimationChannelHead) || (Channel == DMSSimAnimationChannelUpperBody);
		if (RawCurves.FloatCurves.Num() == 0 && !WithHiddenData)
		{
			return;
		}

		FRawCurveTracks RawCurvesFiltered = {};
		bool bTransformCurvesAdded = false;
		for (const auto& Curve : RawCurves.FloatCurves)
		{
			const auto CurveName = Curve.Name.DisplayName.ToString();
			if (DMSSimAnimationFilter::CheckCurveChannelMatch(*CurveName, Channel))
			{
				RawCurvesFiltered.FloatCurves.Push(Curve);
			}
		}
#if WITH_EDITOR
		for (const auto& Curve : RawCurves.TransformCurves)
		{
			const auto CurveName = Curve.Name.DisplayName.ToString();
			if (DMSSimAnimationFilter::CheckCurveChannelMatch(*CurveName, Channel))
			{
				RawCurvesFiltered.TransformCurves.Push(Curve);
				bTransformCurvesAdded = true;
			}
		}
#endif
		UPackage* const Package = CreatePackage(*PackageName);
		Package->FullyLoad();

		UAnimSequence* AnimSequenceFiltered = nullptr;
		if (WithHiddenData)
		{
			AnimSequenceFiltered = DuplicateObject<UAnimSequence>(AnimSequence, Package, *AnimationName);
		}
		else
		{
			AnimSequenceFiltered = NewObject<UAnimSequence>(Package, *AnimationName, RF_Public | RF_Standalone | RF_MarkAsRootSet);
		}
		AnimSequenceFiltered->AddToRoot();
		AnimSequenceFiltered->SetSkeleton(AnimSequence->GetSkeleton());
		AnimSequenceFiltered->SequenceLength = AnimSequence->SequenceLength;
		AnimSequenceFiltered->RawCurveData = RawCurvesFiltered;
#if WITH_EDITOR
		AnimSequenceFiltered->MarkRawDataAsModified();
		// Transform curves need to be extra baked
		if (bTransformCurvesAdded)
		{
			AnimSequenceFiltered->Modify(true);
			AnimSequenceFiltered->BakeTrackCurvesToRawAnimation();
			AnimSequenceFiltered->RequestSyncAnimRecompression(false);
		}
#endif
		AnimSequenceFiltered->PostLoad();

		//AnimSequenceFiltered->UpdateResource();
		Package->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(AnimSequenceFiltered);

		UPackage::SavePackage(Package, AnimSequenceFiltered, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName, GError, nullptr, true, true, SAVE_NoError);
	}

	DMSSimBaseAnimationType GetBaseAnimationType(const char* const Name, const TSet<FString>& AnimationNameSet)
	{
		FString NameStr(Name);
		NameStr.ToLowerInline();
		const size_t Len = NameStr.Len();
		const size_t HeadPostfixLen = strlen(DMSSIM_HEAD_ANIMATION_POSTFIX);
		if (Len < (HeadPostfixLen + 1))
		{
			return DMSSimBaseAnimationHead;
		}

		if (NameStr.Mid(Len - HeadPostfixLen) != DMSSIM_HEAD_ANIMATION_POSTFIX)
		{
			FString HeadNameStr(NameStr);
			HeadNameStr += DMSSIM_HEAD_ANIMATION_POSTFIX;
			if (AnimationNameSet.Find(HeadNameStr)) {
				return DMSSimBaseAnimationBody;
			}
		}
		return DMSSimBaseAnimationHead;
	}

#if WITH_EDITOR
	bool ApplyPitchCorrection(const FString& AssetPath, const float Pitch)
	{
		DMSSimLog::Info() << "Adjusting head pitch (yaw) for " << AssetPath <<  FL;

		bool Result = false;
		const FStringAssetReference Ref(AssetPath);
		UAnimSequence* const AnimSequence = Cast<UAnimSequence>(Ref.TryLoad());
		if (AnimSequence != nullptr)
		{
			
			USkeleton* Skeleton = AnimSequence->GetSkeleton();

			if (Skeleton->GetName() != TEXT("Face_Archetype_Skeleton"))
			{
				DMSSimLog::Info() << "Anim "<< AssetPath  << "Skel " << Skeleton->GetName() << " not supported. Skipping" << FL;
				return false;
			}
			
			FSmartName NewCurveName;
			Skeleton->AddSmartNameAndModify(USkeleton::AnimTrackCurveMappingName, NAME_HEAD_BONE, NewCurveName);

			if (AnimSequence->HasCurveData(NewCurveName.UID,
				true /* inspect only additive curves, ignore compressed data */))
			{
				DMSSimLog::Warn() << "Animation " << AssetPath << " has 'head' curve already. Unpredictable result."  << FL;
			}
		
			// Actually applying yaw change, as head pitch is yaw of the 'head' bone
			const FTransform Transform(FRotator(0.f, Pitch, 0.f));
			AnimSequence->Modify();
			AnimSequence->AddKeyToSequence(0.f, NAME_HEAD_BONE,  Transform);

			// Apply animation
			if (AnimSequence->DoesNeedRebake() || AnimSequence->DoesNeedRecompress())
			{
				if (AnimSequence->DoesNeedRebake())
				{
					AnimSequence->Modify(true);
					AnimSequence->BakeTrackCurvesToRawAnimation();
				}

				if (AnimSequence->DoesNeedRecompress())
				{
					AnimSequence->Modify(true);
					AnimSequence->RequestSyncAnimRecompression(false);
				}
			}


			// Check out animation
			UPackage* Package = AnimSequence->GetPackage();
			const FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
			if (IFileManager::Get().IsReadOnly(*PackageFileName))
			{
				FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*PackageFileName, false);
			}

			// Save
			Result = UPackage::SavePackage(Package, AnimSequence, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName, GError, nullptr, true, true, SAVE_NoError);
		}
		return Result;
	}
#endif // WITH_EDITOR

} // anonymous namespace

void UDMSSimAnimationBuilderBlueprint::BuildDmsAnimations()
{
	DMSSimLog::EnableConsoleOutput(true);

	TSharedPtr<const DMSSimAssetRegistry> AssetRegistry(DMSSimAssetRegistry::Create());
	const DMSSimResourceSet& AnimationSet = AssetRegistry->GetAnimations();

	TSet<FString> AnimationNameSet;

	for (size_t i = 0; i < AnimationSet.GetResourceCount(); ++i)
	{
		FString Name(AnimationSet.GetResourceName(i));
		Name.ToLowerInline();
		if (AnimationNameSet.Find(Name))
		{
			DMSSimLog::Warn() << "DUPLICATE ANIMATION NAME - " << Name << FL;
			continue;
		}
		AnimationNameSet.Add(Name);
	}

	for (size_t i = 0; i < AnimationSet.GetResourceCount(); ++i)
	{
		const char* const Name = AnimationSet.GetResourceName(i);
		const char* const Path = AnimationSet.GetResourcePath(i);

		const DMSSimBaseAnimationType BaseAnimationType = GetBaseAnimationType(Name, AnimationNameSet);

		DMSSimLog::Info() << "("<< i << "/" << AnimationSet.GetResourceCount() <<") " << "Processing animation " << Name << " @ " << Path << FL;
		
		for (const auto& Info : ChannelInfoList)
		{
			const auto ChannelBaseType = DMSSimScenarioParser::GetChannelBaseAnimationType(Info.Type);
			if (!Info.SkipFiltering && (BaseAnimationType == ChannelBaseType))
			{
				BuildFilteredAnimation(Name, Path, Info.Type, Info.Postfix);
			}
		}
	}
}

void UDMSSimAnimationBuilderBlueprint::ApplyIphonePitchCorrectionRecursively(const TArray<FString> Folders, float DesiredHeadPitchOffset)
{
#if WITH_EDITOR
	const FAssetRegistryModule& RegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	const auto& AssetRegistry = RegistryModule.Get();
	
	FARFilter Filter;
	for (auto FolderPath : Folders)
	{
		Filter.PackagePaths.Add(*FolderPath);
		
	}
	Filter.ClassNames.Add(*UAnimSequenceBase::StaticClass()->GetName());
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;

	TArray<FAssetData> Assets;
	const bool Result =  AssetRegistry.GetAssets(Filter, Assets);
	if (Result && Assets.Num() > 0)
	{
		for(const auto Asset: Assets)
		{
			ApplyPitchCorrection(Asset.GetExportTextName(), DesiredHeadPitchOffset);
		}
	}
	else
	{
		DMSSimLog::Error() << "Unable to find assets under folders  " << FString::Join(Folders, TEXT(", ")) << FL;
	}

#else // !WITH_EDITOR
	DMSSimLog::Error() << "The function *UDMSSimAnimationBuilderBlueprint::ApplyIphonePitchCorrectionRecursively* can only be called in editor!" << FL;
#endif // WITH_EDITOR
	
}



UAnimSequenceBase* DMSSimAnimationBuilder::LoadAnimationSequence(const DMSSimAnimationChannelType TargetChannel, const char* const Name, const char* const Path, bool* const ExactMatch)
{
	if (ExactMatch) {
		*ExactMatch = (DMSSimAnimationChannelCommon == TargetChannel);
	}

	if (TargetChannel != DMSSimAnimationChannelCommon)
	{
		FString AnimationName(MakeAnimationName(Name, GetChannelPostfix(TargetChannel)));
		FString PackageName(MakePackageName(AnimationName));
		FStringAssetReference ref(PackageName);
		UAnimSequence* const AnimSequenceFiltered = Cast<UAnimSequence>(ref.TryLoad());
		if (AnimSequenceFiltered)
		{
			if (ExactMatch) {
				*ExactMatch = true;
			}
			return AnimSequenceFiltered;
		}
	}

	FStringAssetReference ref(Path);
	UAnimSequence* const AnimSequence = Cast<UAnimSequence>(ref.TryLoad());
	return AnimSequence;
}
