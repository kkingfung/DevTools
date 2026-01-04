// Copyright DevTools. All Rights Reserved.

#include "UnifiedDebugPanelModule.h"

#define LOCTEXT_NAMESPACE "FUnifiedDebugPanelModule"

void FUnifiedDebugPanelModule::StartupModule()
{
	// モジュール起動時の初期化
	UE_LOG(LogTemp, Log, TEXT("[UnifiedDebugPanel] Runtime module started"));
}

void FUnifiedDebugPanelModule::ShutdownModule()
{
	// モジュール終了時のクリーンアップ
	UE_LOG(LogTemp, Log, TEXT("[UnifiedDebugPanel] Runtime module shutdown"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnifiedDebugPanelModule, UnifiedDebugPanel)
