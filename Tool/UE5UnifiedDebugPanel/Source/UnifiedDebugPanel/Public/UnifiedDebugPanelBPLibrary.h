// Copyright DevTools. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DebugDataTypes.h"
#include "UnifiedDebugPanelBPLibrary.generated.h"

class UDebugDataCollectorSubsystem;

/**
 * Unified Debug Panel Blueprint 関数ライブラリ
 * BPからデバッグパネル機能にアクセスするためのユーティリティ関数を提供
 */
UCLASS()
class UNIFIEDEBUGPANEL_API UUnifiedDebugPanelBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ========== 監視対象管理 ==========

	/**
	 * アクターを監視対象に追加
	 * @param WorldContextObject ワールドコンテキスト
	 * @param Actor 監視対象のアクター
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel", meta = (WorldContext = "WorldContextObject"))
	static void WatchActor(const UObject* WorldContextObject, AActor* Actor);

	/**
	 * アクターを監視対象から削除
	 * @param WorldContextObject ワールドコンテキスト
	 * @param Actor 監視を解除するアクター
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel", meta = (WorldContext = "WorldContextObject"))
	static void UnwatchActor(const UObject* WorldContextObject, AActor* Actor);

	/**
	 * 全ての監視を解除
	 * @param WorldContextObject ワールドコンテキスト
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel", meta = (WorldContext = "WorldContextObject"))
	static void ClearAllWatches(const UObject* WorldContextObject);

	/**
	 * プレイヤーのPawnを監視対象に追加
	 * @param WorldContextObject ワールドコンテキスト
	 * @param PlayerIndex プレイヤーインデックス（デフォルト: 0）
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel", meta = (WorldContext = "WorldContextObject"))
	static void WatchPlayerPawn(const UObject* WorldContextObject, int32 PlayerIndex = 0);

	/**
	 * 指定タグを持つアクターを全て監視
	 * @param WorldContextObject ワールドコンテキスト
	 * @param Tag 監視対象のタグ
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel", meta = (WorldContext = "WorldContextObject"))
	static void WatchActorsWithTag(const UObject* WorldContextObject, FName Tag);

	// ========== データ取得 ==========

	/**
	 * 指定アクターのInsightデータを取得
	 * @param WorldContextObject ワールドコンテキスト
	 * @param Actor 対象アクター
	 * @param OutData 取得したデータ
	 * @return データが存在する場合true
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel", meta = (WorldContext = "WorldContextObject"))
	static bool GetActorInsight(const UObject* WorldContextObject, AActor* Actor, FActorInsightData& OutData);

	/**
	 * 全ての監視対象のInsightデータを取得
	 * @param WorldContextObject ワールドコンテキスト
	 * @return Insightデータの配列
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel", meta = (WorldContext = "WorldContextObject"))
	static TArray<FActorInsightData> GetAllInsightData(const UObject* WorldContextObject);

	/**
	 * 監視対象アクター一覧を取得
	 * @param WorldContextObject ワールドコンテキスト
	 * @return 監視対象アクターの配列
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel", meta = (WorldContext = "WorldContextObject"))
	static TArray<AActor*> GetWatchedActors(const UObject* WorldContextObject);

	/**
	 * 監視対象数を取得
	 * @param WorldContextObject ワールドコンテキスト
	 * @return 監視対象アクター数
	 */
	UFUNCTION(BlueprintPure, Category = "UnifiedDebugPanel", meta = (WorldContext = "WorldContextObject"))
	static int32 GetWatchedActorCount(const UObject* WorldContextObject);

	// ========== 設定 ==========

	/**
	 * 更新間隔を設定（秒）
	 * @param WorldContextObject ワールドコンテキスト
	 * @param Interval 更新間隔（秒）
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel", meta = (WorldContext = "WorldContextObject"))
	static void SetUpdateInterval(const UObject* WorldContextObject, float Interval);

	/**
	 * デバッグ情報収集を有効/無効化
	 * @param WorldContextObject ワールドコンテキスト
	 * @param bEnable 有効にする場合true
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel", meta = (WorldContext = "WorldContextObject"))
	static void SetEnabled(const UObject* WorldContextObject, bool bEnable);

	/**
	 * デバッグ情報収集が有効かどうか取得
	 * @param WorldContextObject ワールドコンテキスト
	 * @return 有効な場合true
	 */
	UFUNCTION(BlueprintPure, Category = "UnifiedDebugPanel", meta = (WorldContext = "WorldContextObject"))
	static bool IsEnabled(const UObject* WorldContextObject);

	// ========== ユーティリティ ==========

	/**
	 * アクターの人間向けサマリーを取得
	 * @param WorldContextObject ワールドコンテキスト
	 * @param Actor 対象アクター
	 * @return 人間向けのサマリー文字列
	 */
	UFUNCTION(BlueprintPure, Category = "UnifiedDebugPanel", meta = (WorldContext = "WorldContextObject"))
	static FString GetActorSummary(const UObject* WorldContextObject, AActor* Actor);

	/**
	 * アクターがアビリティを実行中か確認
	 * @param WorldContextObject ワールドコンテキスト
	 * @param Actor 対象アクター
	 * @return アビリティ実行中の場合true
	 */
	UFUNCTION(BlueprintPure, Category = "UnifiedDebugPanel", meta = (WorldContext = "WorldContextObject"))
	static bool IsActorExecutingAbility(const UObject* WorldContextObject, AActor* Actor);

	/**
	 * アクターがモンタージュを再生中か確認
	 * @param WorldContextObject ワールドコンテキスト
	 * @param Actor 対象アクター
	 * @return モンタージュ再生中の場合true
	 */
	UFUNCTION(BlueprintPure, Category = "UnifiedDebugPanel", meta = (WorldContext = "WorldContextObject"))
	static bool IsActorPlayingMontage(const UObject* WorldContextObject, AActor* Actor);

	/**
	 * アクターのアクティブなエフェクト数を取得
	 * @param WorldContextObject ワールドコンテキスト
	 * @param Actor 対象アクター
	 * @return アクティブなGameplayEffect数
	 */
	UFUNCTION(BlueprintPure, Category = "UnifiedDebugPanel", meta = (WorldContext = "WorldContextObject"))
	static int32 GetActiveEffectCount(const UObject* WorldContextObject, AActor* Actor);

	// ========== 画面表示 ==========

	/**
	 * 画面にInsightサマリーをオーバーレイ表示
	 * @param WorldContextObject ワールドコンテキスト
	 * @param Actor 対象アクター
	 * @param Duration 表示時間（秒）
	 * @param Color 表示色
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel", meta = (WorldContext = "WorldContextObject"))
	static void DisplayActorInsightOnScreen(const UObject* WorldContextObject, AActor* Actor, float Duration = 0.0f, FColor Color = FColor::Green);

private:
	/**
	 * ワールドからサブシステムを取得
	 */
	static UDebugDataCollectorSubsystem* GetSubsystem(const UObject* WorldContextObject);
};
