// Copyright DevTools. All Rights Reserved.

using UnrealBuildTool;

public class BlueprintComplexityAnalyzer : ModuleRules
{
	public BlueprintComplexityAnalyzer(ReadOnlyTargetRules Target) : base(Target)
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
				"BlueprintGraph",
				"Kismet",
				"KismetCompiler",
				"GraphEditor",
				"ToolMenus",
				"WorkspaceMenuStructure",
				"AssetRegistry",
				"ContentBrowser",
				"EditorFramework",
				"LevelEditor",
				"PropertyEditor"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"ToolWidgets",
				"EditorWidgets"
			}
		);
	}
}
