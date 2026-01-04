// Copyright DevTools. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;
class SDockTab;
class FSpawnTabArgs;

/**
 * Asset Dependency & Cost Inspectorモジュール
 */
class FAssetDependencyCostInspectorModule : public IModuleInterface
{
public:
	// IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** モジュールインスタンスを取得 */
	static FAssetDependencyCostInspectorModule& Get();

	/** モジュールがロードされているか */
	static bool IsAvailable();

private:
	/** メニュー拡張を登録 */
	void RegisterMenuExtensions();

	/** メニュー拡張を解除 */
	void UnregisterMenuExtensions();

	/** コンテンツブラウザ拡張を登録 */
	void RegisterContentBrowserExtensions();

	/** タブを生成 */
	TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& SpawnTabArgs);

	/** メニューバーにメニューを追加 */
	void AddMenuBarExtension(FMenuBarBuilder& Builder);

	/** メニューを構築 */
	void BuildMenu(FMenuBuilder& MenuBuilder);

	/** ウィンドウを開く */
	void OpenWindow();

	/** 選択アセットを分析 */
	void AnalyzeSelectedAssets();

	/** 選択フォルダを分析 */
	void AnalyzeSelectedFolder();

private:
	/** タブマネージャー用ID */
	static const FName TabId;

	/** メニュー拡張ハンドル */
	TSharedPtr<FExtender> MenuExtender;

	/** コンテンツブラウザ拡張ハンドル */
	TArray<FContentBrowserMenuExtender_SelectedAssets> ContentBrowserAssetExtenderDelegates;
	TArray<FContentBrowserMenuExtender_SelectedPaths> ContentBrowserPathExtenderDelegates;
	FDelegateHandle ContentBrowserAssetExtenderDelegateHandle;
	FDelegateHandle ContentBrowserPathExtenderDelegateHandle;

	/** パネルウィジェット */
	TSharedPtr<class SAssetCostPanel> AssetCostPanel;
};
