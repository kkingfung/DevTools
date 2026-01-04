// Copyright DevTools. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "DebugDataTypes.h"

class UDebugDataCollectorSubsystem;
class SScrollBox;
class SVerticalBox;
class STextBlock;
class SExpandableArea;

/**
 * 統合デバッグパネル Slate ウィジェット
 * 監視対象アクターの情報を統合表示するメインUI
 */
class UNIFIEDEBUGPANELEDITOR_API SUnifiedDebugPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUnifiedDebugPanel) {}
	SLATE_END_ARGS()

	/** ウィジェット構築 */
	void Construct(const FArguments& InArgs);

	/** ティック処理 */
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	// ========== UI構築メソッド ==========

	/** メインレイアウト構築 */
	TSharedRef<SWidget> BuildMainLayout();

	/** ツールバー構築 */
	TSharedRef<SWidget> BuildToolbar();

	/** アクターリスト構築 */
	TSharedRef<SWidget> BuildActorList();

	/** 詳細パネル構築 */
	TSharedRef<SWidget> BuildDetailPanel();

	// ========== アクターInsight表示 ==========

	/** アクターInsightウィジェット生成 */
	TSharedRef<SWidget> CreateActorInsightWidget(const FActorInsightData& Data);

	/** 基本情報セクション */
	TSharedRef<SWidget> CreateBasicInfoSection(const FActorInsightData& Data);

	/** アビリティセクション */
	TSharedRef<SWidget> CreateAbilitySection(const FActorInsightData& Data);

	/** エフェクトセクション */
	TSharedRef<SWidget> CreateEffectSection(const FActorInsightData& Data);

	/** アニメーションセクション */
	TSharedRef<SWidget> CreateAnimationSection(const FActorInsightData& Data);

	/** AIセクション */
	TSharedRef<SWidget> CreateAISection(const FActorInsightData& Data);

	/** ティックセクション */
	TSharedRef<SWidget> CreateTickSection(const FActorInsightData& Data);

	/** GameplayTagsセクション */
	TSharedRef<SWidget> CreateGameplayTagsSection(const FActorInsightData& Data);

	// ========== ヘルパーメソッド ==========

	/** 展開可能エリア作成 */
	TSharedRef<SWidget> CreateExpandableSection(
		const FText& Title,
		TSharedRef<SWidget> Content,
		bool bInitiallyExpanded = true
	);

	/** キー・値ペアの行を作成 */
	TSharedRef<SWidget> CreateKeyValueRow(const FString& Key, const FString& Value, FLinearColor ValueColor = FLinearColor::White);

	/** ステータスバッジ作成 */
	TSharedRef<SWidget> CreateStatusBadge(const FString& Text, FLinearColor Color);

	/** プログレスバー作成 */
	TSharedRef<SWidget> CreateProgressBar(float Progress, const FString& Label);

	// ========== イベントハンドラ ==========

	/** 更新ボタンクリック */
	FReply OnRefreshClicked();

	/** 自動更新トグル */
	void OnAutoRefreshChanged(ECheckBoxState NewState);

	/** プレイヤーPawn監視ボタン */
	FReply OnWatchPlayerPawnClicked();

	/** 全クリアボタン */
	FReply OnClearAllClicked();

	/** アクター選択変更 */
	void OnActorSelectionChanged(TWeakObjectPtr<AActor> NewSelection);

	// ========== データ ==========

	/** 現在のワールドからサブシステムを取得 */
	UDebugDataCollectorSubsystem* GetDebugSubsystem() const;

	/** キャッシュされたInsightデータ */
	TArray<FActorInsightData> CachedInsightData;

	/** 選択中のアクター */
	TWeakObjectPtr<AActor> SelectedActor;

	/** 自動更新有効フラグ */
	bool bAutoRefresh = true;

	/** 更新間隔 */
	float RefreshInterval = 0.1f;

	/** 最後の更新からの経過時間 */
	float TimeSinceLastRefresh = 0.0f;

	// ========== UIウィジェット参照 ==========

	/** アクターリストコンテナ */
	TSharedPtr<SVerticalBox> ActorListContainer;

	/** 詳細パネルコンテナ */
	TSharedPtr<SVerticalBox> DetailPanelContainer;

	/** サマリーテキスト */
	TSharedPtr<STextBlock> SummaryText;

	/** スクロールボックス */
	TSharedPtr<SScrollBox> MainScrollBox;
};
