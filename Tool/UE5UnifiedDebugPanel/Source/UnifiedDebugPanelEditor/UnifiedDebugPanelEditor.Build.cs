// Copyright DevTools. All Rights Reserved.

using UnrealBuildTool;

public class UnifiedDebugPanelEditor : ModuleRules
{
	public UnifiedDebugPanelEditor(ReadOnlyTargetRules Target) : base(Target)
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
				"WorkspaceMenuStructure",
				"ToolMenus",
				"UnifiedDebugPanel"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"EditorFramework",
				"LevelEditor"
			}
		);
	}
}
