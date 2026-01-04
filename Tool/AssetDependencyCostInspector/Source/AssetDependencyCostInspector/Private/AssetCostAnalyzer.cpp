// Copyright DevTools. All Rights Reserved.

#include "AssetCostAnalyzer.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "Animation/AnimSequence.h"
#include "Engine/Blueprint.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "UObject/UObjectIterator.h"
#include "Serialization/ArchiveCountMem.h"
#include "HAL/PlatformFileManager.h"

UAssetCostAnalyzer::UAssetCostAnalyzer()
{
	Thresholds = FAssetCostThresholds();
}

FAssetCostReport UAssetCostAnalyzer::AnalyzeAsset(const FString& AssetPath)
{
	FAssetCostReport Report;
	Report.AssetPath = AssetPath;
	Report.AnalysisTime = FDateTime::Now();

	// アセットをロード
	UObject* Asset = LoadObject<UObject>(nullptr, *AssetPath);
	if (!Asset)
	{
		Report.HumanReadableSummary = TEXT("アセットをロードできませんでした");
		return Report;
	}

	Report.AssetName = Asset->GetName();
	Report.Category = GetAssetCategory(Asset->GetClass());

	// 各コストを計算
	Report.MemoryCost = CalculateMemoryCost(AssetPath);
	Report.StreamingInfo = GetStreamingInfo(AssetPath);
	Report.LoadTiming = AnalyzeLoadTiming(AssetPath);
	Report.UE5Cost = CalculateUE5Cost(AssetPath);

	// 依存ツリーを構築
	Report.DependencyTree = BuildDependencyTree(AssetPath, 10);

	// 依存関係統計を計算
	TSet<FString> AllDependencies;
	TQueue<const FAssetDependencyNode*> NodeQueue;
	NodeQueue.Enqueue(&Report.DependencyTree);

	int32 MaxDepth = 0;
	while (!NodeQueue.IsEmpty())
	{
		const FAssetDependencyNode* CurrentNode;
		NodeQueue.Dequeue(CurrentNode);

		MaxDepth = FMath::Max(MaxDepth, CurrentNode->Info.Depth);

		for (const FAssetDependencyNode& Child : CurrentNode->Children)
		{
			AllDependencies.Add(Child.Info.AssetPath);
			NodeQueue.Enqueue(&Child);
		}
	}

	Report.DirectDependencyCount = Report.DependencyTree.Children.Num();
	Report.TotalDependencyCount = AllDependencies.Num();
	Report.MaxDependencyDepth = MaxDepth;

	// 循環参照を検出
	DetectCircularReferences(AssetPath, Report.CircularReferences);

	// 問題と最適化推奨を生成
	DetectIssues(Report);
	GenerateOptimizationSuggestions(Report);

	// 総合スコアを計算
	float MemoryScore = FMath::Clamp(Report.MemoryCost.MemorySize / (Thresholds.MemoryCriticalMB * 1024 * 1024) * 40.0f, 0.0f, 40.0f);
	float DependencyScore = FMath::Clamp((float)Report.TotalDependencyCount / Thresholds.DependencyCritical * 30.0f, 0.0f, 30.0f);
	float StreamingScore = Report.StreamingInfo.bIsStreamable ? 0.0f : 15.0f;
	float CircularScore = Report.CircularReferences.Num() > 0 ? 15.0f : 0.0f;

	Report.OverallCostScore = FMath::Min(100.0f, MemoryScore + DependencyScore + StreamingScore + CircularScore);

	// コストレベルを判定
	if (Report.OverallCostScore >= 70.0f)
	{
		Report.OverallCostLevel = EAssetCostLevel::Critical;
	}
	else if (Report.OverallCostScore >= 50.0f)
	{
		Report.OverallCostLevel = EAssetCostLevel::High;
	}
	else if (Report.OverallCostScore >= 30.0f)
	{
		Report.OverallCostLevel = EAssetCostLevel::Medium;
	}
	else
	{
		Report.OverallCostLevel = EAssetCostLevel::Low;
	}

	Report.HumanReadableSummary = GenerateHumanReadableSummary(Report);

	return Report;
}

FAssetCostReport UAssetCostAnalyzer::AnalyzeLoadedAsset(UObject* Asset)
{
	if (!Asset)
	{
		FAssetCostReport EmptyReport;
		EmptyReport.HumanReadableSummary = TEXT("無効なアセット");
		return EmptyReport;
	}

	return AnalyzeAsset(Asset->GetPathName());
}

FProjectCostSummary UAssetCostAnalyzer::AnalyzeFolder(const FString& FolderPath)
{
	FProjectCostSummary Summary;
	Summary.AnalysisTime = FDateTime::Now();
	Summary.AnalyzedPath = FolderPath;

	// アセットレジストリから取得
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*FolderPath));
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssets(Filter, AssetDataList);

	TArray<FAssetCostReport> AllReports;
	TMap<EAssetCategory, FCategoryCostSummary> CategoryMap;

	for (const FAssetData& AssetData : AssetDataList)
	{
		FString AssetPath = AssetData.GetObjectPathString();
		FAssetCostReport Report = AnalyzeAsset(AssetPath);

		AllReports.Add(Report);
		Summary.TotalAssetCount++;
		Summary.TotalMemoryCost += Report.MemoryCost.MemorySize;
		Summary.TotalDiskSize += Report.MemoryCost.DiskSize;

		// カテゴリ別集計
		if (!CategoryMap.Contains(Report.Category))
		{
			FCategoryCostSummary CatSummary;
			CatSummary.Category = Report.Category;
			CatSummary.CategoryName = GetCategoryName(Report.Category);
			CategoryMap.Add(Report.Category, CatSummary);
		}

		FCategoryCostSummary& CatSummary = CategoryMap[Report.Category];
		CatSummary.AssetCount++;
		CatSummary.TotalMemoryCost += Report.MemoryCost.MemorySize;
		CatSummary.TotalDiskSize += Report.MemoryCost.DiskSize;

		if (Report.MemoryCost.MemorySize > CatSummary.HeaviestAssetCost)
		{
			CatSummary.HeaviestAssetCost = Report.MemoryCost.MemorySize;
			CatSummary.HeaviestAsset = Report.AssetName;
		}

		// 統計
		if (Report.UE5Cost.bNaniteEnabled)
		{
			Summary.NaniteAssetCount++;
		}
		if (Report.StreamingInfo.bIsStreamable)
		{
			Summary.StreamableAssetCount++;
		}
		if (Report.CircularReferences.Num() > 0)
		{
			Summary.CircularReferenceCount++;
		}

		// 問題のあるアセット
		if (Report.OverallCostLevel == EAssetCostLevel::Critical ||
			Report.OverallCostLevel == EAssetCostLevel::High)
		{
			Summary.ProblematicAssets.Add(AssetPath);
		}
	}

	// カテゴリ別パーセンテージを計算
	for (auto& Pair : CategoryMap)
	{
		if (Summary.TotalMemoryCost > 0)
		{
			Pair.Value.Percentage = (float)Pair.Value.TotalMemoryCost / Summary.TotalMemoryCost * 100.0f;
		}
		Summary.CategorySummaries.Add(Pair.Value);
	}

	// コストでソート
	Summary.CategorySummaries.Sort([](const FCategoryCostSummary& A, const FCategoryCostSummary& B)
	{
		return A.TotalMemoryCost > B.TotalMemoryCost;
	});

	// 最も重いアセットTop10
	AllReports.Sort([](const FAssetCostReport& A, const FAssetCostReport& B)
	{
		return A.MemoryCost.TotalCostWithDependencies > B.MemoryCost.TotalCostWithDependencies;
	});

	int32 TopCount = FMath::Min(10, AllReports.Num());
	for (int32 i = 0; i < TopCount; ++i)
	{
		Summary.HeaviestAssets.Add(AllReports[i]);
	}

	return Summary;
}

FProjectCostSummary UAssetCostAnalyzer::AnalyzeProject()
{
	return AnalyzeFolder(TEXT("/Game"));
}

FAssetMemoryCost UAssetCostAnalyzer::CalculateMemoryCost(const FString& AssetPath)
{
	FAssetMemoryCost Cost;

	UObject* Asset = LoadObject<UObject>(nullptr, *AssetPath);
	if (!Asset)
	{
		return Cost;
	}

	// アセットタイプに応じたコスト計算
	if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset))
	{
		Cost = CalculateStaticMeshCost(StaticMesh);
	}
	else if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(Asset))
	{
		Cost = CalculateSkeletalMeshCost(SkeletalMesh);
	}
	else if (UTexture* Texture = Cast<UTexture>(Asset))
	{
		Cost = CalculateTextureCost(Texture);
	}
	else if (UMaterialInterface* Material = Cast<UMaterialInterface>(Asset))
	{
		Cost = CalculateMaterialCost(Material);
	}
	else if (USoundBase* Sound = Cast<USoundBase>(Asset))
	{
		Cost = CalculateSoundCost(Sound);
	}
	else
	{
		Cost = CalculateGenericCost(Asset);
	}

	// ディスクサイズを取得
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(AssetPath));
	if (AssetData.IsValid())
	{
		FAssetPackageData PackageData;
		if (AssetRegistry.TryGetAssetPackageData(AssetData.PackageName, PackageData) == UE::AssetRegistry::EExists::Exists)
		{
			Cost.DiskSize = PackageData.DiskSize;
		}
	}

	// 圧縮率
	if (Cost.DiskSize > 0 && Cost.MemorySize > 0)
	{
		Cost.CompressionRatio = (float)Cost.MemorySize / Cost.DiskSize;
	}

	Cost.CostLevel = CalculateCostLevel(Cost.MemorySize);

	return Cost;
}

FAssetDependencyNode UAssetCostAnalyzer::BuildDependencyTree(const FString& AssetPath, int32 MaxDepth)
{
	FAssetDependencyNode RootNode;
	RootNode.Info.AssetPath = AssetPath;
	RootNode.Info.Depth = 0;

	// アセット名を取得
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(AssetPath));
	if (AssetData.IsValid())
	{
		RootNode.Info.AssetName = AssetData.AssetName.ToString();
		RootNode.Info.Category = GetAssetCategory(AssetData.GetClass());
	}

	TSet<FString> VisitedAssets;
	VisitedAssets.Add(AssetPath);

	CollectDependenciesRecursive(AssetPath, VisitedAssets, RootNode, 0, MaxDepth);

	return RootNode;
}

void UAssetCostAnalyzer::CollectDependenciesRecursive(
	const FString& AssetPath,
	TSet<FString>& VisitedAssets,
	FAssetDependencyNode& OutNode,
	int32 CurrentDepth,
	int32 MaxDepth)
{
	if (CurrentDepth >= MaxDepth)
	{
		return;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetIdentifier> Dependencies;
	AssetRegistry.GetDependencies(FAssetIdentifier(FName(*AssetPath)), Dependencies);

	int64 SubtreeCost = 0;
	int32 SubtreeCount = 0;

	for (const FAssetIdentifier& DepId : Dependencies)
	{
		FString DepPath = DepId.PackageName.ToString();

		// エンジンアセットはスキップ
		if (DepPath.StartsWith(TEXT("/Engine")) || DepPath.StartsWith(TEXT("/Script")))
		{
			continue;
		}

		FAssetDependencyNode ChildNode;
		ChildNode.Info.AssetPath = DepPath;
		ChildNode.Info.Depth = CurrentDepth + 1;

		// アセット情報を取得
		FAssetData DepAssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(DepPath));
		if (DepAssetData.IsValid())
		{
			ChildNode.Info.AssetName = DepAssetData.AssetName.ToString();
			ChildNode.Info.Category = GetAssetCategory(DepAssetData.GetClass());
		}
		else
		{
			ChildNode.Info.AssetName = FPaths::GetBaseFilename(DepPath);
		}

		// メモリコストを計算
		FAssetMemoryCost DepCost = CalculateMemoryCost(DepPath);
		ChildNode.Info.MemoryCost = DepCost.MemorySize;

		// 循環参照チェック
		if (VisitedAssets.Contains(DepPath))
		{
			ChildNode.Info.bIsInCircularReference = true;
		}
		else
		{
			VisitedAssets.Add(DepPath);

			// 再帰的に依存を収集
			CollectDependenciesRecursive(DepPath, VisitedAssets, ChildNode, CurrentDepth + 1, MaxDepth);
		}

		SubtreeCost += ChildNode.Info.MemoryCost + ChildNode.SubtreeTotalCost;
		SubtreeCount += 1 + ChildNode.SubtreeAssetCount;

		OutNode.Children.Add(ChildNode);
	}

	OutNode.SubtreeTotalCost = SubtreeCost;
	OutNode.SubtreeAssetCount = SubtreeCount;
}

FAssetStreamingInfo UAssetCostAnalyzer::GetStreamingInfo(const FString& AssetPath)
{
	FAssetStreamingInfo Info;

	UObject* Asset = LoadObject<UObject>(nullptr, *AssetPath);
	if (!Asset)
	{
		return Info;
	}

	// テクスチャのStreaming情報
	if (UTexture2D* Texture = Cast<UTexture2D>(Asset))
	{
		Info.bIsStreamable = Texture->IsStreamable();
		Info.NumMipLevels = Texture->GetNumMips();
		Info.NumResidentMips = Texture->GetNumResidentMips();

		// 各MIPレベルのサイズを計算
		int64 TotalSize = 0;
		int64 ResidentSize = 0;

		for (int32 MipIndex = 0; MipIndex < Info.NumMipLevels; ++MipIndex)
		{
			int32 MipWidth = FMath::Max(1, Texture->GetSizeX() >> MipIndex);
			int32 MipHeight = FMath::Max(1, Texture->GetSizeY() >> MipIndex);
			int64 MipSize = MipWidth * MipHeight * 4; // 簡易計算

			TotalSize += MipSize;
			if (MipIndex >= Info.NumMipLevels - Info.NumResidentMips)
			{
				ResidentSize += MipSize;
			}
		}

		Info.ResidentSize = ResidentSize;
		Info.StreamedSize = TotalSize - ResidentSize;
	}
	// StaticMeshのStreaming情報
	else if (UStaticMesh* Mesh = Cast<UStaticMesh>(Asset))
	{
		Info.bIsStreamable = Mesh->bAllowCPUAccess;
		Info.NumLODs = Mesh->GetNumLODs();
	}
	// SkeletalMeshのStreaming情報
	else if (USkeletalMesh* SkMesh = Cast<USkeletalMesh>(Asset))
	{
		Info.bIsStreamable = true;
		Info.NumLODs = SkMesh->GetLODNum();
	}

	// Streaming影響スコア
	if (Info.bIsStreamable)
	{
		float ResidentRatio = (Info.ResidentSize + Info.StreamedSize > 0) ?
			(float)Info.ResidentSize / (Info.ResidentSize + Info.StreamedSize) : 1.0f;
		Info.StreamingImpactScore = (1.0f - ResidentRatio) * 100.0f;
	}
	else
	{
		Info.StreamingImpactScore = 100.0f; // 非Streamingは常に高コスト
	}

	return Info;
}

FAssetLoadTiming UAssetCostAnalyzer::AnalyzeLoadTiming(const FString& AssetPath)
{
	FAssetLoadTiming Timing;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(AssetPath));
	if (!AssetData.IsValid())
	{
		return Timing;
	}

	// ディスクサイズから読み込み時間を推定（簡易計算）
	FAssetPackageData PackageData;
	if (AssetRegistry.TryGetAssetPackageData(AssetData.PackageName, PackageData) == UE::AssetRegistry::EExists::Exists)
	{
		// SSD想定: 500MB/s、HDD想定: 100MB/s（中間値を使用）
		float ReadSpeedMBps = 300.0f;
		Timing.EstimatedLoadTimeMs = (PackageData.DiskSize / (1024.0f * 1024.0f)) / ReadSpeedMBps * 1000.0f;
	}

	// 非同期ロード可能性
	Timing.bCanAsyncLoad = true; // ほとんどのアセットは非同期ロード可能

	// ブロッキングロードの警告
	if (Timing.EstimatedLoadTimeMs > Thresholds.LoadTimeHighMs)
	{
		Timing.LoadWarnings.Add(FString::Printf(TEXT("推定読み込み時間が%.1fmsと長いです"), Timing.EstimatedLoadTimeMs));
	}

	return Timing;
}

FUE5SpecificCost UAssetCostAnalyzer::CalculateUE5Cost(const FString& AssetPath)
{
	FUE5SpecificCost Cost;

	UObject* Asset = LoadObject<UObject>(nullptr, *AssetPath);
	if (!Asset)
	{
		return Cost;
	}

	// StaticMeshのNanite情報
	if (UStaticMesh* Mesh = Cast<UStaticMesh>(Asset))
	{
		CollectNaniteInfo(Mesh, Cost);
	}

	return Cost;
}

void UAssetCostAnalyzer::CollectNaniteInfo(UStaticMesh* Mesh, FUE5SpecificCost& OutCost)
{
	if (!Mesh)
	{
		return;
	}

	// Nanite設定をチェック
	OutCost.bNaniteEnabled = Mesh->NaniteSettings.bEnabled;

	if (OutCost.bNaniteEnabled)
	{
		// Naniteリソースサイズを取得（利用可能な場合）
		const FStaticMeshRenderData* RenderData = Mesh->GetRenderData();
		if (RenderData)
		{
			// LOD0のトライアングル数
			if (RenderData->LODResources.Num() > 0)
			{
				const FStaticMeshLODResources& LOD0 = RenderData->LODResources[0];
				OutCost.NaniteFallbackTriangleCount = LOD0.GetNumTriangles();
			}
		}
	}
}

FAssetMemoryCost UAssetCostAnalyzer::CalculateStaticMeshCost(UStaticMesh* Mesh)
{
	FAssetMemoryCost Cost;

	if (!Mesh)
	{
		return Cost;
	}

	const FStaticMeshRenderData* RenderData = Mesh->GetRenderData();
	if (RenderData)
	{
		// 各LODのサイズを計算
		for (int32 LODIndex = 0; LODIndex < RenderData->LODResources.Num(); ++LODIndex)
		{
			const FStaticMeshLODResources& LOD = RenderData->LODResources[LODIndex];

			// 頂点バッファサイズ
			Cost.MemorySize += LOD.VertexBuffers.StaticMeshVertexBuffer.GetResourceSize();
			Cost.MemorySize += LOD.VertexBuffers.PositionVertexBuffer.GetNumVertices() * sizeof(FVector3f);

			// インデックスバッファサイズ
			Cost.MemorySize += LOD.IndexBuffer.GetAllocatedSize();
		}

		Cost.GPUMemorySize = Cost.MemorySize; // メッシュはGPUにも常駐
	}

	// Naniteデータサイズ
	if (Mesh->NaniteSettings.bEnabled)
	{
		// Naniteは元のメッシュより大きくなる可能性がある
		Cost.NaniteDataSize = Cost.MemorySize * 2; // 概算
		Cost.MemorySize += Cost.NaniteDataSize;
	}

	return Cost;
}

FAssetMemoryCost UAssetCostAnalyzer::CalculateSkeletalMeshCost(USkeletalMesh* Mesh)
{
	FAssetMemoryCost Cost;

	if (!Mesh)
	{
		return Cost;
	}

	// リソースサイズを取得
	FArchiveCountMem CountMem(nullptr);
	Mesh->Serialize(CountMem);
	Cost.MemorySize = CountMem.GetMax();
	Cost.GPUMemorySize = Cost.MemorySize;

	return Cost;
}

FAssetMemoryCost UAssetCostAnalyzer::CalculateTextureCost(UTexture* Texture)
{
	FAssetMemoryCost Cost;

	if (!Texture)
	{
		return Cost;
	}

	// テクスチャサイズを取得
	Cost.MemorySize = Texture->CalcTextureMemorySizeEnum(TMC_ResidentMips);
	Cost.GPUMemorySize = Cost.MemorySize;

	// VirtualTextureサイズ
	if (Texture->VirtualTextureStreaming)
	{
		Cost.VirtualTextureSize = Texture->CalcTextureMemorySizeEnum(TMC_AllMipsBiased);
	}

	return Cost;
}

FAssetMemoryCost UAssetCostAnalyzer::CalculateMaterialCost(UMaterialInterface* Material)
{
	FAssetMemoryCost Cost;

	if (!Material)
	{
		return Cost;
	}

	// マテリアル自体のサイズは小さいが、参照テクスチャを考慮
	FArchiveCountMem CountMem(nullptr);
	Material->Serialize(CountMem);
	Cost.MemorySize = CountMem.GetMax();

	return Cost;
}

FAssetMemoryCost UAssetCostAnalyzer::CalculateSoundCost(USoundBase* Sound)
{
	FAssetMemoryCost Cost;

	if (!Sound)
	{
		return Cost;
	}

	if (USoundWave* Wave = Cast<USoundWave>(Sound))
	{
		Cost.MemorySize = Wave->GetResourceSizeBytes(EResourceSizeMode::EstimatedTotal);
	}

	return Cost;
}

FAssetMemoryCost UAssetCostAnalyzer::CalculateGenericCost(UObject* Asset)
{
	FAssetMemoryCost Cost;

	if (!Asset)
	{
		return Cost;
	}

	// 汎用サイズ計測
	FArchiveCountMem CountMem(nullptr);
	Asset->Serialize(CountMem);
	Cost.MemorySize = CountMem.GetMax();

	return Cost;
}

EAssetCostLevel UAssetCostAnalyzer::CalculateCostLevel(int64 MemoryCost) const
{
	float MemoryMB = MemoryCost / (1024.0f * 1024.0f);

	if (MemoryMB >= Thresholds.MemoryCriticalMB)
	{
		return EAssetCostLevel::Critical;
	}
	else if (MemoryMB >= Thresholds.MemoryHighMB)
	{
		return EAssetCostLevel::High;
	}
	else if (MemoryMB >= Thresholds.MemoryMediumMB)
	{
		return EAssetCostLevel::Medium;
	}
	return EAssetCostLevel::Low;
}

EAssetCategory UAssetCostAnalyzer::GetAssetCategory(UClass* AssetClass)
{
	if (!AssetClass)
	{
		return EAssetCategory::Other;
	}

	if (AssetClass->IsChildOf(UStaticMesh::StaticClass()))
	{
		return EAssetCategory::StaticMesh;
	}
	else if (AssetClass->IsChildOf(USkeletalMesh::StaticClass()))
	{
		return EAssetCategory::SkeletalMesh;
	}
	else if (AssetClass->IsChildOf(UTexture::StaticClass()))
	{
		return EAssetCategory::Texture;
	}
	else if (AssetClass->IsChildOf(UMaterialInstance::StaticClass()))
	{
		return EAssetCategory::MaterialInstance;
	}
	else if (AssetClass->IsChildOf(UMaterial::StaticClass()))
	{
		return EAssetCategory::Material;
	}
	else if (AssetClass->IsChildOf(UBlueprint::StaticClass()))
	{
		return EAssetCategory::Blueprint;
	}
	else if (AssetClass->IsChildOf(UAnimSequenceBase::StaticClass()))
	{
		return EAssetCategory::Animation;
	}
	else if (AssetClass->IsChildOf(USoundBase::StaticClass()))
	{
		return EAssetCategory::Sound;
	}

	return EAssetCategory::Other;
}

FString UAssetCostAnalyzer::GetCategoryName(EAssetCategory Category)
{
	switch (Category)
	{
	case EAssetCategory::StaticMesh: return TEXT("StaticMesh");
	case EAssetCategory::SkeletalMesh: return TEXT("SkeletalMesh");
	case EAssetCategory::Texture: return TEXT("Texture");
	case EAssetCategory::Material: return TEXT("Material");
	case EAssetCategory::MaterialInstance: return TEXT("MaterialInstance");
	case EAssetCategory::Blueprint: return TEXT("Blueprint");
	case EAssetCategory::Animation: return TEXT("Animation");
	case EAssetCategory::Sound: return TEXT("Sound");
	case EAssetCategory::ParticleSystem: return TEXT("ParticleSystem");
	case EAssetCategory::Niagara: return TEXT("Niagara");
	case EAssetCategory::DataAsset: return TEXT("DataAsset");
	case EAssetCategory::Level: return TEXT("Level");
	default: return TEXT("Other");
	}
}

FLinearColor UAssetCostAnalyzer::GetCostLevelColor(EAssetCostLevel Level)
{
	switch (Level)
	{
	case EAssetCostLevel::Low: return FLinearColor(0.2f, 0.8f, 0.2f);
	case EAssetCostLevel::Medium: return FLinearColor(0.9f, 0.8f, 0.1f);
	case EAssetCostLevel::High: return FLinearColor(0.9f, 0.5f, 0.1f);
	case EAssetCostLevel::Critical: return FLinearColor(0.9f, 0.2f, 0.2f);
	default: return FLinearColor::White;
	}
}

FString UAssetCostAnalyzer::GetCostLevelString(EAssetCostLevel Level)
{
	switch (Level)
	{
	case EAssetCostLevel::Low: return TEXT("低コスト");
	case EAssetCostLevel::Medium: return TEXT("中コスト");
	case EAssetCostLevel::High: return TEXT("高コスト");
	case EAssetCostLevel::Critical: return TEXT("危険");
	default: return TEXT("不明");
	}
}

void UAssetCostAnalyzer::DetectIssues(FAssetCostReport& Report)
{
	// メモリが大きすぎる
	if (Report.MemoryCost.MemorySize > Thresholds.MemoryCriticalMB * 1024 * 1024)
	{
		Report.Issues.Add(FString::Printf(TEXT("メモリ使用量が%sMBを超えています"),
			*FString::SanitizeFloat(Thresholds.MemoryCriticalMB)));
	}

	// 依存が多すぎる
	if (Report.TotalDependencyCount > Thresholds.DependencyCritical)
	{
		Report.Issues.Add(FString::Printf(TEXT("依存アセット数が%dを超えています"),
			Thresholds.DependencyCritical));
	}

	// 循環参照
	if (Report.CircularReferences.Num() > 0)
	{
		Report.Issues.Add(TEXT("循環参照が検出されました"));
	}

	// 非Streaming
	if (!Report.StreamingInfo.bIsStreamable && Report.MemoryCost.MemorySize > 10 * 1024 * 1024)
	{
		Report.Issues.Add(TEXT("大きなアセットがStreaming無効です"));
	}

	// 読み込み時間
	if (Report.LoadTiming.EstimatedLoadTimeMs > Thresholds.LoadTimeHighMs)
	{
		Report.Issues.Add(TEXT("読み込み時間が長すぎます"));
	}
}

void UAssetCostAnalyzer::GenerateOptimizationSuggestions(FAssetCostReport& Report)
{
	// テクスチャの最適化
	if (Report.Category == EAssetCategory::Texture)
	{
		if (Report.MemoryCost.MemorySize > 50 * 1024 * 1024)
		{
			Report.OptimizationSuggestions.Add(TEXT("テクスチャサイズを縮小するか、圧縮設定を見直してください"));
		}
		if (!Report.StreamingInfo.bIsStreamable)
		{
			Report.OptimizationSuggestions.Add(TEXT("Streamingを有効にすることを検討してください"));
		}
	}

	// メッシュの最適化
	if (Report.Category == EAssetCategory::StaticMesh)
	{
		if (!Report.UE5Cost.bNaniteEnabled && Report.MemoryCost.MemorySize > 10 * 1024 * 1024)
		{
			Report.OptimizationSuggestions.Add(TEXT("Naniteを有効にすることを検討してください"));
		}
		if (Report.StreamingInfo.NumLODs < 2)
		{
			Report.OptimizationSuggestions.Add(TEXT("LODを追加することを検討してください"));
		}
	}

	// 依存関係の最適化
	if (Report.TotalDependencyCount > Thresholds.DependencyMedium)
	{
		Report.OptimizationSuggestions.Add(TEXT("依存関係を整理し、不要な参照を削除してください"));
	}

	// 循環参照
	if (Report.CircularReferences.Num() > 0)
	{
		Report.OptimizationSuggestions.Add(TEXT("循環参照を解消してください"));
	}
}

FString UAssetCostAnalyzer::GenerateHumanReadableSummary(const FAssetCostReport& Report)
{
	TArray<FString> Parts;

	Parts.Add(FString::Printf(TEXT("[%s]"), *Report.AssetName));
	Parts.Add(FString::Printf(TEXT("%s (%s)"),
		*GetCostLevelString(Report.OverallCostLevel),
		*FAssetMemoryCost::FormatBytes(Report.MemoryCost.MemorySize)));

	if (Report.TotalDependencyCount > 0)
	{
		Parts.Add(FString::Printf(TEXT("依存: %d件"), Report.TotalDependencyCount));
	}

	if (Report.UE5Cost.bNaniteEnabled)
	{
		Parts.Add(TEXT("Nanite有効"));
	}

	if (!Report.StreamingInfo.bIsStreamable)
	{
		Parts.Add(TEXT("Streaming無効"));
	}

	if (Report.Issues.Num() > 0)
	{
		Parts.Add(FString::Printf(TEXT("問題: %d件"), Report.Issues.Num()));
	}

	return FString::Join(Parts, TEXT(" | "));
}

void UAssetCostAnalyzer::DetectCircularReferences(const FString& RootAsset, TArray<FString>& OutCircularPaths)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetIdentifier> Dependencies;
	AssetRegistry.GetDependencies(FAssetIdentifier(FName(*RootAsset)), Dependencies);

	for (const FAssetIdentifier& DepId : Dependencies)
	{
		TArray<FAssetIdentifier> ReverseDeps;
		AssetRegistry.GetReferencers(DepId, ReverseDeps);

		for (const FAssetIdentifier& RevDep : ReverseDeps)
		{
			if (RevDep.PackageName.ToString() == RootAsset)
			{
				OutCircularPaths.Add(FString::Printf(TEXT("%s <-> %s"),
					*RootAsset, *DepId.PackageName.ToString()));
			}
		}
	}
}

TArray<FCategoryCostSummary> UAssetCostAnalyzer::BuildCategorySummaries(const TArray<FAssetCostReport>& Reports)
{
	TMap<EAssetCategory, FCategoryCostSummary> CategoryMap;

	for (const FAssetCostReport& Report : Reports)
	{
		if (!CategoryMap.Contains(Report.Category))
		{
			FCategoryCostSummary Summary;
			Summary.Category = Report.Category;
			Summary.CategoryName = GetCategoryName(Report.Category);
			CategoryMap.Add(Report.Category, Summary);
		}

		FCategoryCostSummary& Summary = CategoryMap[Report.Category];
		Summary.AssetCount++;
		Summary.TotalMemoryCost += Report.MemoryCost.MemorySize;
		Summary.TotalDiskSize += Report.MemoryCost.DiskSize;
	}

	TArray<FCategoryCostSummary> Result;
	CategoryMap.GenerateValueArray(Result);

	Result.Sort([](const FCategoryCostSummary& A, const FCategoryCostSummary& B)
	{
		return A.TotalMemoryCost > B.TotalMemoryCost;
	});

	return Result;
}
