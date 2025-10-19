#include "DmsDeviceProfileSelectorModule.h"
#include "Modules/ModuleManager.h"
#include "RHI.h"
#include "RHICommandList.h"
#include "DynamicRHI.h"
#include "Misc/ConfigCacheIni.h"

IMPLEMENT_MODULE(FDMSDeviceProfileSelectorModule, DMSDeviceProfileSelector);

FDMSDeviceProfileSelectorModule::~FDMSDeviceProfileSelectorModule()
{	
}

void FDMSDeviceProfileSelectorModule::StartupModule()
{
}

void FDMSDeviceProfileSelectorModule::ShutdownModule()
{
}

const FString FDMSDeviceProfileSelectorModule::GetRuntimeDeviceProfileName()
{
	FString ProfileName = FPlatformProperties::PlatformName();

	const TCHAR* const CommandLine = FCommandLine::Get();
	if (CommandLine)
	{
		TArray<FString> Tokens;
		TArray<FString> Switches;
		FCommandLine::Parse(CommandLine, Tokens, Switches);

		if (Tokens.Num() >= 2)
		{
			FString ProfileCandidate;
			for (size_t i = 0; i < Tokens.Num() - 1; ++i)
			{
				const auto& ArgSwitch = Tokens[i];
				const auto& ArgValue = Tokens[i + 1];
				if (ArgSwitch == TEXT("p"))
				{
					ProfileCandidate = ArgValue;
				}
			}

			if (!ProfileCandidate.IsEmpty() && FApp::CanEverRender())
			{
				FString DeviceProfileFileName;
				FConfigCacheIni::LoadGlobalIniFile(DeviceProfileFileName, TEXT("DeviceProfiles"));

				TArray<FString> AvailableProfiles;
				GConfig->GetSectionNames(DeviceProfileFileName, AvailableProfiles);

				if (AvailableProfiles.Contains(FString::Printf(TEXT("%s DeviceProfile"), *ProfileCandidate)))
				{
					ProfileName = MoveTemp(ProfileCandidate);
				}
			}
		}
	}

	UE_LOG(LogInit, Log, TEXT("Selected Device Profile: [%s]"), *ProfileName);
	return ProfileName;
}

