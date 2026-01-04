// Copyright DevTools. All Rights Reserved.

#include "BlueprintComplexityAnalyzerModule.h"
#include "SBPComplexityPanel.h"
#include "BPComplexityAnalyzer.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Framework/Docking/TabManager.h"
#include "ContentBrowserModule.h"
#include "ContentBrowserDelegates.h"
#include "Engine/Blueprint.h"

#define LOCTEXT_NAMESPACE "FBlueprintComplexityAnalyzerModule"

const FName FBlueprintComplexityAnalyzerModule::AnalyzerTabId(TEXT("BlueprintComplexityAnalyzer"));

void FBlueprintComplexityAnalyzerModule::StartupModule()
{
	// タブスポナーを登録
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		AnalyzerTabId,
		FOnSpawnTab::CreateRaw(this, &FBlueprintComplexityAnalyzerModule::OnSpawnAnalyzerTab)
	)
	.SetDisplayName(LOCTEXT("TabTitle", "BP Complexity Analyzer"))
	.SetTooltipText(LOCTEXT("TabTooltip", "Blueprint複雑度アナライザー - BPの健全性を信号機表示で可視化"))
	.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsDebugCategory())
	.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Blueprint"));

	// メニュー拡張を登録
	RegisterMenuExtensions();

	// コンテンツブラウザ拡張を登録
	RegisterContentBrowserExtensions();

	UE_LOG(LogTemp, Log, TEXT("[BlueprintComplexityAnalyzer] Module started"));
}

void FBlueprintComplexityAnalyzerModule::ShutdownModule()
{
	// メニュー拡張を解除
	UnregisterMenuExtensions();

	// タブスポナーを解除
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AnalyzerTabId);

	UE_LOG(LogTemp, Log, TEXT("[BlueprintComplexityAnalyzer] Module shutdown"));
}

void FBlueprintComplexityAnalyzerModule::OpenAnalyzerTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(AnalyzerTabId);
}

TSharedRef<SDockTab> FBlueprintComplexityAnalyzerModule::OnSpawnAnalyzerTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("TabLabel", "BP Complexity"))
		[
			SNew(SBPComplexityPanel)
		];
}

void FBlueprintComplexityAnalyzerModule::RegisterMenuExtensions()
{
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([this]()
	{
		// Window メニューに追加
		UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		if (WindowMenu)
		{
			FToolMenuSection& Section = WindowMenu->FindOrAddSection("LevelEditor");
			Section.AddMenuEntry(
				"OpenBPComplexityAnalyzer",
				LOCTEXT("MenuEntryTitle", "BP Complexity Analyzer"),
				LOCTEXT("MenuEntryTooltip", "Blueprint複雑度アナライザーを開く"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Blueprint"),
				FUIAction(FExecuteAction::CreateRaw(this, &FBlueprintComplexityAnalyzerModule::OpenAnalyzerTab))
			);
		}

		// Blueprintエディタのメニューにも追加
		UToolMenu* BlueprintMenu = UToolMenus::Get()->ExtendMenu("AssetEditor.BlueprintEditor.MainMenu.Asset");
		if (BlueprintMenu)
		{
			FToolMenuSection& Section = BlueprintMenu->FindOrAddSection("BlueprintComplexity");
			Section.AddMenuEntry(
				"AnalyzeBPComplexity",
				LOCTEXT("AnalyzeMenuTitle", "Analyze Complexity"),
				LOCTEXT("AnalyzeMenuTooltip", "このBlueprintの複雑度を分析"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Info"),
				FUIAction(FExecuteAction::CreateRaw(this, &FBlueprintComplexityAnalyzerModule::OpenAnalyzerTab))
			);
		}
	}));
}

void FBlueprintComplexityAnalyzerModule::UnregisterMenuExtensions()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
}

void FBlueprintComplexityAnalyzerModule::RegisterContentBrowserExtensions()
{
	// コンテンツブラウザの右クリックメニューに追加
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	// アセット選択時の拡張
	TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();

	CBMenuExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedAssets::CreateLambda(
		[this](const TArray<FAssetData>& SelectedAssets) -> TSharedRef<FExtender>
		{
			TSharedRef<FExtender> Extender = MakeShared<FExtender>();

			// Blueprintが選択されているかチェック
			bool bHasBlueprint = false;
			for (const FAssetData& Asset : SelectedAssets)
			{
				if (Asset.AssetClassPath == UBlueprint::StaticClass()->GetClassPathName())
				{
					bHasBlueprint = true;
					break;
				}
			}

			if (bHasBlueprint)
			{
				Extender->AddMenuExtension(
					"GetAssetActions",
					EExtensionHook::After,
					nullptr,
					FMenuExtensionDelegate::CreateLambda([this](FMenuBuilder& MenuBuilder)
					{
						MenuBuilder.AddMenuEntry(
							LOCTEXT("AnalyzeComplexity", "Analyze BP Complexity"),
							LOCTEXT("AnalyzeComplexityTooltip", "選択したBlueprintの複雑度を分析"),
							FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Info"),
							FUIAction(FExecuteAction::CreateRaw(this, &FBlueprintComplexityAnalyzerModule::OpenAnalyzerTab))
						);
					})
				);
			}

			return Extender;
		}
	));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBlueprintComplexityAnalyzerModule, BlueprintComplexityAnalyzer)
