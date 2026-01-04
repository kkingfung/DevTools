// Copyright DevTools. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BPComplexityTypes.generated.h"

/**
 * 健全性レベル（信号機表示用）
 */
UENUM(BlueprintType)
enum class EBPHealthLevel : uint8
{
	/** 健全: 問題なし */
	Green UMETA(DisplayName = "Green - Healthy"),

	/** 警告: 注意が必要 */
	Yellow UMETA(DisplayName = "Yellow - Warning"),

	/** 危険: 即座に対応が必要 */
	Red UMETA(DisplayName = "Red - Critical")
};

/**
 * ノードカテゴリ別カウント
 */
USTRUCT(BlueprintType)
struct BLUEPRINTCOMPLEXITYANALYZER_API FBPNodeCategoryCount
{
	GENERATED_BODY()

	/** カテゴリ名 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FString CategoryName;

	/** ノード数 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 Count = 0;

	/** 全体に占める割合 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	float Percentage = 0.0f;
};

/**
 * ノード数メトリクス
 */
USTRUCT(BlueprintType)
struct BLUEPRINTCOMPLEXITYANALYZER_API FBPNodeMetrics
{
	GENERATED_BODY()

	/** 総ノード数 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 TotalNodeCount = 0;

	/** 関数ノード数 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 FunctionCallCount = 0;

	/** 変数アクセスノード数 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 VariableAccessCount = 0;

	/** 制御フローノード数（Branch, ForLoop等） */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 ControlFlowCount = 0;

	/** 数学/演算ノード数 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 MathOperationCount = 0;

	/** イベントノード数 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 EventNodeCount = 0;

	/** マクロノード数 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 MacroCount = 0;

	/** カスタムイベント数 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 CustomEventCount = 0;

	/** 最大のグラフのノード数 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 LargestGraphNodeCount = 0;

	/** 最大グラフ名 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FString LargestGraphName;

	/** カテゴリ別詳細 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	TArray<FBPNodeCategoryCount> CategoryBreakdown;

	/** 健全性レベル */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	EBPHealthLevel HealthLevel = EBPHealthLevel::Green;

	/** スコア（0-100、低いほど良い） */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	float ComplexityScore = 0.0f;
};

/**
 * 依存関係情報
 */
USTRUCT(BlueprintType)
struct BLUEPRINTCOMPLEXITYANALYZER_API FBPDependencyInfo
{
	GENERATED_BODY()

	/** 依存先アセットパス */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FString AssetPath;

	/** 依存先クラス名 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FString ClassName;

	/** 参照回数 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 ReferenceCount = 0;

	/** 循環参照かどうか */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	bool bIsCircular = false;
};

/**
 * 依存深度メトリクス
 */
USTRUCT(BlueprintType)
struct BLUEPRINTCOMPLEXITYANALYZER_API FBPDependencyMetrics
{
	GENERATED_BODY()

	/** 直接依存数 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 DirectDependencyCount = 0;

	/** 間接依存数（推移的依存） */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 TransitiveDependencyCount = 0;

	/** 依存深度（最大チェーン長） */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 MaxDependencyDepth = 0;

	/** 循環参照数 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 CircularReferenceCount = 0;

	/** 依存詳細リスト */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	TArray<FBPDependencyInfo> Dependencies;

	/** 循環参照の詳細 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	TArray<FString> CircularReferencePaths;

	/** 健全性レベル */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	EBPHealthLevel HealthLevel = EBPHealthLevel::Green;

	/** スコア（0-100、低いほど良い） */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	float ComplexityScore = 0.0f;
};

/**
 * Tick使用情報
 */
USTRUCT(BlueprintType)
struct BLUEPRINTCOMPLEXITYANALYZER_API FBPTickInfo
{
	GENERATED_BODY()

	/** グラフ名（EventTick がある場所） */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FString GraphName;

	/** Tick内のノード数 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 NodeCountInTick = 0;

	/** Tick内で呼ばれる関数リスト */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	TArray<FString> FunctionsCalledInTick;

	/** 重い処理の警告 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	TArray<FString> HeavyOperationWarnings;
};

/**
 * Tick使用メトリクス
 */
USTRUCT(BlueprintType)
struct BLUEPRINTCOMPLEXITYANALYZER_API FBPTickMetrics
{
	GENERATED_BODY()

	/** Tick使用しているか */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	bool bUsesTick = false;

	/** Tickイベント数 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 TickEventCount = 0;

	/** Tick内の総ノード数 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 TotalNodesInTick = 0;

	/** Tick詳細情報 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	TArray<FBPTickInfo> TickDetails;

	/** 最適化推奨事項 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	TArray<FString> OptimizationSuggestions;

	/** 健全性レベル */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	EBPHealthLevel HealthLevel = EBPHealthLevel::Green;

	/** スコア（0-100、低いほど良い） */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	float ComplexityScore = 0.0f;
};

/**
 * C++化推奨度情報
 */
USTRUCT(BlueprintType)
struct BLUEPRINTCOMPLEXITYANALYZER_API FBPCppMigrationMetrics
{
	GENERATED_BODY()

	/** C++化推奨度（0-100、高いほど推奨） */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	float MigrationScore = 0.0f;

	/** 推奨理由リスト */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	TArray<FString> Reasons;

	/** C++化の難易度（1-5、高いほど難しい） */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 MigrationDifficulty = 1;

	/** C++化で改善される項目 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	TArray<FString> ExpectedImprovements;

	/** C++化推奨の優先度 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	EBPHealthLevel Priority = EBPHealthLevel::Green;
};

/**
 * 問題点詳細
 */
USTRUCT(BlueprintType)
struct BLUEPRINTCOMPLEXITYANALYZER_API FBPIssue
{
	GENERATED_BODY()

	/** 問題のカテゴリ */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FString Category;

	/** 問題の説明 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FString Description;

	/** 深刻度 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	EBPHealthLevel Severity = EBPHealthLevel::Yellow;

	/** 該当箇所（グラフ名など） */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FString Location;

	/** 推奨される修正方法 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FString SuggestedFix;
};

/**
 * Blueprint分析結果の統合レポート
 */
USTRUCT(BlueprintType)
struct BLUEPRINTCOMPLEXITYANALYZER_API FBPAnalysisReport
{
	GENERATED_BODY()

	/** 分析対象Blueprintパス */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FString BlueprintPath;

	/** Blueprint名 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FString BlueprintName;

	/** 親クラス名 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FString ParentClassName;

	/** 分析日時 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FDateTime AnalysisTime;

	// ========== 各メトリクス ==========

	/** ノード数メトリクス */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FBPNodeMetrics NodeMetrics;

	/** 依存関係メトリクス */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FBPDependencyMetrics DependencyMetrics;

	/** Tick使用メトリクス */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FBPTickMetrics TickMetrics;

	/** C++化推奨度メトリクス */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FBPCppMigrationMetrics CppMigrationMetrics;

	// ========== 総合評価 ==========

	/** 総合スコア（0-100、低いほど健全） */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	float OverallComplexityScore = 0.0f;

	/** 総合健全性レベル */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	EBPHealthLevel OverallHealthLevel = EBPHealthLevel::Green;

	/** 検出された問題リスト */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	TArray<FBPIssue> Issues;

	/** 人間向けサマリー */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FString HumanReadableSummary;

	/** 推奨アクション */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	TArray<FString> RecommendedActions;
};

/**
 * プロジェクト全体の分析サマリー
 */
USTRUCT(BlueprintType)
struct BLUEPRINTCOMPLEXITYANALYZER_API FBPProjectAnalysisSummary
{
	GENERATED_BODY()

	/** 分析したBlueprint総数 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 TotalBlueprintCount = 0;

	/** Green状態のBlueprint数 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 GreenCount = 0;

	/** Yellow状態のBlueprint数 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 YellowCount = 0;

	/** Red状態のBlueprint数 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	int32 RedCount = 0;

	/** 平均複雑度スコア */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	float AverageComplexityScore = 0.0f;

	/** 最も複雑なBlueprint Top10 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	TArray<FBPAnalysisReport> MostComplexBlueprints;

	/** Tick使用Blueprint一覧 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	TArray<FString> BlueprintsUsingTick;

	/** 循環参照を持つBlueprint一覧 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	TArray<FString> BlueprintsWithCircularReferences;

	/** C++化推奨Blueprint一覧 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	TArray<FString> BlueprintsRecommendedForCpp;

	/** 分析日時 */
	UPROPERTY(BlueprintReadOnly, Category = "Analysis")
	FDateTime AnalysisTime;
};

/**
 * 閾値設定
 */
USTRUCT(BlueprintType)
struct BLUEPRINTCOMPLEXITYANALYZER_API FBPComplexityThresholds
{
	GENERATED_BODY()

	// ========== ノード数閾値 ==========

	/** ノード数 Yellow閾値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds|Nodes")
	int32 NodeCountYellow = 100;

	/** ノード数 Red閾値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds|Nodes")
	int32 NodeCountRed = 300;

	/** 単一グラフノード数 Yellow閾値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds|Nodes")
	int32 SingleGraphNodeCountYellow = 50;

	/** 単一グラフノード数 Red閾値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds|Nodes")
	int32 SingleGraphNodeCountRed = 100;

	// ========== 依存関係閾値 ==========

	/** 直接依存数 Yellow閾値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds|Dependencies")
	int32 DirectDependencyYellow = 10;

	/** 直接依存数 Red閾値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds|Dependencies")
	int32 DirectDependencyRed = 20;

	/** 依存深度 Yellow閾値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds|Dependencies")
	int32 DependencyDepthYellow = 5;

	/** 依存深度 Red閾値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds|Dependencies")
	int32 DependencyDepthRed = 10;

	// ========== Tick閾値 ==========

	/** Tick内ノード数 Yellow閾値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds|Tick")
	int32 TickNodeCountYellow = 10;

	/** Tick内ノード数 Red閾値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds|Tick")
	int32 TickNodeCountRed = 30;

	// ========== C++化推奨閾値 ==========

	/** C++化推奨スコア閾値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds|CppMigration")
	float CppMigrationScoreThreshold = 60.0f;
};
