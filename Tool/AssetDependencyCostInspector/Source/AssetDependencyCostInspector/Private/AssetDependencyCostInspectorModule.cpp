// Copyright DevTools. All Rights Reserved.

#include "AssetDependencyCostInspectorModule.h"
#include "SAssetCostPanel.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Widgets/Docking/SDockTab.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "AssetDependencyCostInspector"

const FName FAssetDependencyCostInspectorModule::TabId("AssetCostInspectorTab");

void FAssetDependencyCostInspectorModule::StartupModule()
{
	// タブスポナー登録
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		TabId,
		FOnSpawnTab::CreateRaw(this, &FAssetDependencyCostInspectorModule::SpawnTab)
	)
	.SetDisplayName(LOCTEXT("TabTitle", "Asset Cost Inspector"))
	.SetTooltipText(LOCTEXT("TabTooltip", "アセットの依存関係とコストを分析"))
	.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory())
	.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Default"));

	// メニュー拡張
	RegisterMenuExtensions();

	// コンテンツブラウザ拡張
	RegisterContentBrowserExtensions();
}

void FAssetDependencyCostInspectorModule::ShutdownModule()
{
	UnregisterMenuExtensions();

	// コンテンツブラウザ拡張解除
	if (FModuleManager::Get().IsModuleLoaded("ContentBrowser"))
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::GetModuleChecked<FContentBrowserModule>("ContentBrowser");
		TArray<FContentBrowserMenuExtender_SelectedAssets>& AssetExtenders = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
		AssetExtenders.RemoveAll([this](const FContentBrowserMenuExtender_SelectedAssets& Delegate)
		{
			return Delegate.GetHandle() == ContentBrowserAssetExtenderDelegateHandle;
		});

		TArray<FContentBrowserMenuExtender_SelectedPaths>& PathExtenders = ContentBrowserModule.GetAllPathViewContextMenuExtenders();
		PathExtenders.RemoveAll([this](const FContentBrowserMenuExtender_SelectedPaths& Delegate)
		{
			return Delegate.GetHandle() == ContentBrowserPathExtenderDelegateHandle;
		});
	}

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabId);
}

FAssetDependencyCostInspectorModule& FAssetDependencyCostInspectorModule::Get()
{
	return FModuleManager::GetModuleChecked<FAssetDependencyCostInspectorModule>("AssetDependencyCostInspector");
}

bool FAssetDependencyCostInspectorModule::IsAvailable()
{
	return FModuleManager::Get().IsModuleLoaded("AssetDependencyCostInspector");
}

void FAssetDependencyCostInspectorModule::RegisterMenuExtensions()
{
	// Tool Menusを使用
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([this]()
	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		FToolMenuSection& Section = Menu->FindOrAddSection("ExperimentalTabSpawners");

		Section.AddMenuEntry(
			"AssetCostInspector",
			LOCTEXT("MenuEntry", "Asset Cost Inspector"),
			LOCTEXT("MenuEntryTooltip", "アセットの依存関係とコストを分析するツールを開く"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Default"),
			FUIAction(FExecuteAction::CreateRaw(this, &FAssetDependencyCostInspectorModule::OpenWindow))
		);
	}));
}

void FAssetDependencyCostInspectorModule::UnregisterMenuExtensions()
{
	UToolMenus::UnregisterOwner(this);
}

void FAssetDependencyCostInspectorModule::RegisterContentBrowserExtensions()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	// アセット右クリックメニュー拡張
	TArray<FContentBrowserMenuExtender_SelectedAssets>& AssetExtenders = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
	AssetExtenders.Add(FContentBrowserMenuExtender_SelectedAssets::CreateLambda(
		[this](const TArray<FAssetData>& SelectedAssets) -> TSharedRef<FExtender>
		{
			TSharedRef<FExtender> Extender = MakeShared<FExtender>();

			if (SelectedAssets.Num() > 0)
			{
				Extender->AddMenuExtension(
					"GetAssetActions",
					EExtensionHook::After,
					nullptr,
					FMenuExtensionDelegate::CreateLambda([this, SelectedAssets](FMenuBuilder& MenuBuilder)
					{
						MenuBuilder.AddMenuEntry(
							LOCTEXT("AnalyzeCost", "コストを分析"),
							LOCTEXT("AnalyzeCostTooltip", "このアセットの依存関係とコストを分析"),
							FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Default"),
							FUIAction(FExecuteAction::CreateRaw(this, &FAssetDependencyCostInspectorModule::AnalyzeSelectedAssets))
						);
					})
				);
			}

			return Extender;
		}
	));
	ContentBrowserAssetExtenderDelegateHandle = AssetExtenders.Last().GetHandle();

	// フォルダ右クリックメニュー拡張
	TArray<FContentBrowserMenuExtender_SelectedPaths>& PathExtenders = ContentBrowserModule.GetAllPathViewContextMenuExtenders();
	PathExtenders.Add(FContentBrowserMenuExtender_SelectedPaths::CreateLambda(
		[this](const TArray<FString>& SelectedPaths) -> TSharedRef<FExtender>
		{
			TSharedRef<FExtender> Extender = MakeShared<FExtender>();

			if (SelectedPaths.Num() > 0)
			{
				Extender->AddMenuExtension(
					"PathContextBulkOperations",
					EExtensionHook::After,
					nullptr,
					FMenuExtensionDelegate::CreateLambda([this](FMenuBuilder& MenuBuilder)
					{
						MenuBuilder.AddMenuEntry(
							LOCTEXT("AnalyzeFolderCost", "フォルダのコストを分析"),
							LOCTEXT("AnalyzeFolderCostTooltip", "このフォルダ内のアセットの依存関係とコストを分析"),
							FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Default"),
							FUIAction(FExecuteAction::CreateRaw(this, &FAssetDependencyCostInspectorModule::AnalyzeSelectedFolder))
						);
					})
				);
			}

			return Extender;
		}
	));
	ContentBrowserPathExtenderDelegateHandle = PathExtenders.Last().GetHandle();
}

TSharedRef<SDockTab> FAssetDependencyCostInspectorModule::SpawnTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SAssignNew(AssetCostPanel, SAssetCostPanel)
		];
}

void FAssetDependencyCostInspectorModule::OpenWindow()
{
	FGlobalTabmanager::Get()->TryInvokeTab(TabId);
}

void FAssetDependencyCostInspectorModule::AnalyzeSelectedAssets()
{
	// ウィンドウを開く
	OpenWindow();

	// 選択アセットを分析
	if (AssetCostPanel.IsValid())
	{
		AssetCostPanel->AnalyzeSelectedAssets();
	}
}

void FAssetDependencyCostInspectorModule::AnalyzeSelectedFolder()
{
	// ウィンドウを開く
	OpenWindow();

	// 選択フォルダを分析
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FString> SelectedPaths;
	ContentBrowserModule.Get().GetSelectedPathViewFolders(SelectedPaths);

	if (SelectedPaths.Num() > 0 && AssetCostPanel.IsValid())
	{
		AssetCostPanel->AnalyzeFolder(SelectedPaths[0]);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAssetDependencyCostInspectorModule, AssetDependencyCostInspector)
