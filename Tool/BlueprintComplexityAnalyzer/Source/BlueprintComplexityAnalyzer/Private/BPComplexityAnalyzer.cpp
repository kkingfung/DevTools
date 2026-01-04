// Copyright DevTools. All Rights Reserved.

#include "BPComplexityAnalyzer.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_Tunnel.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/BlueprintGeneratedClass.h"

UBPComplexityAnalyzer::UBPComplexityAnalyzer()
{
	// デフォルト閾値設定
	Thresholds = FBPComplexityThresholds();
}

FBPAnalysisReport UBPComplexityAnalyzer::AnalyzeBlueprint(UBlueprint* Blueprint)
{
	FBPAnalysisReport Report;

	if (!Blueprint)
	{
		Report.HumanReadableSummary = TEXT("Invalid Blueprint");
		return Report;
	}

	// 基本情報
	Report.BlueprintPath = Blueprint->GetPathName();
	Report.BlueprintName = Blueprint->GetName();
	Report.AnalysisTime = FDateTime::Now();

	if (Blueprint->ParentClass)
	{
		Report.ParentClassName = Blueprint->ParentClass->GetName();
	}

	// 各メトリクスを分析
	Report.NodeMetrics = AnalyzeNodeCount(Blueprint);
	Report.DependencyMetrics = AnalyzeDependencies(Blueprint);
	Report.TickMetrics = AnalyzeTickUsage(Blueprint);

	// C++化推奨度を計算（他のメトリクスに基づく）
	Report.CppMigrationMetrics = CalculateCppMigrationScore(Blueprint, Report);

	// 総合スコアを計算
	Report.OverallComplexityScore =
		(Report.NodeMetrics.ComplexityScore * 0.35f) +
		(Report.DependencyMetrics.ComplexityScore * 0.25f) +
		(Report.TickMetrics.ComplexityScore * 0.25f) +
		(Report.CppMigrationMetrics.MigrationScore * 0.15f);

	Report.OverallHealthLevel = CalculateHealthLevel(Report.OverallComplexityScore);

	// 問題を検出
	DetectIssues(Report, Report.Issues);

	// サマリーと推奨アクションを生成
	Report.HumanReadableSummary = GenerateHumanReadableSummary(Report);
	Report.RecommendedActions = GenerateRecommendedActions(Report);

	return Report;
}

FBPProjectAnalysisSummary UBPComplexityAnalyzer::AnalyzeProject(const FString& PathFilter)
{
	FBPProjectAnalysisSummary Summary;
	Summary.AnalysisTime = FDateTime::Now();

	// アセットレジストリからBlueprintを取得
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	FARFilter Filter;
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	Filter.bRecursivePaths = true;

	if (!PathFilter.IsEmpty())
	{
		Filter.PackagePaths.Add(FName(*PathFilter));
	}
	else
	{
		Filter.PackagePaths.Add(TEXT("/Game"));
	}

	TArray<FAssetData> BlueprintAssets;
	AssetRegistry.GetAssets(Filter, BlueprintAssets);

	TArray<FBPAnalysisReport> AllReports;
	float TotalScore = 0.0f;

	for (const FAssetData& AssetData : BlueprintAssets)
	{
		UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
		if (!Blueprint)
		{
			continue;
		}

		FBPAnalysisReport Report = AnalyzeBlueprint(Blueprint);
		AllReports.Add(Report);

		Summary.TotalBlueprintCount++;
		TotalScore += Report.OverallComplexityScore;

		// 健全性レベル別カウント
		switch (Report.OverallHealthLevel)
		{
		case EBPHealthLevel::Green:
			Summary.GreenCount++;
			break;
		case EBPHealthLevel::Yellow:
			Summary.YellowCount++;
			break;
		case EBPHealthLevel::Red:
			Summary.RedCount++;
			break;
		}

		// Tick使用チェック
		if (Report.TickMetrics.bUsesTick)
		{
			Summary.BlueprintsUsingTick.Add(Report.BlueprintPath);
		}

		// 循環参照チェック
		if (Report.DependencyMetrics.CircularReferenceCount > 0)
		{
			Summary.BlueprintsWithCircularReferences.Add(Report.BlueprintPath);
		}

		// C++化推奨チェック
		if (Report.CppMigrationMetrics.MigrationScore >= Thresholds.CppMigrationScoreThreshold)
		{
			Summary.BlueprintsRecommendedForCpp.Add(Report.BlueprintPath);
		}
	}

	// 平均スコア
	if (Summary.TotalBlueprintCount > 0)
	{
		Summary.AverageComplexityScore = TotalScore / Summary.TotalBlueprintCount;
	}

	// 最も複雑なTop10を抽出
	AllReports.Sort([](const FBPAnalysisReport& A, const FBPAnalysisReport& B)
	{
		return A.OverallComplexityScore > B.OverallComplexityScore;
	});

	int32 TopCount = FMath::Min(10, AllReports.Num());
	for (int32 i = 0; i < TopCount; ++i)
	{
		Summary.MostComplexBlueprints.Add(AllReports[i]);
	}

	return Summary;
}

FBPAnalysisReport UBPComplexityAnalyzer::AnalyzeBlueprintByPath(const FString& AssetPath)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *AssetPath);
	return AnalyzeBlueprint(Blueprint);
}

FBPNodeMetrics UBPComplexityAnalyzer::AnalyzeNodeCount(UBlueprint* Blueprint)
{
	FBPNodeMetrics Metrics;

	if (!Blueprint)
	{
		return Metrics;
	}

	// カテゴリカウント用マップ
	TMap<FString, int32> CategoryCounts;
	int32 LargestGraphNodes = 0;

	// 全グラフを分析
	TArray<UEdGraph*> AllGraphs;
	Blueprint->GetAllGraphs(AllGraphs);

	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph)
		{
			continue;
		}

		int32 GraphNodeCount = 0;

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node)
			{
				continue;
			}

			Metrics.TotalNodeCount++;
			GraphNodeCount++;

			// ノードタイプ別カウント
			if (UK2Node_CallFunction* FuncNode = Cast<UK2Node_CallFunction>(Node))
			{
				Metrics.FunctionCallCount++;
			}
			else if (Cast<UK2Node_VariableGet>(Node) || Cast<UK2Node_VariableSet>(Node))
			{
				Metrics.VariableAccessCount++;
			}
			else if (Cast<UK2Node_IfThenElse>(Node))
			{
				Metrics.ControlFlowCount++;
			}
			else if (Cast<UK2Node_Event>(Node))
			{
				Metrics.EventNodeCount++;
			}
			else if (Cast<UK2Node_MacroInstance>(Node))
			{
				Metrics.MacroCount++;
			}
			else if (Cast<UK2Node_CustomEvent>(Node))
			{
				Metrics.CustomEventCount++;
			}

			// カテゴリ別カウント
			FString Category = GetNodeCategory(Node);
			CategoryCounts.FindOrAdd(Category)++;
		}

		// 最大グラフを更新
		if (GraphNodeCount > LargestGraphNodes)
		{
			LargestGraphNodes = GraphNodeCount;
			Metrics.LargestGraphNodeCount = GraphNodeCount;
			Metrics.LargestGraphName = Graph->GetName();
		}
	}

	// カテゴリ別内訳を構築
	for (const auto& Pair : CategoryCounts)
	{
		FBPNodeCategoryCount CatCount;
		CatCount.CategoryName = Pair.Key;
		CatCount.Count = Pair.Value;
		CatCount.Percentage = (Metrics.TotalNodeCount > 0) ?
			(float)Pair.Value / Metrics.TotalNodeCount * 100.0f : 0.0f;
		Metrics.CategoryBreakdown.Add(CatCount);
	}

	// スコア計算（ノード数ベース）
	float BaseScore = FMath::Clamp((float)Metrics.TotalNodeCount / (float)Thresholds.NodeCountRed * 100.0f, 0.0f, 100.0f);
	float GraphScore = FMath::Clamp((float)Metrics.LargestGraphNodeCount / (float)Thresholds.SingleGraphNodeCountRed * 50.0f, 0.0f, 50.0f);
	Metrics.ComplexityScore = FMath::Min(100.0f, BaseScore * 0.6f + GraphScore * 0.4f);

	// 健全性レベル判定
	if (Metrics.TotalNodeCount >= Thresholds.NodeCountRed ||
		Metrics.LargestGraphNodeCount >= Thresholds.SingleGraphNodeCountRed)
	{
		Metrics.HealthLevel = EBPHealthLevel::Red;
	}
	else if (Metrics.TotalNodeCount >= Thresholds.NodeCountYellow ||
		Metrics.LargestGraphNodeCount >= Thresholds.SingleGraphNodeCountYellow)
	{
		Metrics.HealthLevel = EBPHealthLevel::Yellow;
	}
	else
	{
		Metrics.HealthLevel = EBPHealthLevel::Green;
	}

	return Metrics;
}

FBPDependencyMetrics UBPComplexityAnalyzer::AnalyzeDependencies(UBlueprint* Blueprint)
{
	FBPDependencyMetrics Metrics;

	if (!Blueprint)
	{
		return Metrics;
	}

	TSet<FString> VisitedAssets;
	int32 MaxDepth = 0;

	// 依存関係を再帰的に収集
	CollectDependenciesRecursive(Blueprint, VisitedAssets, Metrics.Dependencies, 0, MaxDepth);

	Metrics.DirectDependencyCount = Metrics.Dependencies.Num();
	Metrics.TransitiveDependencyCount = VisitedAssets.Num();
	Metrics.MaxDependencyDepth = MaxDepth;

	// 循環参照を検出
	DetectCircularReferences(Blueprint, Metrics.CircularReferencePaths);
	Metrics.CircularReferenceCount = Metrics.CircularReferencePaths.Num();

	// スコア計算
	float DepCountScore = FMath::Clamp((float)Metrics.DirectDependencyCount / (float)Thresholds.DirectDependencyRed * 50.0f, 0.0f, 50.0f);
	float DepthScore = FMath::Clamp((float)Metrics.MaxDependencyDepth / (float)Thresholds.DependencyDepthRed * 30.0f, 0.0f, 30.0f);
	float CircularScore = Metrics.CircularReferenceCount > 0 ? 50.0f : 0.0f;

	Metrics.ComplexityScore = FMath::Min(100.0f, DepCountScore + DepthScore + CircularScore);

	// 健全性レベル判定
	if (Metrics.CircularReferenceCount > 0 ||
		Metrics.DirectDependencyCount >= Thresholds.DirectDependencyRed ||
		Metrics.MaxDependencyDepth >= Thresholds.DependencyDepthRed)
	{
		Metrics.HealthLevel = EBPHealthLevel::Red;
	}
	else if (Metrics.DirectDependencyCount >= Thresholds.DirectDependencyYellow ||
		Metrics.MaxDependencyDepth >= Thresholds.DependencyDepthYellow)
	{
		Metrics.HealthLevel = EBPHealthLevel::Yellow;
	}
	else
	{
		Metrics.HealthLevel = EBPHealthLevel::Green;
	}

	return Metrics;
}

FBPTickMetrics UBPComplexityAnalyzer::AnalyzeTickUsage(UBlueprint* Blueprint)
{
	FBPTickMetrics Metrics;

	if (!Blueprint)
	{
		return Metrics;
	}

	TArray<UEdGraph*> AllGraphs;
	Blueprint->GetAllGraphs(AllGraphs);

	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph)
		{
			continue;
		}

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
			if (!EventNode)
			{
				continue;
			}

			// EventTick を検出
			FName EventName = EventNode->GetFunctionName();
			if (EventName == TEXT("ReceiveTick") || EventName.ToString().Contains(TEXT("Tick")))
			{
				Metrics.bUsesTick = true;
				Metrics.TickEventCount++;

				FBPTickInfo TickInfo;
				TickInfo.GraphName = Graph->GetName();

				// Tickからの実行パスを追跡
				TraceTickExecutionPath(EventNode, TickInfo);

				Metrics.TotalNodesInTick += TickInfo.NodeCountInTick;
				Metrics.TickDetails.Add(TickInfo);
			}
		}
	}

	// 最適化推奨を生成
	if (Metrics.bUsesTick)
	{
		if (Metrics.TotalNodesInTick > Thresholds.TickNodeCountRed)
		{
			Metrics.OptimizationSuggestions.Add(TEXT("Tick内の処理量が多すぎます。タイマーまたはイベント駆動への変更を検討してください。"));
		}

		if (Metrics.TotalNodesInTick > Thresholds.TickNodeCountYellow)
		{
			Metrics.OptimizationSuggestions.Add(TEXT("Tick内でのLoop処理は避け、配列処理は分散させてください。"));
		}

		Metrics.OptimizationSuggestions.Add(TEXT("Tickの使用を最小限にし、必要な場合はC++での実装を検討してください。"));
	}

	// スコア計算
	if (!Metrics.bUsesTick)
	{
		Metrics.ComplexityScore = 0.0f;
		Metrics.HealthLevel = EBPHealthLevel::Green;
	}
	else
	{
		float TickNodeScore = FMath::Clamp((float)Metrics.TotalNodesInTick / (float)Thresholds.TickNodeCountRed * 80.0f, 0.0f, 80.0f);
		float TickCountScore = FMath::Clamp((float)Metrics.TickEventCount * 10.0f, 0.0f, 20.0f);

		Metrics.ComplexityScore = FMath::Min(100.0f, TickNodeScore + TickCountScore);

		if (Metrics.TotalNodesInTick >= Thresholds.TickNodeCountRed)
		{
			Metrics.HealthLevel = EBPHealthLevel::Red;
		}
		else if (Metrics.TotalNodesInTick >= Thresholds.TickNodeCountYellow)
		{
			Metrics.HealthLevel = EBPHealthLevel::Yellow;
		}
		else
		{
			Metrics.HealthLevel = EBPHealthLevel::Green;
		}
	}

	return Metrics;
}

FBPCppMigrationMetrics UBPComplexityAnalyzer::CalculateCppMigrationScore(UBlueprint* Blueprint, const FBPAnalysisReport& Report)
{
	FBPCppMigrationMetrics Metrics;

	if (!Blueprint)
	{
		return Metrics;
	}

	float Score = 0.0f;
	int32 Difficulty = 1;

	// ノード数が多い場合
	if (Report.NodeMetrics.TotalNodeCount >= Thresholds.NodeCountRed)
	{
		Score += 30.0f;
		Metrics.Reasons.Add(FString::Printf(TEXT("ノード数が多い (%d nodes)"), Report.NodeMetrics.TotalNodeCount));
		Metrics.ExpectedImprovements.Add(TEXT("パフォーマンス向上"));
		Difficulty = FMath::Max(Difficulty, 3);
	}
	else if (Report.NodeMetrics.TotalNodeCount >= Thresholds.NodeCountYellow)
	{
		Score += 15.0f;
		Metrics.Reasons.Add(FString::Printf(TEXT("ノード数がやや多い (%d nodes)"), Report.NodeMetrics.TotalNodeCount));
		Difficulty = FMath::Max(Difficulty, 2);
	}

	// Tick使用の場合
	if (Report.TickMetrics.bUsesTick)
	{
		Score += 25.0f;
		Metrics.Reasons.Add(TEXT("Tickを使用している"));
		Metrics.ExpectedImprovements.Add(TEXT("Tick処理の最適化"));
		Difficulty = FMath::Max(Difficulty, 2);

		if (Report.TickMetrics.TotalNodesInTick >= Thresholds.TickNodeCountRed)
		{
			Score += 15.0f;
			Metrics.Reasons.Add(TEXT("Tick内の処理が重い"));
			Difficulty = FMath::Max(Difficulty, 4);
		}
	}

	// 循環参照がある場合
	if (Report.DependencyMetrics.CircularReferenceCount > 0)
	{
		Score += 20.0f;
		Metrics.Reasons.Add(TEXT("循環参照が存在する"));
		Metrics.ExpectedImprovements.Add(TEXT("依存関係の整理"));
		Difficulty = FMath::Max(Difficulty, 4);
	}

	// 依存が深い場合
	if (Report.DependencyMetrics.MaxDependencyDepth >= Thresholds.DependencyDepthRed)
	{
		Score += 10.0f;
		Metrics.Reasons.Add(TEXT("依存関係が深い"));
		Difficulty = FMath::Max(Difficulty, 3);
	}

	// 数学処理が多い場合
	float MathRatio = (Report.NodeMetrics.TotalNodeCount > 0) ?
		(float)Report.NodeMetrics.MathOperationCount / Report.NodeMetrics.TotalNodeCount : 0.0f;
	if (MathRatio > 0.3f)
	{
		Score += 15.0f;
		Metrics.Reasons.Add(TEXT("数学/演算処理が多い"));
		Metrics.ExpectedImprovements.Add(TEXT("計算処理の高速化"));
		Difficulty = FMath::Max(Difficulty, 2);
	}

	Metrics.MigrationScore = FMath::Clamp(Score, 0.0f, 100.0f);
	Metrics.MigrationDifficulty = Difficulty;

	// 優先度判定
	if (Metrics.MigrationScore >= 70.0f)
	{
		Metrics.Priority = EBPHealthLevel::Red;
	}
	else if (Metrics.MigrationScore >= 40.0f)
	{
		Metrics.Priority = EBPHealthLevel::Yellow;
	}
	else
	{
		Metrics.Priority = EBPHealthLevel::Green;
	}

	return Metrics;
}

FLinearColor UBPComplexityAnalyzer::GetHealthLevelColor(EBPHealthLevel Level)
{
	switch (Level)
	{
	case EBPHealthLevel::Green:
		return FLinearColor(0.2f, 0.8f, 0.2f);
	case EBPHealthLevel::Yellow:
		return FLinearColor(0.9f, 0.8f, 0.1f);
	case EBPHealthLevel::Red:
		return FLinearColor(0.9f, 0.2f, 0.2f);
	default:
		return FLinearColor::White;
	}
}

FString UBPComplexityAnalyzer::GetHealthLevelString(EBPHealthLevel Level)
{
	switch (Level)
	{
	case EBPHealthLevel::Green:
		return TEXT("健全 (Green)");
	case EBPHealthLevel::Yellow:
		return TEXT("警告 (Yellow)");
	case EBPHealthLevel::Red:
		return TEXT("危険 (Red)");
	default:
		return TEXT("不明");
	}
}

FString UBPComplexityAnalyzer::GetNodeCategory(UEdGraphNode* Node)
{
	if (!Node)
	{
		return TEXT("Unknown");
	}

	if (Cast<UK2Node_CallFunction>(Node))
	{
		return TEXT("Function Call");
	}
	else if (Cast<UK2Node_VariableGet>(Node) || Cast<UK2Node_VariableSet>(Node))
	{
		return TEXT("Variable Access");
	}
	else if (Cast<UK2Node_IfThenElse>(Node))
	{
		return TEXT("Control Flow");
	}
	else if (Cast<UK2Node_Event>(Node))
	{
		return TEXT("Event");
	}
	else if (Cast<UK2Node_MacroInstance>(Node))
	{
		return TEXT("Macro");
	}
	else if (Cast<UK2Node_CustomEvent>(Node))
	{
		return TEXT("Custom Event");
	}
	else if (Cast<UK2Node_Tunnel>(Node))
	{
		return TEXT("Tunnel");
	}

	return TEXT("Other");
}

void UBPComplexityAnalyzer::TraceTickExecutionPath(UEdGraphNode* TickNode, FBPTickInfo& OutInfo)
{
	if (!TickNode)
	{
		return;
	}

	TSet<UEdGraphNode*> VisitedNodes;
	TQueue<UEdGraphNode*> NodesToVisit;
	NodesToVisit.Enqueue(TickNode);

	while (!NodesToVisit.IsEmpty())
	{
		UEdGraphNode* CurrentNode;
		NodesToVisit.Dequeue(CurrentNode);

		if (!CurrentNode || VisitedNodes.Contains(CurrentNode))
		{
			continue;
		}

		VisitedNodes.Add(CurrentNode);
		OutInfo.NodeCountInTick++;

		// 関数呼び出しを記録
		if (UK2Node_CallFunction* FuncNode = Cast<UK2Node_CallFunction>(CurrentNode))
		{
			FName FuncName = FuncNode->GetFunctionName();
			if (FuncName != NAME_None)
			{
				OutInfo.FunctionsCalledInTick.AddUnique(FuncName.ToString());
			}

			// 重い処理を警告
			FString FuncNameStr = FuncName.ToString();
			if (FuncNameStr.Contains(TEXT("GetAllActors")) ||
				FuncNameStr.Contains(TEXT("LineTrace")) ||
				FuncNameStr.Contains(TEXT("Overlap")))
			{
				OutInfo.HeavyOperationWarnings.Add(FString::Printf(TEXT("Tick内で%sを呼び出し"), *FuncNameStr));
			}
		}

		// 出力ピンから次のノードを辿る
		for (UEdGraphPin* Pin : CurrentNode->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Output)
			{
				for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
				{
					if (LinkedPin && LinkedPin->GetOwningNode())
					{
						NodesToVisit.Enqueue(LinkedPin->GetOwningNode());
					}
				}
			}
		}
	}
}

void UBPComplexityAnalyzer::CollectDependenciesRecursive(UBlueprint* Blueprint, TSet<FString>& VisitedAssets,
	TArray<FBPDependencyInfo>& OutDependencies, int32 CurrentDepth, int32& MaxDepth)
{
	if (!Blueprint)
	{
		return;
	}

	FString CurrentPath = Blueprint->GetPathName();
	if (VisitedAssets.Contains(CurrentPath))
	{
		return;
	}

	VisitedAssets.Add(CurrentPath);
	MaxDepth = FMath::Max(MaxDepth, CurrentDepth);

	// 直接依存を収集
	TArray<FAssetIdentifier> Dependencies;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	AssetRegistry.GetDependencies(FAssetIdentifier(FName(*CurrentPath)), Dependencies);

	for (const FAssetIdentifier& DepId : Dependencies)
	{
		FString DepPath = DepId.PackageName.ToString();

		// Blueprintアセットのみをカウント
		if (!DepPath.StartsWith(TEXT("/Game")))
		{
			continue;
		}

		FBPDependencyInfo DepInfo;
		DepInfo.AssetPath = DepPath;
		DepInfo.ReferenceCount = 1;

		OutDependencies.Add(DepInfo);
	}
}

void UBPComplexityAnalyzer::DetectCircularReferences(UBlueprint* Blueprint, TArray<FString>& OutCircularPaths)
{
	if (!Blueprint)
	{
		return;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	FString CurrentPath = Blueprint->GetPathName();
	TArray<FAssetIdentifier> Dependencies;
	AssetRegistry.GetDependencies(FAssetIdentifier(FName(*CurrentPath)), Dependencies);

	// 各依存先が自分を参照しているかチェック
	for (const FAssetIdentifier& DepId : Dependencies)
	{
		TArray<FAssetIdentifier> ReverseDeps;
		AssetRegistry.GetDependencies(DepId, ReverseDeps);

		for (const FAssetIdentifier& RevDep : ReverseDeps)
		{
			if (RevDep.PackageName.ToString() == CurrentPath)
			{
				OutCircularPaths.Add(FString::Printf(TEXT("%s <-> %s"),
					*CurrentPath, *DepId.PackageName.ToString()));
			}
		}
	}
}

EBPHealthLevel UBPComplexityAnalyzer::CalculateHealthLevel(float Score) const
{
	if (Score >= 70.0f)
	{
		return EBPHealthLevel::Red;
	}
	else if (Score >= 40.0f)
	{
		return EBPHealthLevel::Yellow;
	}
	return EBPHealthLevel::Green;
}

void UBPComplexityAnalyzer::DetectIssues(const FBPAnalysisReport& Report, TArray<FBPIssue>& OutIssues)
{
	// ノード数の問題
	if (Report.NodeMetrics.TotalNodeCount >= Thresholds.NodeCountRed)
	{
		FBPIssue Issue;
		Issue.Category = TEXT("ノード数");
		Issue.Description = FString::Printf(TEXT("総ノード数が%dを超えています (%d)"),
			Thresholds.NodeCountRed, Report.NodeMetrics.TotalNodeCount);
		Issue.Severity = EBPHealthLevel::Red;
		Issue.SuggestedFix = TEXT("機能を複数のBPに分割するか、C++への移行を検討してください。");
		OutIssues.Add(Issue);
	}

	// 単一グラフの問題
	if (Report.NodeMetrics.LargestGraphNodeCount >= Thresholds.SingleGraphNodeCountRed)
	{
		FBPIssue Issue;
		Issue.Category = TEXT("グラフサイズ");
		Issue.Description = FString::Printf(TEXT("グラフ '%s' のノード数が多すぎます (%d)"),
			*Report.NodeMetrics.LargestGraphName, Report.NodeMetrics.LargestGraphNodeCount);
		Issue.Severity = EBPHealthLevel::Red;
		Issue.Location = Report.NodeMetrics.LargestGraphName;
		Issue.SuggestedFix = TEXT("関数に分割するか、マクロを使用してください。");
		OutIssues.Add(Issue);
	}

	// 循環参照
	if (Report.DependencyMetrics.CircularReferenceCount > 0)
	{
		FBPIssue Issue;
		Issue.Category = TEXT("循環参照");
		Issue.Description = FString::Printf(TEXT("循環参照が%d件検出されました"),
			Report.DependencyMetrics.CircularReferenceCount);
		Issue.Severity = EBPHealthLevel::Red;
		Issue.SuggestedFix = TEXT("インターフェースを使用するか、依存関係を見直してください。");
		OutIssues.Add(Issue);
	}

	// Tick使用
	if (Report.TickMetrics.bUsesTick && Report.TickMetrics.TotalNodesInTick >= Thresholds.TickNodeCountRed)
	{
		FBPIssue Issue;
		Issue.Category = TEXT("Tick使用");
		Issue.Description = FString::Printf(TEXT("Tick内の処理が重すぎます (%d nodes)"),
			Report.TickMetrics.TotalNodesInTick);
		Issue.Severity = EBPHealthLevel::Red;
		Issue.SuggestedFix = TEXT("タイマーまたはイベント駆動に変更するか、C++で実装してください。");
		OutIssues.Add(Issue);
	}

	// 依存深度
	if (Report.DependencyMetrics.MaxDependencyDepth >= Thresholds.DependencyDepthRed)
	{
		FBPIssue Issue;
		Issue.Category = TEXT("依存深度");
		Issue.Description = FString::Printf(TEXT("依存関係が深すぎます (深度: %d)"),
			Report.DependencyMetrics.MaxDependencyDepth);
		Issue.Severity = EBPHealthLevel::Yellow;
		Issue.SuggestedFix = TEXT("依存関係を整理し、中間層を減らしてください。");
		OutIssues.Add(Issue);
	}
}

FString UBPComplexityAnalyzer::GenerateHumanReadableSummary(const FBPAnalysisReport& Report)
{
	TArray<FString> Parts;

	Parts.Add(FString::Printf(TEXT("[%s]"), *Report.BlueprintName));
	Parts.Add(FString::Printf(TEXT("総合: %s (スコア: %.0f)"),
		*GetHealthLevelString(Report.OverallHealthLevel), Report.OverallComplexityScore));

	Parts.Add(FString::Printf(TEXT("ノード: %d"), Report.NodeMetrics.TotalNodeCount));

	if (Report.TickMetrics.bUsesTick)
	{
		Parts.Add(FString::Printf(TEXT("Tick使用 (%d nodes)"), Report.TickMetrics.TotalNodesInTick));
	}

	if (Report.DependencyMetrics.CircularReferenceCount > 0)
	{
		Parts.Add(FString::Printf(TEXT("循環参照: %d"), Report.DependencyMetrics.CircularReferenceCount));
	}

	if (Report.CppMigrationMetrics.MigrationScore >= 50.0f)
	{
		Parts.Add(TEXT("C++化推奨"));
	}

	return FString::Join(Parts, TEXT(" | "));
}

TArray<FString> UBPComplexityAnalyzer::GenerateRecommendedActions(const FBPAnalysisReport& Report)
{
	TArray<FString> Actions;

	if (Report.OverallHealthLevel == EBPHealthLevel::Red)
	{
		Actions.Add(TEXT("【緊急】このBlueprintは即座にリファクタリングが必要です。"));
	}

	if (Report.NodeMetrics.TotalNodeCount >= Thresholds.NodeCountYellow)
	{
		Actions.Add(TEXT("機能を複数のBlueprintまたは関数に分割してください。"));
	}

	if (Report.TickMetrics.bUsesTick)
	{
		Actions.Add(TEXT("Tick処理をタイマーまたはイベント駆動に置き換えてください。"));
	}

	if (Report.DependencyMetrics.CircularReferenceCount > 0)
	{
		Actions.Add(TEXT("循環参照を解消するためにインターフェースを使用してください。"));
	}

	if (Report.CppMigrationMetrics.MigrationScore >= Thresholds.CppMigrationScoreThreshold)
	{
		Actions.Add(TEXT("パフォーマンス向上のためC++への移行を検討してください。"));
	}

	if (Actions.Num() == 0)
	{
		Actions.Add(TEXT("現在の状態は健全です。このまま維持してください。"));
	}

	return Actions;
}
