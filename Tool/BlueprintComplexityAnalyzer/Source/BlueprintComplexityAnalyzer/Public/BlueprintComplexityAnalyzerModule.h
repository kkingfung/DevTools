// Copyright DevTools. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;
class SDockTab;

/**
 * Blueprint Complexity Analyzer エディタモジュール
 */
class FBlueprintComplexityAnalyzerModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	 * 分析パネルタブを開く
	 */
	void OpenAnalyzerTab();

private:
	/** タブ生成コールバック */
	TSharedRef<SDockTab> OnSpawnAnalyzerTab(const FSpawnTabArgs& SpawnTabArgs);

	/** メニュー拡張を登録 */
	void RegisterMenuExtensions();

	/** メニュー拡張を解除 */
	void UnregisterMenuExtensions();

	/** コンテンツブラウザ拡張を登録 */
	void RegisterContentBrowserExtensions();

	/** タブID */
	static const FName AnalyzerTabId;
};
