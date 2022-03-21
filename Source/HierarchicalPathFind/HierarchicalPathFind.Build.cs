// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class HierarchicalPathFind : ModuleRules
{
	public HierarchicalPathFind(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "HierarchicalPathFind4UE" });
	}
}
