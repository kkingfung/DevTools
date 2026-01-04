// Copyright DevTools. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetCostTypes.h"
#include "AssetCostAnalyzer.generated.h"

class UStaticMesh;
class USkeletalMesh;
class UTexture;
class UMaterialInterface;
class USoundBase;

/**
 * アセットコストアナライザー
 * アセットの依存関係とコストを分析
 */
UCLASS()
class ASSETDEPENDENCYCOSTINSPECTOR_API UAssetCostAnalyzer : public UObject
{
	GENERATED_BODY()

public:
	UAssetCostAnalyzer();

	// ========== メイン分析API ==========

	/**
	 * アセットを分析してレポートを生成
	 * @param AssetPath 分析対象のアセットパス
	 * @return 分析レポート
	 */
	UFUNCTION(BlueprintCallable, Category = "Asset Cost Analyzer")
	FAssetCostReport AnalyzeAsset(const FString& AssetPath);

	/**
	 * ロード済みアセットを分析
	 * @param Asset 分析対象のアセット
	 * @return 分析レポート
	 */
	UFUNCTION(BlueprintCallable, Category = "Asset Cost Analyzer")
	FAssetCostReport AnalyzeLoadedAsset(UObject* Asset);

	/**
	 * フォルダ内の全アセットを分析
	 * @param FolderPath フォルダパス
	 * @return プロジェクトコストサマリー
	 */
	UFUNCTION(BlueprintCallable, Category = "Asset Cost Analyzer")
	FProjectCostSummary AnalyzeFolder(const FString& FolderPath);

	/**
	 * プロジェクト全体を分析
	 * @return プロジェクトコストサマリー
	 */
	UFUNCTION(BlueprintCallable, Category = "Asset Cost Analyzer")
	FProjectCostSummary AnalyzeProject();

	// ========== 個別分析API ==========

	/**
	 * メモリコストを計算
	 */
	UFUNCTION(BlueprintCallable, Category = "Asset Cost Analyzer")
	FAssetMemoryCost CalculateMemoryCost(const FString& AssetPath);

	/**
	 * 依存ツリーを構築
	 */
	UFUNCTION(BlueprintCallable, Category = "Asset Cost Analyzer")
	FAssetDependencyNode BuildDependencyTree(const FString& AssetPath, int32 MaxDepth = 10);

	/**
	 * Streaming情報を取得
	 */
	UFUNCTION(BlueprintCallable, Category = "Asset Cost Analyzer")
	FAssetStreamingInfo GetStreamingInfo(const FString& AssetPath);

	/**
	 * 読み込みタイミングを分析
	 */
	UFUNCTION(BlueprintCallable, Category = "Asset Cost Analyzer")
	FAssetLoadTiming AnalyzeLoadTiming(const FString& AssetPath);

	/**
	 * UE5特有コストを計算
	 */
	UFUNCTION(BlueprintCallable, Category = "Asset Cost Analyzer")
	FUE5SpecificCost CalculateUE5Cost(const FString& AssetPath);

	// ========== 設定 ==========

	/**
	 * 閾値設定を取得
	 */
	UFUNCTION(BlueprintCallable, Category = "Asset Cost Analyzer")
	FAssetCostThresholds GetThresholds() const { return Thresholds; }

	/**
	 * 閾値設定を更新
	 */
	UFUNCTION(BlueprintCallable, Category = "Asset Cost Analyzer")
	void SetThresholds(const FAssetCostThresholds& NewThresholds) { Thresholds = NewThresholds; }

	// ========== ユーティリティ ==========

	/**
	 * アセットカテゴリを判定
	 */
	UFUNCTION(BlueprintPure, Category = "Asset Cost Analyzer")
	static EAssetCategory GetAssetCategory(UClass* AssetClass);

	/**
	 * カテゴリ名を取得
	 */
	UFUNCTION(BlueprintPure, Category = "Asset Cost Analyzer")
	static FString GetCategoryName(EAssetCategory Category);

	/**
	 * コストレベルを色に変換
	 */
	UFUNCTION(BlueprintPure, Category = "Asset Cost Analyzer")
	static FLinearColor GetCostLevelColor(EAssetCostLevel Level);

	/**
	 * コストレベルを文字列に変換
	 */
	UFUNCTION(BlueprintPure, Category = "Asset Cost Analyzer")
	static FString GetCostLevelString(EAssetCostLevel Level);

protected:
	/** 閾値設定 */
	UPROPERTY()
	FAssetCostThresholds Thresholds;

	// ========== 内部ヘルパー ==========

	/**
	 * 依存関係を再帰的に収集
	 */
	void CollectDependenciesRecursive(
		const FString& AssetPath,
		TSet<FString>& VisitedAssets,
		FAssetDependencyNode& OutNode,
		int32 CurrentDepth,
		int32 MaxDepth
	);

	/**
	 * StaticMeshのコストを計算
	 */
	FAssetMemoryCost CalculateStaticMeshCost(UStaticMesh* Mesh);

	/**
	 * SkeletalMeshのコストを計算
	 */
	FAssetMemoryCost CalculateSkeletalMeshCost(USkeletalMesh* Mesh);

	/**
	 * Textureのコストを計算
	 */
	FAssetMemoryCost CalculateTextureCost(UTexture* Texture);

	/**
	 * Materialのコストを計算
	 */
	FAssetMemoryCost CalculateMaterialCost(UMaterialInterface* Material);

	/**
	 * Soundのコストを計算
	 */
	FAssetMemoryCost CalculateSoundCost(USoundBase* Sound);

	/**
	 * 汎用アセットのコストを計算
	 */
	FAssetMemoryCost CalculateGenericCost(UObject* Asset);

	/**
	 * Nanite情報を収集
	 */
	void CollectNaniteInfo(UStaticMesh* Mesh, FUE5SpecificCost& OutCost);

	/**
	 * コストレベルを判定
	 */
	EAssetCostLevel CalculateCostLevel(int64 MemoryCost) const;

	/**
	 * 問題を検出
	 */
	void DetectIssues(FAssetCostReport& Report);

	/**
	 * 最適化推奨を生成
	 */
	void GenerateOptimizationSuggestions(FAssetCostReport& Report);

	/**
	 * 人間向けサマリーを生成
	 */
	FString GenerateHumanReadableSummary(const FAssetCostReport& Report);

	/**
	 * 循環参照を検出
	 */
	void DetectCircularReferences(
		const FString& RootAsset,
		TArray<FString>& OutCircularPaths
	);

	/**
	 * カテゴリ別サマリーを構築
	 */
	TArray<FCategoryCostSummary> BuildCategorySummaries(
		const TArray<FAssetCostReport>& Reports
	);
};
