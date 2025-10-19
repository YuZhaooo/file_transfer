// Copyright Epic Games, Inc. All Rights Reserved.

#include "DMSSimCore.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"

namespace
{
const TCHAR* DLLS[] = { // Order is important due to dependencies between DLLs
				TEXT("avutil-57.dll"),
				TEXT("swresample-4.dll"),
				TEXT("avcodec-59.dll"),
				TEXT("swscale-6.dll"),
				TEXT("avformat-59.dll"),
				TEXT("avfilter-8.dll"),
				TEXT("avdevice-59.dll"),
};

TArray<void*> DllHandles;
}

void FDMSSimCoreModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FString BaseDir = IPluginManager::Get().FindPlugin("DMSSimCore")->GetBaseDir();
	for (auto Dll : DLLS)
	{
		const auto LibPath = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/ffmpeg/bin"), Dll);
		if (!LibPath.IsEmpty())
		{
			void* const handle = FPlatformProcess::GetDllHandle(*LibPath);
			if (handle != nullptr)
			{
				DllHandles.Push(handle); // Handle as a LIFO
			}
		}
	}
}

void FDMSSimCoreModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	while (DllHandles.Num() > 0)
	{
		const auto handle = DllHandles.Pop();
		FPlatformProcess::FreeDllHandle(handle);
	}
	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FDMSSimCoreModule, DMSSimCore)