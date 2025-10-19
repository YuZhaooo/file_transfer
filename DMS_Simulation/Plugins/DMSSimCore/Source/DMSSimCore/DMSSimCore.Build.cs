// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class DMSSimCore : ModuleRules
{
	public DMSSimCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bEnableExceptions = true;

		string ConfigPath = Path.Combine(ModuleDirectory, "Public", "config.yml");
		RuntimeDependencies.Add(ConfigPath);

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Projects",
				"Slate",
				"SlateCore",
				"Core",
				"FFmpeg",
				"Yaml",
				"Eigen2",
				"GeometryCache",
				"HairStrandsCore",
				"Rapidjson",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

			if (Target.bBuildEditor == true)
			{
				PublicDependencyModuleNames.Add("RenderCore");
				PublicDependencyModuleNames.Add("RHI");
			}
	}
}
