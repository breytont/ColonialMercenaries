// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ColonialMercenaries : ModuleRules
{
	public ColonialMercenaries(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ColonialMercenaries",
			"ColonialMercenaries/Variant_Platforming",
			"ColonialMercenaries/Variant_Platforming/Animation",
			"ColonialMercenaries/Variant_Combat",
			"ColonialMercenaries/Variant_Combat/AI",
			"ColonialMercenaries/Variant_Combat/Animation",
			"ColonialMercenaries/Variant_Combat/Gameplay",
			"ColonialMercenaries/Variant_Combat/Interfaces",
			"ColonialMercenaries/Variant_Combat/UI",
			"ColonialMercenaries/Variant_SideScrolling",
			"ColonialMercenaries/Variant_SideScrolling/AI",
			"ColonialMercenaries/Variant_SideScrolling/Gameplay",
			"ColonialMercenaries/Variant_SideScrolling/Interfaces",
			"ColonialMercenaries/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
