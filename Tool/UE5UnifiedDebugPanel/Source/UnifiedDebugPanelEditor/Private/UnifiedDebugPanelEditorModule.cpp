// Copyright DevTools. All Rights Reserved.

#include "UnifiedDebugPanelEditorModule.h"
#include "SUnifiedDebugPanel.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Framework/Docking/TabManager.h"

#define LOCTEXT_NAMESPACE "FUnifiedDebugPanelEditorModule"

const FName FUnifiedDebugPanelEditorModule::DebugPanelTabId(TEXT("UnifiedDebugPanel"));

void FUnifiedDebugPanelEditorModule::StartupModule()
{
	// タブスポナーを登録
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		DebugPanelTabId,
		FOnSpawnTab::CreateRaw(this, &FUnifiedDebugPanelEditorModule::OnSpawnDebugPanelTab)
	)
	.SetDisplayName(LOCTEXT("TabTitle", "Unified Debug Panel"))
	.SetTooltipText(LOCTEXT("TabTooltip", "UE5統合デバッグ＆インサイトパネル - アクターの内部状態を人間の言葉で表示"))
	.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsDebugCategory())
	.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Debug"));

	// メニュー拡張を登録
	RegisterMenuExtensions();

	UE_LOG(LogTemp, Log, TEXT("[UnifiedDebugPanel] Editor module started"));
}

void FUnifiedDebugPanelEditorModule::ShutdownModule()
{
	// メニュー拡張を解除
	UnregisterMenuExtensions();

	// タブスポナーを解除
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(DebugPanelTabId);

	UE_LOG(LogTemp, Log, TEXT("[UnifiedDebugPanel] Editor module shutdown"));
}

void FUnifiedDebugPanelEditorModule::OpenDebugPanelTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(DebugPanelTabId);
}

TSharedRef<SDockTab> FUnifiedDebugPanelEditorModule::OnSpawnDebugPanelTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("TabLabel", "Debug Panel"))
		[
			SNew(SUnifiedDebugPanel)
		];
}

void FUnifiedDebugPanelEditorModule::RegisterMenuExtensions()
{
	// UToolMenus を使用してメニューを拡張
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([this]()
	{
		// Window メニューに追加
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		if (Menu)
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("LevelEditor");
			Section.AddMenuEntry(
				"OpenUnifiedDebugPanel",
				LOCTEXT("MenuEntryTitle", "Unified Debug Panel"),
				LOCTEXT("MenuEntryTooltip", "UE5統合デバッグ＆インサイトパネルを開く"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Debug"),
				FUIAction(FExecuteAction::CreateRaw(this, &FUnifiedDebugPanelEditorModule::OpenDebugPanelTab))
			);
		}

		// ツールバーにも追加（オプション）
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		if (ToolbarMenu)
		{
			FToolMenuSection& ToolbarSection = ToolbarMenu->FindOrAddSection("PluginTools");
			ToolbarSection.AddEntry(FToolMenuEntry::InitToolBarButton(
				"UnifiedDebugPanelButton",
				FUIAction(FExecuteAction::CreateRaw(this, &FUnifiedDebugPanelEditorModule::OpenDebugPanelTab)),
				LOCTEXT("ToolbarButtonLabel", "Debug Panel"),
				LOCTEXT("ToolbarButtonTooltip", "Unified Debug Panelを開く"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Debug")
			));
		}
	}));
}

void FUnifiedDebugPanelEditorModule::UnregisterMenuExtensions()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnifiedDebugPanelEditorModule, UnifiedDebugPanelEditor)
