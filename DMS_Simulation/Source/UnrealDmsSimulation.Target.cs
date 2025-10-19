// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UnrealDmsSimulationTarget : TargetRules
{
	public UnrealDmsSimulationTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.AddRange( new string[] { "UnrealDmsSimulation" } );

		// This instructs UBT to build an additional executable with the CONSOLE subsystem
		bBuildAdditionalConsoleApp = true;
	}
}
