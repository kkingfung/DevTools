// Copyright DevTools. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetCostTypes.generated.h"

/**
 * コスト警告レベル
 */
UENUM(BlueprintType)
enum class EAssetCostLevel : uint8
{
	/** 低コスト: 問題なし */
	Low UMETA(DisplayName = "Low Cost"),

	/** 中コスト: 注意が必要 */
	Medium UMETA(DisplayName = "Medium Cost"),

	/** 高コスト: 要最適化 */
	High UMETA(DisplayName = "High Cost"),

	/** 危険: 即座に対応が必要 */
	Critical UMETA(DisplayName = "Critical Cost")
};

/**
 * アセットタイプ分類
 */
UENUM(BlueprintType)
enum class EAssetCategory : uint8
{
	StaticMesh,
	SkeletalMesh,
	Texture,
	Material,
	MaterialInstance,
	Blueprint,
	Animation,
	Sound,
	ParticleSystem,
	Niagara,
	DataAsset,
	Level,
	Other
};

/**
 * メモリコスト詳細
 */
USTRUCT(BlueprintType)
struct ASSETDEPENDENCYCOSTINSPECTOR_API FAssetMemoryCost
{
	GENERATED_BODY()

	/** ディスク上のサイズ（バイト） */
	UPROPERTY(BlueprintReadOnly, Category = "Cost")
	int64 DiskSize = 0;

	/** メモリ上のサイズ（バイト） */
	UPROPERTY(BlueprintReadOnly, Category = "Cost")
	int64 MemorySize = 0;

	/** GPUメモリサイズ（バイト）- テクスチャ等 */
	UPROPERTY(BlueprintReadOnly, Category = "Cost")
	int64 GPUMemorySize = 0;

	/** Nanite使用時の追加コスト（バイト） */
	UPROPERTY(BlueprintReadOnly, Category = "Cost")
	int64 NaniteDataSize = 0;

	/** Virtual Textureサイズ（バイト） */
	UPROPERTY(BlueprintReadOnly, Category = "Cost")
	int64 VirtualTextureSize = 0;

	/** 圧縮率（ディスク/メモリ） */
	UPROPERTY(BlueprintReadOnly, Category = "Cost")
	float CompressionRatio = 1.0f;

	/** 依存を含めた総コスト（バイト） */
	UPROPERTY(BlueprintReadOnly, Category = "Cost")
	int64 TotalCostWithDependencies = 0;

	/** コスト警告レベル */
	UPROPERTY(BlueprintReadOnly, Category = "Cost")
	EAssetCostLevel CostLevel = EAssetCostLevel::Low;

	/** 人間向けサイズ文字列を取得 */
	FString GetFormattedSize() const
	{
		return FormatBytes(MemorySize);
	}

	/** 人間向け総コスト文字列を取得 */
	FString GetFormattedTotalCost() const
	{
		return FormatBytes(TotalCostWithDependencies);
	}

	static FString FormatBytes(int64 Bytes)
	{
		if (Bytes < 1024)
			return FString::Printf(TEXT("%lld B"), Bytes);
		else if (Bytes < 1024 * 1024)
			return FString::Printf(TEXT("%.2f KB"), Bytes / 1024.0);
		else if (Bytes < 1024 * 1024 * 1024)
			return FString::Printf(TEXT("%.2f MB"), Bytes / (1024.0 * 1024.0));
		else
			return FString::Printf(TEXT("%.2f GB"), Bytes / (1024.0 * 1024.0 * 1024.0));
	}
};

/**
 * Streaming情報
 */
USTRUCT(BlueprintType)
struct ASSETDEPENDENCYCOSTINSPECTOR_API FAssetStreamingInfo
{
	GENERATED_BODY()

	/** Streaming対応かどうか */
	UPROPERTY(BlueprintReadOnly, Category = "Streaming")
	bool bIsStreamable = false;

	/** 常駐メモリサイズ（バイト） */
	UPROPERTY(BlueprintReadOnly, Category = "Streaming")
	int64 ResidentSize = 0;

	/** ストリーミングサイズ（バイト） */
	UPROPERTY(BlueprintReadOnly, Category = "Streaming")
	int64 StreamedSize = 0;

	/** MIPレベル数（テクスチャ） */
	UPROPERTY(BlueprintReadOnly, Category = "Streaming")
	int32 NumMipLevels = 0;

	/** 常駐MIPレベル数 */
	UPROPERTY(BlueprintReadOnly, Category = "Streaming")
	int32 NumResidentMips = 0;

	/** LODレベル数（メッシュ） */
	UPROPERTY(BlueprintReadOnly, Category = "Streaming")
	int32 NumLODs = 0;

	/** ストリーミンググループ */
	UPROPERTY(BlueprintReadOnly, Category = "Streaming")
	FString StreamingGroup;

	/** 優先度 */
	UPROPERTY(BlueprintReadOnly, Category = "Streaming")
	int32 Priority = 0;

	/** Streaming影響スコア（0-100） */
	UPROPERTY(BlueprintReadOnly, Category = "Streaming")
	float StreamingImpactScore = 0.0f;
};

/**
 * 読み込みタイミング情報
 */
USTRUCT(BlueprintType)
struct ASSETDEPENDENCYCOSTINSPECTOR_API FAssetLoadTiming
{
	GENERATED_BODY()

	/** 読み込みフェーズ */
	UPROPERTY(BlueprintReadOnly, Category = "Loading")
	FString LoadPhase;

	/** 推定読み込み時間（ミリ秒） */
	UPROPERTY(BlueprintReadOnly, Category = "Loading")
	float EstimatedLoadTimeMs = 0.0f;

	/** ブロッキングロードかどうか */
	UPROPERTY(BlueprintReadOnly, Category = "Loading")
	bool bIsBlockingLoad = false;

	/** 非同期ロード可能かどうか */
	UPROPERTY(BlueprintReadOnly, Category = "Loading")
	bool bCanAsyncLoad = true;

	/** 初回参照時にロードされるか */
	UPROPERTY(BlueprintReadOnly, Category = "Loading")
	bool bLoadOnFirstReference = false;

	/** AlwaysLoadedかどうか */
	UPROPERTY(BlueprintReadOnly, Category = "Loading")
	bool bAlwaysLoaded = false;

	/** ロード順序（依存関係ベース） */
	UPROPERTY(BlueprintReadOnly, Category = "Loading")
	int32 LoadOrder = 0;

	/** 読み込みタイミングの警告 */
	UPROPERTY(BlueprintReadOnly, Category = "Loading")
	TArray<FString> LoadWarnings;
};

/**
 * 依存関係情報
 */
USTRUCT(BlueprintType)
struct ASSETDEPENDENCYCOSTINSPECTOR_API FAssetDependencyInfo
{
	GENERATED_BODY()

	/** アセットパス */
	UPROPERTY(BlueprintReadOnly, Category = "Dependency")
	FString AssetPath;

	/** アセット名 */
	UPROPERTY(BlueprintReadOnly, Category = "Dependency")
	FString AssetName;

	/** アセットカテゴリ */
	UPROPERTY(BlueprintReadOnly, Category = "Dependency")
	EAssetCategory Category = EAssetCategory::Other;

	/** 依存深度（0がルート） */
	UPROPERTY(BlueprintReadOnly, Category = "Dependency")
	int32 Depth = 0;

	/** このアセットのメモリコスト */
	UPROPERTY(BlueprintReadOnly, Category = "Dependency")
	int64 MemoryCost = 0;

	/** ハード依存かどうか */
	UPROPERTY(BlueprintReadOnly, Category = "Dependency")
	bool bIsHardDependency = true;

	/** ソフト依存かどうか */
	UPROPERTY(BlueprintReadOnly, Category = "Dependency")
	bool bIsSoftDependency = false;

	/** 循環参照に関与しているか */
	UPROPERTY(BlueprintReadOnly, Category = "Dependency")
	bool bIsInCircularReference = false;

	/** 参照カウント（何箇所から参照されているか） */
	UPROPERTY(BlueprintReadOnly, Category = "Dependency")
	int32 ReferenceCount = 1;
};

/**
 * 依存チェーン（ツリー構造）
 */
USTRUCT(BlueprintType)
struct ASSETDEPENDENCYCOSTINSPECTOR_API FAssetDependencyNode
{
	GENERATED_BODY()

	/** このノードの情報 */
	UPROPERTY(BlueprintReadOnly, Category = "Dependency")
	FAssetDependencyInfo Info;

	/** 子ノード（このアセットが依存しているアセット） */
	UPROPERTY(BlueprintReadOnly, Category = "Dependency")
	TArray<FAssetDependencyNode> Children;

	/** サブツリーの総コスト */
	UPROPERTY(BlueprintReadOnly, Category = "Dependency")
	int64 SubtreeTotalCost = 0;

	/** サブツリーのアセット数 */
	UPROPERTY(BlueprintReadOnly, Category = "Dependency")
	int32 SubtreeAssetCount = 0;
};

/**
 * UE5特有のコスト情報
 */
USTRUCT(BlueprintType)
struct ASSETDEPENDENCYCOSTINSPECTOR_API FUE5SpecificCost
{
	GENERATED_BODY()

	// ========== Nanite ==========

	/** Nanite有効かどうか */
	UPROPERTY(BlueprintReadOnly, Category = "UE5|Nanite")
	bool bNaniteEnabled = false;

	/** Naniteトライアングル数 */
	UPROPERTY(BlueprintReadOnly, Category = "UE5|Nanite")
	int64 NaniteTriangleCount = 0;

	/** Naniteクラスタ数 */
	UPROPERTY(BlueprintReadOnly, Category = "UE5|Nanite")
	int32 NaniteClusterCount = 0;

	/** Naniteフォールバックトライアングル数 */
	UPROPERTY(BlueprintReadOnly, Category = "UE5|Nanite")
	int64 NaniteFallbackTriangleCount = 0;

	// ========== Lumen ==========

	/** Lumen対応マテリアルかどうか */
	UPROPERTY(BlueprintReadOnly, Category = "UE5|Lumen")
	bool bLumenCompatible = true;

	/** Lumenカード数 */
	UPROPERTY(BlueprintReadOnly, Category = "UE5|Lumen")
	int32 LumenCardCount = 0;

	/** ライトマップ解像度 */
	UPROPERTY(BlueprintReadOnly, Category = "UE5|Lumen")
	int32 LightmapResolution = 0;

	// ========== Virtual Shadow Maps ==========

	/** VSM対応かどうか */
	UPROPERTY(BlueprintReadOnly, Category = "UE5|VSM")
	bool bVSMCompatible = true;

	/** シャドウキャスター複雑度 */
	UPROPERTY(BlueprintReadOnly, Category = "UE5|VSM")
	float ShadowComplexity = 0.0f;

	// ========== World Partition ==========

	/** World Partition対応かどうか */
	UPROPERTY(BlueprintReadOnly, Category = "UE5|WorldPartition")
	bool bWorldPartitionCompatible = true;

	/** 所属グリッドセル数 */
	UPROPERTY(BlueprintReadOnly, Category = "UE5|WorldPartition")
	int32 GridCellCount = 0;
};

/**
 * アセットコスト分析結果
 */
USTRUCT(BlueprintType)
struct ASSETDEPENDENCYCOSTINSPECTOR_API FAssetCostReport
{
	GENERATED_BODY()

	/** アセットパス */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	FString AssetPath;

	/** アセット名 */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	FString AssetName;

	/** アセットカテゴリ */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	EAssetCategory Category = EAssetCategory::Other;

	/** 分析日時 */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	FDateTime AnalysisTime;

	// ========== コスト情報 ==========

	/** メモリコスト */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	FAssetMemoryCost MemoryCost;

	/** Streaming情報 */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	FAssetStreamingInfo StreamingInfo;

	/** 読み込みタイミング */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	FAssetLoadTiming LoadTiming;

	/** UE5特有コスト */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	FUE5SpecificCost UE5Cost;

	// ========== 依存関係 ==========

	/** 依存ツリーのルート */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	FAssetDependencyNode DependencyTree;

	/** 直接依存数 */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	int32 DirectDependencyCount = 0;

	/** 総依存数（推移的） */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	int32 TotalDependencyCount = 0;

	/** 最大依存深度 */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	int32 MaxDependencyDepth = 0;

	/** 循環参照パス */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	TArray<FString> CircularReferences;

	// ========== 評価 ==========

	/** 総合コストスコア（0-100、高いほど重い） */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	float OverallCostScore = 0.0f;

	/** 総合コストレベル */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	EAssetCostLevel OverallCostLevel = EAssetCostLevel::Low;

	/** 問題点リスト */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	TArray<FString> Issues;

	/** 最適化推奨事項 */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	TArray<FString> OptimizationSuggestions;

	/** 人間向けサマリー */
	UPROPERTY(BlueprintReadOnly, Category = "Report")
	FString HumanReadableSummary;
};

/**
 * カテゴリ別コストサマリー
 */
USTRUCT(BlueprintType)
struct ASSETDEPENDENCYCOSTINSPECTOR_API FCategoryCostSummary
{
	GENERATED_BODY()

	/** カテゴリ */
	UPROPERTY(BlueprintReadOnly, Category = "Summary")
	EAssetCategory Category = EAssetCategory::Other;

	/** カテゴリ名 */
	UPROPERTY(BlueprintReadOnly, Category = "Summary")
	FString CategoryName;

	/** アセット数 */
	UPROPERTY(BlueprintReadOnly, Category = "Summary")
	int32 AssetCount = 0;

	/** 総メモリコスト */
	UPROPERTY(BlueprintReadOnly, Category = "Summary")
	int64 TotalMemoryCost = 0;

	/** 総ディスクサイズ */
	UPROPERTY(BlueprintReadOnly, Category = "Summary")
	int64 TotalDiskSize = 0;

	/** 全体に占める割合 */
	UPROPERTY(BlueprintReadOnly, Category = "Summary")
	float Percentage = 0.0f;

	/** 最も重いアセット */
	UPROPERTY(BlueprintReadOnly, Category = "Summary")
	FString HeaviestAsset;

	/** 最も重いアセットのコスト */
	UPROPERTY(BlueprintReadOnly, Category = "Summary")
	int64 HeaviestAssetCost = 0;
};

/**
 * プロジェクト全体のコストサマリー
 */
USTRUCT(BlueprintType)
struct ASSETDEPENDENCYCOSTINSPECTOR_API FProjectCostSummary
{
	GENERATED_BODY()

	/** 分析日時 */
	UPROPERTY(BlueprintReadOnly, Category = "Summary")
	FDateTime AnalysisTime;

	/** 分析パス */
	UPROPERTY(BlueprintReadOnly, Category = "Summary")
	FString AnalyzedPath;

	/** 総アセット数 */
	UPROPERTY(BlueprintReadOnly, Category = "Summary")
	int32 TotalAssetCount = 0;

	/** 総メモリコスト */
	UPROPERTY(BlueprintReadOnly, Category = "Summary")
	int64 TotalMemoryCost = 0;

	/** 総ディスクサイズ */
	UPROPERTY(BlueprintReadOnly, Category = "Summary")
	int64 TotalDiskSize = 0;

	/** カテゴリ別サマリー */
	UPROPERTY(BlueprintReadOnly, Category = "Summary")
	TArray<FCategoryCostSummary> CategorySummaries;

	/** 最も重いアセット Top10 */
	UPROPERTY(BlueprintReadOnly, Category = "Summary")
	TArray<FAssetCostReport> HeaviestAssets;

	/** 問題のあるアセット一覧 */
	UPROPERTY(BlueprintReadOnly, Category = "Summary")
	TArray<FString> ProblematicAssets;

	/** Nanite使用アセット数 */
	UPROPERTY(BlueprintReadOnly, Category = "Summary")
	int32 NaniteAssetCount = 0;

	/** Streaming有効アセット数 */
	UPROPERTY(BlueprintReadOnly, Category = "Summary")
	int32 StreamableAssetCount = 0;

	/** 循環参照を持つアセット数 */
	UPROPERTY(BlueprintReadOnly, Category = "Summary")
	int32 CircularReferenceCount = 0;
};

/**
 * コスト閾値設定
 */
USTRUCT(BlueprintType)
struct ASSETDEPENDENCYCOSTINSPECTOR_API FAssetCostThresholds
{
	GENERATED_BODY()

	// ========== メモリ閾値 ==========

	/** メモリMedium閾値（MB） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds|Memory")
	float MemoryMediumMB = 50.0f;

	/** メモリHigh閾値（MB） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds|Memory")
	float MemoryHighMB = 200.0f;

	/** メモリCritical閾値（MB） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds|Memory")
	float MemoryCriticalMB = 500.0f;

	// ========== 依存関係閾値 ==========

	/** 依存数Medium閾値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds|Dependencies")
	int32 DependencyMedium = 20;

	/** 依存数High閾値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds|Dependencies")
	int32 DependencyHigh = 50;

	/** 依存数Critical閾値 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds|Dependencies")
	int32 DependencyCritical = 100;

	// ========== Streaming閾値 ==========

	/** 常駐サイズMedium閾値（MB） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds|Streaming")
	float ResidentSizeMediumMB = 10.0f;

	/** 常駐サイズHigh閾値（MB） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds|Streaming")
	float ResidentSizeHighMB = 50.0f;

	// ========== ロード時間閾値 ==========

	/** ロード時間Medium閾値（ms） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds|Loading")
	float LoadTimeMediumMs = 100.0f;

	/** ロード時間High閾値（ms） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thresholds|Loading")
	float LoadTimeHighMs = 500.0f;
};
