// Copyright DevTools. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DebugDataTypes.h"
#include "DebugDataCollectorSubsystem.generated.h"

class UAbilitySystemComponent;
class UBehaviorTreeComponent;
class UBlackboardComponent;
class USkeletalMeshComponent;
class UAnimInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorInsightUpdated, const FActorInsightData&, InsightData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWatchedActorAdded, AActor*, Actor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWatchedActorRemoved, AActor*, Actor);

/**
 * デバッグデータ収集サブシステム
 * ワールド内のアクターからデバッグ情報を収集し、統合表示用のデータを提供
 */
UCLASS()
class UNIFIEDEBUGPANEL_API UDebugDataCollectorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** サブシステム初期化 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** サブシステム終了 */
	virtual void Deinitialize() override;

	/** ティック可能か */
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

	/** ティック処理 */
	virtual void Tick(float DeltaTime);

	// ========== 監視対象管理 ==========

	/**
	 * アクターを監視対象に追加
	 * @param Actor 監視対象のアクター
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel")
	void WatchActor(AActor* Actor);

	/**
	 * アクターを監視対象から削除
	 * @param Actor 監視を解除するアクター
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel")
	void UnwatchActor(AActor* Actor);

	/**
	 * 全ての監視を解除
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel")
	void ClearAllWatches();

	/**
	 * プレイヤーが操作中のPawnを自動監視
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel")
	void WatchPlayerPawn(int32 PlayerIndex = 0);

	/**
	 * 指定タグを持つアクターを全て監視
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel")
	void WatchActorsWithTag(FName Tag);

	// ========== データ取得 ==========

	/**
	 * 指定アクターのInsightデータを取得
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel")
	bool GetActorInsight(AActor* Actor, FActorInsightData& OutData) const;

	/**
	 * 全ての監視対象のInsightデータを取得
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel")
	TArray<FActorInsightData> GetAllInsightData() const;

	/**
	 * 監視対象アクター一覧を取得
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel")
	TArray<AActor*> GetWatchedActors() const;

	/**
	 * 監視対象数を取得
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel")
	int32 GetWatchedActorCount() const { return WatchedActors.Num(); }

	// ========== 設定 ==========

	/**
	 * 更新間隔を設定（秒）
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel")
	void SetUpdateInterval(float Interval) { UpdateInterval = FMath::Max(0.016f, Interval); }

	/**
	 * デバッグ情報収集を有効/無効化
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel")
	void SetEnabled(bool bEnable) { bIsEnabled = bEnable; }

	/**
	 * 有効かどうか取得
	 */
	UFUNCTION(BlueprintCallable, Category = "UnifiedDebugPanel")
	bool IsEnabled() const { return bIsEnabled; }

	// ========== イベント ==========

	/** Insightデータ更新時のイベント */
	UPROPERTY(BlueprintAssignable, Category = "UnifiedDebugPanel")
	FOnActorInsightUpdated OnActorInsightUpdated;

	/** 監視対象追加時のイベント */
	UPROPERTY(BlueprintAssignable, Category = "UnifiedDebugPanel")
	FOnWatchedActorAdded OnWatchedActorAdded;

	/** 監視対象削除時のイベント */
	UPROPERTY(BlueprintAssignable, Category = "UnifiedDebugPanel")
	FOnWatchedActorRemoved OnWatchedActorRemoved;

protected:
	/** ティック登録用のデリゲートハンドル */
	FDelegateHandle TickDelegateHandle;

	/** 監視対象アクター一覧 */
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> WatchedActors;

	/** キャッシュされたInsightデータ */
	UPROPERTY()
	TMap<TWeakObjectPtr<AActor>, FActorInsightData> CachedInsightData;

	/** 更新間隔 */
	float UpdateInterval = 0.1f;

	/** 前回更新からの経過時間 */
	float TimeSinceLastUpdate = 0.0f;

	/** 有効フラグ */
	bool bIsEnabled = true;

	// ========== データ収集メソッド ==========

	/**
	 * アクターの全デバッグ情報を収集
	 */
	FActorInsightData CollectActorInsight(AActor* Actor);

	/**
	 * 基本状態を収集
	 */
	FActorDebugState CollectBasicState(AActor* Actor);

	/**
	 * Gameplay Ability System 情報を収集
	 */
	void CollectAbilitySystemData(UAbilitySystemComponent* ASC, FActorInsightData& OutData);

	/**
	 * Animation 情報を収集
	 */
	void CollectAnimationData(USkeletalMeshComponent* SkelMesh, FActorInsightData& OutData);

	/**
	 * AI (Behavior Tree) 情報を収集
	 */
	void CollectBehaviorTreeData(UBehaviorTreeComponent* BTC, FActorInsightData& OutData);

	/**
	 * Blackboard 情報を収集
	 */
	void CollectBlackboardData(UBlackboardComponent* BBC, FActorInsightData& OutData);

	/**
	 * Tick 統計情報を収集
	 */
	void CollectTickData(AActor* Actor, FActorInsightData& OutData);

	/**
	 * 人間向けサマリーを生成
	 */
	FString GenerateHumanReadableSummary(const FActorInsightData& Data);

	/**
	 * 無効になったアクターをクリーンアップ
	 */
	void CleanupInvalidActors();
};
