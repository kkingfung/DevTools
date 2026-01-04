// Copyright DevTools. All Rights Reserved.

using UnrealBuildTool;

public class UnifiedDebugPanel : ModuleRules
{
	public UnifiedDebugPanel(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"Slate",
				"SlateCore",
				"UMG",
				"GameplayAbilities",
				"GameplayTags",
				"GameplayTasks",
				"AIModule",
				"NavigationSystem"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AnimGraphRuntime"
			}
		);

		// GAS (Gameplay Ability System) サポート
		if (Target.bBuildWithEditorOnlyData)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}

		// デバッグビルドでのみ有効な機能
		PublicDefinitions.Add("WITH_UNIFIED_DEBUG_PANEL=1");
	}
}
