// Copyright DevTools. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;
class SDockTab;
class FSpawnTabArgs;

/**
 * Gameplay Live Tuning Dashboardモジュール
 */
class FGameplayLiveTuningDashboardModule : public IModuleInterface
{
public:
	// IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** モジュールインスタンスを取得 */
	static FGameplayLiveTuningDashboardModule& Get();

	/** モジュールがロードされているか */
	static bool IsAvailable();

private:
	/** メニュー拡張を登録 */
	void RegisterMenuExtensions();

	/** タブを生成 */
	TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& SpawnTabArgs);

	/** ウィンドウを開く */
	void OpenWindow();

private:
	/** タブマネージャー用ID */
	static const FName TabId;

	/** パネルウィジェット */
	TSharedPtr<class STuningDashboardPanel> DashboardPanel;
};
