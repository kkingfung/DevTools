// Copyright DevTools. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "BPComplexityTypes.h"

class UBPComplexityAnalyzer;
class SScrollBox;
class SVerticalBox;
class STextBlock;
class UBlueprint;

/**
 * Blueprint複雑度パネル Slate ウィジェット
 * 信号機表示と詳細分析結果を表示するメインUI
 */
class BLUEPRINTCOMPLEXITYANALYZER_API SBPComplexityPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBPComplexityPanel) {}
	SLATE_END_ARGS()

	/** ウィジェット構築 */
	void Construct(const FArguments& InArgs);

	/** 指定されたBlueprintを分析 */
	void AnalyzeBlueprint(UBlueprint* Blueprint);

	/** プロジェクト全体を分析 */
	void AnalyzeProject(const FString& PathFilter = TEXT(""));

	/** 現在の分析結果を取得 */
	const FBPAnalysisReport& GetCurrentReport() const { return CurrentReport; }

private:
	// ========== UI構築 ==========

	/** メインレイアウト構築 */
	TSharedRef<SWidget> BuildMainLayout();

	/** ツールバー構築 */
	TSharedRef<SWidget> BuildToolbar();

	/** 信号機表示構築 */
	TSharedRef<SWidget> BuildTrafficLight();

	/** スコア表示構築 */
	TSharedRef<SWidget> BuildScoreDisplay();

	/** 詳細パネル構築 */
	TSharedRef<SWidget> BuildDetailPanel();

	/** 問題リスト構築 */
	TSharedRef<SWidget> BuildIssuesList();

	/** プロジェクトサマリー構築 */
	TSharedRef<SWidget> BuildProjectSummary();

	// ========== セクション構築 ==========

	/** ノードメトリクスセクション */
	TSharedRef<SWidget> BuildNodeMetricsSection();

	/** 依存関係セクション */
	TSharedRef<SWidget> BuildDependencySection();

	/** Tickセクション */
	TSharedRef<SWidget> BuildTickSection();

	/** C++化推奨セクション */
	TSharedRef<SWidget> BuildCppMigrationSection();

	// ========== ヘルパー ==========

	/** 信号機ライトを作成 */
	TSharedRef<SWidget> CreateTrafficLightCircle(EBPHealthLevel Level, bool bIsActive);

	/** スコアバー作成 */
	TSharedRef<SWidget> CreateScoreBar(float Score, const FString& Label, EBPHealthLevel Level);

	/** 展開可能セクション作成 */
	TSharedRef<SWidget> CreateExpandableSection(const FText& Title, TSharedRef<SWidget> Content, bool bInitiallyExpanded = true);

	/** キー・値行作成 */
	TSharedRef<SWidget> CreateKeyValueRow(const FString& Key, const FString& Value, FLinearColor ValueColor = FLinearColor::White);

	/** 問題行作成 */
	TSharedRef<SWidget> CreateIssueRow(const FBPIssue& Issue);

	/** ステータスバッジ作成 */
	TSharedRef<SWidget> CreateStatusBadge(const FString& Text, EBPHealthLevel Level);

	// ========== イベントハンドラ ==========

	/** 現在選択中のBPを分析 */
	FReply OnAnalyzeSelectedClicked();

	/** プロジェクト分析 */
	FReply OnAnalyzeProjectClicked();

	/** 閾値設定を開く */
	FReply OnOpenSettingsClicked();

	/** レポートをエクスポート */
	FReply OnExportReportClicked();

	/** UIを更新 */
	void RefreshUI();

	// ========== データ ==========

	/** アナライザーインスタンス */
	UPROPERTY()
	TObjectPtr<UBPComplexityAnalyzer> Analyzer;

	/** 現在の分析レポート */
	FBPAnalysisReport CurrentReport;

	/** プロジェクトサマリー */
	FBPProjectAnalysisSummary ProjectSummary;

	/** プロジェクト分析モードかどうか */
	bool bProjectMode = false;

	/** 現在分析中のBlueprintパス */
	FString CurrentBlueprintPath;

	// ========== UI参照 ==========

	/** メインコンテナ */
	TSharedPtr<SVerticalBox> MainContainer;

	/** 詳細パネルコンテナ */
	TSharedPtr<SVerticalBox> DetailContainer;

	/** 信号機コンテナ */
	TSharedPtr<SHorizontalBox> TrafficLightContainer;

	/** サマリーテキスト */
	TSharedPtr<STextBlock> SummaryText;

	/** スクロールボックス */
	TSharedPtr<SScrollBox> MainScrollBox;
};
