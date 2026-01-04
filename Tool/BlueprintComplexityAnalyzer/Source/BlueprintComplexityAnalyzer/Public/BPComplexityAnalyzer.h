// Copyright DevTools. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BPComplexityTypes.h"
#include "BPComplexityAnalyzer.generated.h"

class UBlueprint;
class UEdGraph;
class UEdGraphNode;

/**
 * Blueprint複雑度アナライザー
 * BPの健全性を分析し、問題点を可視化する
 */
UCLASS()
class BLUEPRINTCOMPLEXITYANALYZER_API UBPComplexityAnalyzer : public UObject
{
	GENERATED_BODY()

public:
	UBPComplexityAnalyzer();

	// ========== メイン分析API ==========

	/**
	 * Blueprintを分析してレポートを生成
	 * @param Blueprint 分析対象のBlueprint
	 * @return 分析レポート
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprint Complexity Analyzer")
	FBPAnalysisReport AnalyzeBlueprint(UBlueprint* Blueprint);

	/**
	 * プロジェクト内の全Blueprintを分析
	 * @param PathFilter パスフィルタ（空の場合は全て）
	 * @return プロジェクト分析サマリー
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprint Complexity Analyzer")
	FBPProjectAnalysisSummary AnalyzeProject(const FString& PathFilter = TEXT(""));

	/**
	 * アセットパスからBlueprintを分析
	 * @param AssetPath アセットパス
	 * @return 分析レポート
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprint Complexity Analyzer")
	FBPAnalysisReport AnalyzeBlueprintByPath(const FString& AssetPath);

	// ========== 個別分析API ==========

	/**
	 * ノード数を分析
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprint Complexity Analyzer")
	FBPNodeMetrics AnalyzeNodeCount(UBlueprint* Blueprint);

	/**
	 * 依存関係を分析
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprint Complexity Analyzer")
	FBPDependencyMetrics AnalyzeDependencies(UBlueprint* Blueprint);

	/**
	 * Tick使用を分析
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprint Complexity Analyzer")
	FBPTickMetrics AnalyzeTickUsage(UBlueprint* Blueprint);

	/**
	 * C++化推奨度を計算
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprint Complexity Analyzer")
	FBPCppMigrationMetrics CalculateCppMigrationScore(UBlueprint* Blueprint, const FBPAnalysisReport& Report);

	// ========== 設定 ==========

	/**
	 * 閾値設定を取得
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprint Complexity Analyzer")
	FBPComplexityThresholds GetThresholds() const { return Thresholds; }

	/**
	 * 閾値設定を更新
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprint Complexity Analyzer")
	void SetThresholds(const FBPComplexityThresholds& NewThresholds) { Thresholds = NewThresholds; }

	// ========== ユーティリティ ==========

	/**
	 * 健全性レベルを色に変換
	 */
	UFUNCTION(BlueprintPure, Category = "Blueprint Complexity Analyzer")
	static FLinearColor GetHealthLevelColor(EBPHealthLevel Level);

	/**
	 * 健全性レベルを文字列に変換
	 */
	UFUNCTION(BlueprintPure, Category = "Blueprint Complexity Analyzer")
	static FString GetHealthLevelString(EBPHealthLevel Level);

protected:
	/** 閾値設定 */
	UPROPERTY()
	FBPComplexityThresholds Thresholds;

	// ========== 内部ヘルパー ==========

	/**
	 * グラフ内のノードを分析
	 */
	void AnalyzeGraph(UEdGraph* Graph, FBPNodeMetrics& OutMetrics);

	/**
	 * ノードのカテゴリを判定
	 */
	FString GetNodeCategory(UEdGraphNode* Node);

	/**
	 * Tickイベントからの実行パスを追跡
	 */
	void TraceTickExecutionPath(UEdGraphNode* TickNode, FBPTickInfo& OutInfo);

	/**
	 * 依存関係を再帰的に収集
	 */
	void CollectDependenciesRecursive(UBlueprint* Blueprint, TSet<FString>& VisitedAssets,
		TArray<FBPDependencyInfo>& OutDependencies, int32 CurrentDepth, int32& MaxDepth);

	/**
	 * 循環参照を検出
	 */
	void DetectCircularReferences(UBlueprint* Blueprint, TArray<FString>& OutCircularPaths);

	/**
	 * スコアから健全性レベルを判定
	 */
	EBPHealthLevel CalculateHealthLevel(float Score) const;

	/**
	 * 問題を検出してリストに追加
	 */
	void DetectIssues(const FBPAnalysisReport& Report, TArray<FBPIssue>& OutIssues);

	/**
	 * 人間向けサマリーを生成
	 */
	FString GenerateHumanReadableSummary(const FBPAnalysisReport& Report);

	/**
	 * 推奨アクションを生成
	 */
	TArray<FString> GenerateRecommendedActions(const FBPAnalysisReport& Report);
};
