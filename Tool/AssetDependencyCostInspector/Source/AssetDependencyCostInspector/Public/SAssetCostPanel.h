// Copyright DevTools. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
#include "AssetCostTypes.h"

class UAssetCostAnalyzer;
class ITableRow;
class STableViewBase;

/**
 * 依存ノード表示用アイテム
 */
class FAssetCostTreeItem : public TSharedFromThis<FAssetCostTreeItem>
{
public:
	FAssetCostTreeItem(const FAssetDependencyNode& InNode)
		: Node(InNode)
	{
		for (const FAssetDependencyNode& ChildNode : Node.Children)
		{
			Children.Add(MakeShared<FAssetCostTreeItem>(ChildNode));
		}
	}

	FAssetDependencyNode Node;
	TArray<TSharedPtr<FAssetCostTreeItem>> Children;
};

/**
 * アセットコストインスペクターUIパネル
 */
class SAssetCostPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAssetCostPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** 選択アセットを分析 */
	void AnalyzeSelectedAssets();

	/** アセットパスを指定して分析 */
	void AnalyzeAsset(const FString& AssetPath);

	/** フォルダを分析 */
	void AnalyzeFolder(const FString& FolderPath);

private:
	/** UIを構築 */
	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildOverviewPanel();
	TSharedRef<SWidget> BuildDependencyTreePanel();
	TSharedRef<SWidget> BuildCostBreakdownPanel();
	TSharedRef<SWidget> BuildIssuesPanel();

	/** ツリービューコールバック */
	TSharedRef<ITableRow> OnGenerateTreeRow(TSharedPtr<FAssetCostTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnGetTreeChildren(TSharedPtr<FAssetCostTreeItem> Item, TArray<TSharedPtr<FAssetCostTreeItem>>& OutChildren);
	void OnTreeSelectionChanged(TSharedPtr<FAssetCostTreeItem> Item, ESelectInfo::Type SelectInfo);

	/** コストレベルに応じた色を取得 */
	FSlateColor GetCostLevelColor(EAssetCostLevel Level) const;
	FSlateColor GetCostBarColor(int64 Cost, int64 MaxCost) const;

	/** バイトサイズを人間可読形式に変換 */
	FString FormatBytes(int64 Bytes) const;

	/** パーセンテージバーを生成 */
	TSharedRef<SWidget> CreateCostBar(int64 Cost, int64 MaxCost, FLinearColor Color);

	/** 問題アイテムを生成 */
	TSharedRef<SWidget> CreateIssueItem(const FAssetIssue& Issue);

	/** レポートをリフレッシュ */
	void RefreshDisplay();

	/** エクスポートボタン */
	FReply OnExportClicked();

private:
	/** コストアナライザー */
	UPROPERTY()
	TObjectPtr<UAssetCostAnalyzer> Analyzer;

	/** 現在のレポート */
	FAssetCostReport CurrentReport;

	/** プロジェクトサマリー（フォルダ分析時） */
	FProjectCostSummary ProjectSummary;

	/** 依存ツリーアイテム */
	TArray<TSharedPtr<FAssetCostTreeItem>> TreeItems;

	/** ツリービュー */
	TSharedPtr<STreeView<TSharedPtr<FAssetCostTreeItem>>> DependencyTreeView;

	/** 概要テキスト */
	TSharedPtr<STextBlock> OverviewText;
	TSharedPtr<STextBlock> MemoryCostText;
	TSharedPtr<STextBlock> StreamingInfoText;
	TSharedPtr<STextBlock> LoadTimingText;
	TSharedPtr<STextBlock> UE5CostText;

	/** 問題リストコンテナ */
	TSharedPtr<SVerticalBox> IssuesContainer;

	/** 推奨事項コンテナ */
	TSharedPtr<SVerticalBox> SuggestionsContainer;

	/** アセットパス入力 */
	TSharedPtr<SEditableTextBox> AssetPathInput;

	/** 分析モード（単一/フォルダ） */
	bool bFolderMode = false;
};
