// Copyright DevTools. All Rights Reserved.

using UnrealBuildTool;

public class GameplayLiveTuningDashboard : ModuleRules
{
	public GameplayLiveTuningDashboard(ReadOnlyTargetRules Target) : base(Target)
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
				"Slate",
				"SlateCore",
				"InputCore",
				"UnrealEd",
				"EditorStyle",
				"ToolMenus",
				"WorkspaceMenuStructure",
				"EditorFramework",
				"LevelEditor",
				"PropertyEditor",
				"Json",
				"JsonUtilities"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"ToolWidgets",
				"EditorWidgets",
				"GameplayTags"
			}
		);
	}
}
