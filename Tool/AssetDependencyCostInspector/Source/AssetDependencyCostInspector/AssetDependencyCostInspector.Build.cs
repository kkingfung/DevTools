// Copyright DevTools. All Rights Reserved.

using UnrealBuildTool;

public class AssetDependencyCostInspector : ModuleRules
{
	public AssetDependencyCostInspector(ReadOnlyTargetRules Target) : base(Target)
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
				"AssetRegistry",
				"ContentBrowser",
				"ToolMenus",
				"WorkspaceMenuStructure",
				"EditorFramework",
				"LevelEditor",
				"PropertyEditor",
				"Projects"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"ToolWidgets",
				"EditorWidgets",
				"RenderCore",
				"RHI"
			}
		);

		// Nanite/Lumen関連
		if (Target.bBuildWithEditorOnlyData)
		{
			PrivateDependencyModuleNames.Add("DerivedDataCache");
		}
	}
}
