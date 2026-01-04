// Copyright DevTools. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * Unified Debug Panel ランタイムモジュール
 * ゲーム実行中のデバッグ情報収集と表示を担当
 */
class FUnifiedDebugPanelModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	 * モジュールのシングルトンインスタンスを取得
	 */
	static FUnifiedDebugPanelModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FUnifiedDebugPanelModule>("UnifiedDebugPanel");
	}

	/**
	 * モジュールがロードされているかチェック
	 */
	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("UnifiedDebugPanel");
	}
};
