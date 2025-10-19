// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class FFmpeg : ModuleRules
{
	public FFmpeg(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "include"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "include"));
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string[] libs =
			{
				"avcodec-59",
				"avdevice-59",
				"avfilter-8",
				"avformat-59",
				"avutil-57",
				"swresample-4",
				"swscale-6"
			};

			PublicRuntimeLibraryPaths.Add(Path.Combine(ModuleDirectory, "bin"));

			foreach (string lib in libs)
			{
				string dllFile = lib + ".dll";
				PublicDelayLoadDLLs.Add(dllFile);
				string soPath = Path.Combine(ModuleDirectory, "bin", dllFile);
				RuntimeDependencies.Add(soPath);
				PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "import", lib + ".lib"));
			}

		}
	}
}
