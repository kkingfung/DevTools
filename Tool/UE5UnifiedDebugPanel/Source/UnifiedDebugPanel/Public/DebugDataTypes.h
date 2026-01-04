// Copyright DevTools. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "DebugDataTypes.generated.h"

/**
 * アクターの基本状態情報
 */
USTRUCT(BlueprintType)
struct UNIFIEDEBUGPANEL_API FActorDebugState
{
	GENERATED_BODY()

	/** アクター名 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FString ActorName;

	/** アクタークラス名 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FString ClassName;

	/** ワールド座標 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FVector Location = FVector::ZeroVector;

	/** 回転 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FRotator Rotation = FRotator::ZeroRotator;

	/** 速度 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FVector Velocity = FVector::ZeroVector;

	/** アクティブかどうか */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	bool bIsActive = false;

	/** ティック有効かどうか */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	bool bIsTickEnabled = false;
};

/**
 * Gameplay Ability の状態情報
 */
USTRUCT(BlueprintType)
struct UNIFIEDEBUGPANEL_API FAbilityDebugInfo
{
	GENERATED_BODY()

	/** アビリティ名 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Ability")
	FString AbilityName;

	/** アビリティクラス名 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Ability")
	FString ClassName;

	/** アビリティタグ */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Ability")
	FGameplayTagContainer AbilityTags;

	/** 現在アクティブか */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Ability")
	bool bIsActive = false;

	/** クールダウン中か */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Ability")
	bool bIsOnCooldown = false;

	/** 残りクールダウン時間 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Ability")
	float CooldownRemaining = 0.0f;

	/** アビリティレベル */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Ability")
	int32 Level = 1;

	/** 入力がバインドされているか */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Ability")
	bool bInputBound = false;

	/** 現在のタスク名（実行中の場合） */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Ability")
	FString CurrentTaskName;
};

/**
 * Gameplay Effect の状態情報
 */
USTRUCT(BlueprintType)
struct UNIFIEDEBUGPANEL_API FEffectDebugInfo
{
	GENERATED_BODY()

	/** エフェクト名 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Effect")
	FString EffectName;

	/** エフェクトタグ */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Effect")
	FGameplayTagContainer EffectTags;

	/** 残り時間（Duration型の場合） */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Effect")
	float RemainingTime = 0.0f;

	/** スタック数 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Effect")
	int32 StackCount = 1;

	/** 適用元のアビリティ名 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Effect")
	FString SourceAbilityName;

	/** Instigator名 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Effect")
	FString InstigatorName;
};

/**
 * Animation Montage の状態情報
 */
USTRUCT(BlueprintType)
struct UNIFIEDEBUGPANEL_API FMontageDebugInfo
{
	GENERATED_BODY()

	/** モンタージュ名 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Animation")
	FString MontageName;

	/** 現在のセクション名 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Animation")
	FString CurrentSectionName;

	/** 再生位置（秒） */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Animation")
	float Position = 0.0f;

	/** 再生レート */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Animation")
	float PlayRate = 1.0f;

	/** ブレンドイン中か */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Animation")
	bool bIsBlendingIn = false;

	/** ブレンドアウト中か */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Animation")
	bool bIsBlendingOut = false;

	/** 残り時間 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Animation")
	float RemainingTime = 0.0f;
};

/**
 * Animation State Machine の状態情報
 */
USTRUCT(BlueprintType)
struct UNIFIEDEBUGPANEL_API FAnimStateMachineDebugInfo
{
	GENERATED_BODY()

	/** ステートマシン名 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Animation")
	FString StateMachineName;

	/** 現在のステート名 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Animation")
	FString CurrentStateName;

	/** ステート滞在時間 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Animation")
	float TimeInState = 0.0f;

	/** 遷移中か */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Animation")
	bool bIsTransitioning = false;

	/** 遷移先ステート名（遷移中の場合） */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Animation")
	FString TransitionTargetState;
};

/**
 * AI Behavior Tree の状態情報
 */
USTRUCT(BlueprintType)
struct UNIFIEDEBUGPANEL_API FBehaviorTreeDebugInfo
{
	GENERATED_BODY()

	/** ビヘイビアツリー名 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|AI")
	FString TreeName;

	/** 現在実行中のノード名 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|AI")
	FString CurrentNodeName;

	/** 現在実行中のノードタイプ */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|AI")
	FString CurrentNodeType;

	/** アクティブなサービス一覧 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|AI")
	TArray<FString> ActiveServices;

	/** 実行パス（ルートから現在ノードまで） */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|AI")
	TArray<FString> ExecutionPath;

	/** ツリーが実行中か */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|AI")
	bool bIsRunning = false;

	/** 最後に失敗したノード名 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|AI")
	FString LastFailedNode;
};

/**
 * Blackboard の状態情報
 */
USTRUCT(BlueprintType)
struct UNIFIEDEBUGPANEL_API FBlackboardDebugInfo
{
	GENERATED_BODY()

	/** キー名と値のペア */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|AI")
	TMap<FString, FString> KeyValues;

	/** 最近変更されたキー */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|AI")
	TArray<FString> RecentlyChangedKeys;
};

/**
 * Tick 統計情報
 */
USTRUCT(BlueprintType)
struct UNIFIEDEBUGPANEL_API FTickDebugInfo
{
	GENERATED_BODY()

	/** コンポーネント/アクター名 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Performance")
	FString Name;

	/** ティックグループ */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Performance")
	FString TickGroup;

	/** 平均ティック時間（ms） */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Performance")
	float AverageTickTime = 0.0f;

	/** 最大ティック時間（ms） */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Performance")
	float MaxTickTime = 0.0f;

	/** ティック有効か */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Performance")
	bool bIsEnabled = false;
};

/**
 * Gameplay Task の状態情報
 */
USTRUCT(BlueprintType)
struct UNIFIEDEBUGPANEL_API FGameplayTaskDebugInfo
{
	GENERATED_BODY()

	/** タスク名 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Task")
	FString TaskName;

	/** タスククラス名 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Task")
	FString ClassName;

	/** タスク状態（Active, Paused, Finished など） */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Task")
	FString State;

	/** 優先度 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Task")
	int32 Priority = 0;

	/** 実行時間 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Task")
	float ElapsedTime = 0.0f;

	/** オーナーアビリティ名 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug|Task")
	FString OwnerAbilityName;
};

/**
 * 監視対象アクターの統合デバッグ情報
 */
USTRUCT(BlueprintType)
struct UNIFIEDEBUGPANEL_API FActorInsightData
{
	GENERATED_BODY()

	/** 監視対象アクターへの弱参照 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	TWeakObjectPtr<AActor> Actor;

	/** 基本状態 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FActorDebugState BasicState;

	/** アクティブなアビリティ一覧 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	TArray<FAbilityDebugInfo> ActiveAbilities;

	/** 付与されているアビリティ一覧 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	TArray<FAbilityDebugInfo> GrantedAbilities;

	/** アクティブなエフェクト一覧 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	TArray<FEffectDebugInfo> ActiveEffects;

	/** 再生中のモンタージュ一覧 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	TArray<FMontageDebugInfo> ActiveMontages;

	/** アニメーションステートマシン情報 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	TArray<FAnimStateMachineDebugInfo> AnimStateMachines;

	/** ビヘイビアツリー情報 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FBehaviorTreeDebugInfo BehaviorTree;

	/** ブラックボード情報 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FBlackboardDebugInfo Blackboard;

	/** ティック情報 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	TArray<FTickDebugInfo> TickInfo;

	/** 実行中のGameplayTask一覧 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	TArray<FGameplayTaskDebugInfo> ActiveTasks;

	/** 保持しているGameplayTags */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FGameplayTagContainer OwnedGameplayTags;

	/** 人間向けの状態サマリー */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	FString HumanReadableSummary;

	/** 最終更新時間 */
	UPROPERTY(BlueprintReadOnly, Category = "Debug")
	float LastUpdateTime = 0.0f;
};
