// Copyright DevTools. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "TuningTypes.h"

class UTuningSubsystem;
class ITableRow;
class STableViewBase;

/**
 * パラメータリスト表示用アイテム
 */
class FTuningParameterItem : public TSharedFromThis<FTuningParameterItem>
{
public:
	FTuningParameter Parameter;
	bool bIsModified = false;
	ETuningWarningLevel WarningLevel = ETuningWarningLevel::None;

	FTuningParameterItem(const FTuningParameter& InParam)
		: Parameter(InParam)
	{
	}
};

/**
 * 履歴リスト表示用アイテム
 */
class FTuningHistoryItem : public TSharedFromThis<FTuningHistoryItem>
{
public:
	FTuningHistoryEntry Entry;

	FTuningHistoryItem(const FTuningHistoryEntry& InEntry)
		: Entry(InEntry)
	{
	}
};

/**
 * Gameplay Live Tuning Dashboard UIパネル
 */
class STuningDashboardPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STuningDashboardPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~STuningDashboardPanel();

private:
	// ========== UI構築 ==========

	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildLayerTabs();
	TSharedRef<SWidget> BuildParameterList();
	TSharedRef<SWidget> BuildDetailPanel();
	TSharedRef<SWidget> BuildHistoryPanel();
	TSharedRef<SWidget> BuildComparisonPanel();
	TSharedRef<SWidget> BuildBenchmarkPanel();

	// ========== パラメータリスト ==========

	TSharedRef<ITableRow> OnGenerateParameterRow(TSharedPtr<FTuningParameterItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnParameterSelectionChanged(TSharedPtr<FTuningParameterItem> Item, ESelectInfo::Type SelectInfo);

	// ========== 履歴リスト ==========

	TSharedRef<ITableRow> OnGenerateHistoryRow(TSharedPtr<FTuningHistoryItem> Item, const TSharedRef<STableViewBase>& OwnerTable);

	// ========== イベントハンドラ ==========

	void OnLayerTabChanged(ETuningLayer NewLayer);
	void OnParameterValueChanged(FName ParameterId, float NewValue);
	void OnParameterValueCommitted(FName ParameterId, float NewValue, ETextCommit::Type CommitType);
	void OnResetClicked(FName ParameterId);
	FReply OnUndoClicked();
	FReply OnRedoClicked();
	FReply OnSavePresetClicked();
	FReply OnRunBenchmarkClicked();
	FReply OnExportClicked();
	FReply OnImportClicked();
	FReply OnStartSessionClicked();
	FReply OnEndSessionClicked();

	// ========== ヘルパー ==========

	void RefreshParameterList();
	void RefreshHistoryList();
	void RefreshDetailPanel();
	void RefreshBenchmarkPanel();
	void UpdateLayerSummaries();

	FSlateColor GetLayerTabColor(ETuningLayer Layer) const;
	FSlateColor GetWarningColor(ETuningWarningLevel Level) const;
	FString GetLayerName(ETuningLayer Layer) const;
	FString FormatTimestamp(const FDateTime& Time) const;

	/** 値編集ウィジェットを生成 */
	TSharedRef<SWidget> CreateValueEditor(TSharedPtr<FTuningParameterItem> Item);

	/** Before/After比較ウィジェットを生成 */
	TSharedRef<SWidget> CreateComparisonWidget(const FTuningComparison& Comparison);

private:
	/** サブシステム参照 */
	UTuningSubsystem* TuningSubsystem = nullptr;

	/** 現在選択中のレイヤー */
	ETuningLayer CurrentLayer = ETuningLayer::Character;

	/** パラメータリストアイテム */
	TArray<TSharedPtr<FTuningParameterItem>> ParameterItems;

	/** 履歴リストアイテム */
	TArray<TSharedPtr<FTuningHistoryItem>> HistoryItems;

	/** パラメータリストビュー */
	TSharedPtr<SListView<TSharedPtr<FTuningParameterItem>>> ParameterListView;

	/** 履歴リストビュー */
	TSharedPtr<SListView<TSharedPtr<FTuningHistoryItem>>> HistoryListView;

	/** 選択中のパラメータ */
	TSharedPtr<FTuningParameterItem> SelectedParameter;

	/** 詳細パネル */
	TSharedPtr<SVerticalBox> DetailContainer;

	/** ベンチマーク結果パネル */
	TSharedPtr<SVerticalBox> BenchmarkContainer;

	/** 比較結果パネル */
	TSharedPtr<SVerticalBox> ComparisonContainer;

	/** レイヤータブボタン */
	TMap<ETuningLayer, TSharedPtr<SButton>> LayerTabButtons;

	/** レイヤーサマリー表示 */
	TMap<ETuningLayer, TSharedPtr<STextBlock>> LayerSummaryTexts;

	/** セッション名入力 */
	TSharedPtr<SEditableTextBox> SessionNameInput;

	/** プリセット名入力 */
	TSharedPtr<SEditableTextBox> PresetNameInput;

	/** 最新のベンチマーク結果 */
	FTuningBenchmarkResult LastBenchmarkResult;

	/** イベントハンドル */
	FDelegateHandle OnParameterChangedHandle;
	FDelegateHandle OnWarningTriggeredHandle;
};
