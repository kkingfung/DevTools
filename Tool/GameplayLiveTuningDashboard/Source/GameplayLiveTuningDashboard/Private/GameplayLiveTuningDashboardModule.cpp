// Copyright DevTools. All Rights Reserved.

#include "GameplayLiveTuningDashboardModule.h"
#include "STuningDashboardPanel.h"
#include "TuningSubsystem.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "GameplayLiveTuningDashboard"

const FName FGameplayLiveTuningDashboardModule::TabId("GameplayLiveTuningDashboardTab");

void FGameplayLiveTuningDashboardModule::StartupModule()
{
	// タブスポナー登録
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		TabId,
		FOnSpawnTab::CreateRaw(this, &FGameplayLiveTuningDashboardModule::SpawnTab)
	)
	.SetDisplayName(LOCTEXT("TabTitle", "Live Tuning Dashboard"))
	.SetTooltipText(LOCTEXT("TabTooltip", "ゲームプレイパラメータをリアルタイムで調整"))
	.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory())
	.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Default"));

	// メニュー拡張
	RegisterMenuExtensions();

	UE_LOG(LogTemp, Log, TEXT("[GameplayLiveTuningDashboard] Module started"));
}

void FGameplayLiveTuningDashboardModule::ShutdownModule()
{
	UToolMenus::UnregisterOwner(this);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabId);

	UE_LOG(LogTemp, Log, TEXT("[GameplayLiveTuningDashboard] Module shutdown"));
}

FGameplayLiveTuningDashboardModule& FGameplayLiveTuningDashboardModule::Get()
{
	return FModuleManager::GetModuleChecked<FGameplayLiveTuningDashboardModule>("GameplayLiveTuningDashboard");
}

bool FGameplayLiveTuningDashboardModule::IsAvailable()
{
	return FModuleManager::Get().IsModuleLoaded("GameplayLiveTuningDashboard");
}

void FGameplayLiveTuningDashboardModule::RegisterMenuExtensions()
{
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([this]()
	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		FToolMenuSection& Section = Menu->FindOrAddSection("ExperimentalTabSpawners");

		Section.AddMenuEntry(
			"LiveTuningDashboard",
			LOCTEXT("MenuEntry", "Live Tuning Dashboard"),
			LOCTEXT("MenuEntryTooltip", "ゲームプレイパラメータをリアルタイムで調整するダッシュボードを開く"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Default"),
			FUIAction(FExecuteAction::CreateRaw(this, &FGameplayLiveTuningDashboardModule::OpenWindow))
		);
	}));
}

TSharedRef<SDockTab> FGameplayLiveTuningDashboardModule::SpawnTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SAssignNew(DashboardPanel, STuningDashboardPanel)
		];
}

void FGameplayLiveTuningDashboardModule::OpenWindow()
{
	FGlobalTabmanager::Get()->TryInvokeTab(TabId);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGameplayLiveTuningDashboardModule, GameplayLiveTuningDashboard)
