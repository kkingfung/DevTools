// Copyright DevTools. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;
class SDockTab;

/**
 * Unified Debug Panel エディタモジュール
 * エディタUI（ドッキングウィンドウ）とメニュー統合を担当
 */
class FUnifiedDebugPanelEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	 * デバッグパネルタブを開く
	 */
	void OpenDebugPanelTab();

private:
	/** タブ生成コールバック */
	TSharedRef<SDockTab> OnSpawnDebugPanelTab(const FSpawnTabArgs& SpawnTabArgs);

	/** メニュー拡張を登録 */
	void RegisterMenuExtensions();

	/** メニュー拡張を解除 */
	void UnregisterMenuExtensions();

	/** ツールメニュー拡張ハンドル */
	FDelegateHandle ToolMenusExtensionHandle;

	/** タブID */
	static const FName DebugPanelTabId;
};
