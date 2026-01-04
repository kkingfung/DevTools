// Copyright DevTools. All Rights Reserved.

#include "SUnifiedDebugPanel.h"
#include "DebugDataCollectorSubsystem.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"
#include "EditorStyleSet.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "UnifiedDebugPanel"

void SUnifiedDebugPanel::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		BuildMainLayout()
	];
}

void SUnifiedDebugPanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (!bAutoRefresh)
	{
		return;
	}

	TimeSinceLastRefresh += InDeltaTime;
	if (TimeSinceLastRefresh < RefreshInterval)
	{
		return;
	}
	TimeSinceLastRefresh = 0.0f;

	// データ更新
	if (UDebugDataCollectorSubsystem* Subsystem = GetDebugSubsystem())
	{
		CachedInsightData = Subsystem->GetAllInsightData();

		// UIを再構築
		if (ActorListContainer.IsValid())
		{
			ActorListContainer->ClearChildren();

			if (CachedInsightData.Num() == 0)
			{
				ActorListContainer->AddSlot()
				.AutoHeight()
				.Padding(10.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("NoWatchedActors", "監視対象がありません。\n「Watch Player」ボタンでプレイヤーを追加するか、\nBlueprintからWatchActorを呼び出してください。"))
					.ColorAndOpacity(FLinearColor::Gray)
				];
			}
			else
			{
				for (const FActorInsightData& Data : CachedInsightData)
				{
					ActorListContainer->AddSlot()
					.AutoHeight()
					.Padding(2.0f)
					[
						CreateActorInsightWidget(Data)
					];
				}
			}
		}
	}
}

TSharedRef<SWidget> SUnifiedDebugPanel::BuildMainLayout()
{
	return SNew(SVerticalBox)
		// ツールバー
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			BuildToolbar()
		]

		// セパレータ
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSeparator)
		]

		// メインコンテンツ（スプリッター）
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SSplitter)
			.Orientation(Orient_Horizontal)

			// 左: アクターリスト
			+ SSplitter::Slot()
			.Value(0.35f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(4.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("WatchedActors", "監視対象アクター"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
					]

					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SAssignNew(ActorListContainer, SVerticalBox)
						]
					]
				]
			]

			// 右: 詳細表示
			+ SSplitter::Slot()
			.Value(0.65f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(4.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("InsightDetails", "Insight 詳細"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.0f)
					[
						SAssignNew(SummaryText, STextBlock)
						.Text(LOCTEXT("SelectActor", "アクターを選択してください"))
						.AutoWrapText(true)
						.ColorAndOpacity(FLinearColor::Gray)
					]

					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SAssignNew(MainScrollBox, SScrollBox)
						+ SScrollBox::Slot()
						[
							SAssignNew(DetailPanelContainer, SVerticalBox)
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SUnifiedDebugPanel::BuildToolbar()
{
	return SNew(SHorizontalBox)
		// Watch Player ボタン
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("WatchPlayer", "Watch Player"))
			.ToolTipText(LOCTEXT("WatchPlayerTooltip", "プレイヤーのPawnを監視対象に追加"))
			.OnClicked(this, &SUnifiedDebugPanel::OnWatchPlayerPawnClicked)
		]

		// Clear All ボタン
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("ClearAll", "Clear All"))
			.ToolTipText(LOCTEXT("ClearAllTooltip", "全ての監視対象を解除"))
			.OnClicked(this, &SUnifiedDebugPanel::OnClearAllClicked)
		]

		// スペーサー
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNullWidget::NullWidget
		]

		// 自動更新チェックボックス
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.IsChecked(bAutoRefresh ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			.OnCheckStateChanged(this, &SUnifiedDebugPanel::OnAutoRefreshChanged)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AutoRefresh", "自動更新"))
			]
		]

		// Refresh ボタン
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Refresh", "Refresh"))
			.OnClicked(this, &SUnifiedDebugPanel::OnRefreshClicked)
		];
}

TSharedRef<SWidget> SUnifiedDebugPanel::CreateActorInsightWidget(const FActorInsightData& Data)
{
	AActor* Actor = Data.Actor.Get();
	FString ActorName = Actor ? Actor->GetName() : TEXT("Invalid");
	bool bIsSelected = (SelectedActor.Get() == Actor);

	// クリック時の選択変更用ラムダ
	TWeakObjectPtr<AActor> WeakActor = Data.Actor;

	return SNew(SBorder)
		.BorderImage(bIsSelected ? FAppStyle::GetBrush("DetailsView.CategoryTop") : FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8.0f)
		.OnMouseButtonDown_Lambda([this, WeakActor](const FGeometry&, const FPointerEvent&) -> FReply
		{
			OnActorSelectionChanged(WeakActor);
			return FReply::Handled();
		})
		[
			SNew(SVerticalBox)
			// アクター名とステータス
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(FText::FromString(ActorName))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				[
					CreateStatusBadge(
						Data.BasicState.bIsActive ? TEXT("Active") : TEXT("Hidden"),
						Data.BasicState.bIsActive ? FLinearColor::Green : FLinearColor::Gray
					)
				]
			]

			// サマリー
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Data.HumanReadableSummary))
				.AutoWrapText(true)
				.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			]

			// クイック情報（アビリティ・エフェクト数）
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					CreateStatusBadge(
						FString::Printf(TEXT("Abilities: %d"), Data.ActiveAbilities.Num()),
						Data.ActiveAbilities.Num() > 0 ? FLinearColor(0.2f, 0.6f, 1.0f) : FLinearColor::Gray
					)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					CreateStatusBadge(
						FString::Printf(TEXT("Effects: %d"), Data.ActiveEffects.Num()),
						Data.ActiveEffects.Num() > 0 ? FLinearColor(0.8f, 0.4f, 1.0f) : FLinearColor::Gray
					)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					CreateStatusBadge(
						FString::Printf(TEXT("Montages: %d"), Data.ActiveMontages.Num()),
						Data.ActiveMontages.Num() > 0 ? FLinearColor(1.0f, 0.6f, 0.2f) : FLinearColor::Gray
					)
				]
			]
		];
}

TSharedRef<SWidget> SUnifiedDebugPanel::CreateBasicInfoSection(const FActorInsightData& Data)
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[CreateKeyValueRow(TEXT("Class"), Data.BasicState.ClassName)]
		+ SVerticalBox::Slot().AutoHeight()[CreateKeyValueRow(TEXT("Location"), Data.BasicState.Location.ToString())]
		+ SVerticalBox::Slot().AutoHeight()[CreateKeyValueRow(TEXT("Rotation"), Data.BasicState.Rotation.ToString())]
		+ SVerticalBox::Slot().AutoHeight()[CreateKeyValueRow(TEXT("Velocity"), FString::Printf(TEXT("%.1f cm/s"), Data.BasicState.Velocity.Size()))]
		+ SVerticalBox::Slot().AutoHeight()[CreateKeyValueRow(TEXT("Tick Enabled"), Data.BasicState.bIsTickEnabled ? TEXT("Yes") : TEXT("No"))];
}

TSharedRef<SWidget> SUnifiedDebugPanel::CreateAbilitySection(const FActorInsightData& Data)
{
	TSharedRef<SVerticalBox> Container = SNew(SVerticalBox);

	// アクティブなアビリティ
	if (Data.ActiveAbilities.Num() > 0)
	{
		Container->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ActiveAbilities", "実行中アビリティ:"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			.ColorAndOpacity(FLinearColor::Green)
		];

		for (const FAbilityDebugInfo& Ability : Data.ActiveAbilities)
		{
			Container->AddSlot()
			.AutoHeight()
			.Padding(16.0f, 2.0f, 0.0f, 2.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Ability.AbilityName))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					CreateKeyValueRow(TEXT("  Level"), FString::FromInt(Ability.Level))
				]
			];
		}
	}

	// 付与されているアビリティ一覧
	Container->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 8.0f, 0.0f, 4.0f)
	[
		SNew(STextBlock)
		.Text(FText::Format(LOCTEXT("GrantedAbilitiesFmt", "付与済みアビリティ ({0}):"), FText::AsNumber(Data.GrantedAbilities.Num())))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
	];

	for (const FAbilityDebugInfo& Ability : Data.GrantedAbilities)
	{
		FLinearColor StatusColor = Ability.bIsActive ? FLinearColor::Green :
								   Ability.bIsOnCooldown ? FLinearColor::Yellow : FLinearColor::Gray;

		Container->AddSlot()
		.AutoHeight()
		.Padding(16.0f, 1.0f, 0.0f, 1.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(Ability.AbilityName))
				.ColorAndOpacity(StatusColor)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(8.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Ability.bIsOnCooldown ?
					FString::Printf(TEXT("(CD: %.1fs)"), Ability.CooldownRemaining) : TEXT("")))
				.ColorAndOpacity(FLinearColor::Yellow)
			]
		];
	}

	return Container;
}

TSharedRef<SWidget> SUnifiedDebugPanel::CreateEffectSection(const FActorInsightData& Data)
{
	TSharedRef<SVerticalBox> Container = SNew(SVerticalBox);

	if (Data.ActiveEffects.Num() == 0)
	{
		Container->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoActiveEffects", "アクティブなエフェクトなし"))
			.ColorAndOpacity(FLinearColor::Gray)
		];
	}
	else
	{
		for (const FEffectDebugInfo& Effect : Data.ActiveEffects)
		{
			Container->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 2.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(STextBlock)
						.Text(FText::FromString(Effect.EffectName))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f)
					[
						CreateStatusBadge(FString::Printf(TEXT("x%d"), Effect.StackCount), FLinearColor(0.8f, 0.4f, 1.0f))
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					Effect.RemainingTime > 0.0f ?
						CreateProgressBar(Effect.RemainingTime / 10.0f, FString::Printf(TEXT("%.1fs remaining"), Effect.RemainingTime)) :
						SNullWidget::NullWidget
				]
			];
		}
	}

	return Container;
}

TSharedRef<SWidget> SUnifiedDebugPanel::CreateAnimationSection(const FActorInsightData& Data)
{
	TSharedRef<SVerticalBox> Container = SNew(SVerticalBox);

	// モンタージュ
	if (Data.ActiveMontages.Num() > 0)
	{
		Container->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ActiveMontages", "再生中モンタージュ:"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			.ColorAndOpacity(FLinearColor(1.0f, 0.6f, 0.2f))
		];

		for (const FMontageDebugInfo& Montage : Data.ActiveMontages)
		{
			Container->AddSlot()
			.AutoHeight()
			.Padding(16.0f, 2.0f, 0.0f, 2.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[CreateKeyValueRow(TEXT("Name"), Montage.MontageName)]
				+ SVerticalBox::Slot().AutoHeight()[CreateKeyValueRow(TEXT("Section"), Montage.CurrentSectionName)]
				+ SVerticalBox::Slot().AutoHeight()[CreateKeyValueRow(TEXT("Position"), FString::Printf(TEXT("%.2fs"), Montage.Position))]
				+ SVerticalBox::Slot().AutoHeight()[CreateKeyValueRow(TEXT("Play Rate"), FString::Printf(TEXT("%.2f"), Montage.PlayRate))]
				+ SVerticalBox::Slot().AutoHeight()[CreateKeyValueRow(TEXT("Remaining"), FString::Printf(TEXT("%.2fs"), Montage.RemainingTime))]
			];
		}
	}
	else
	{
		Container->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoActiveMontages", "再生中のモンタージュなし"))
			.ColorAndOpacity(FLinearColor::Gray)
		];
	}

	return Container;
}

TSharedRef<SWidget> SUnifiedDebugPanel::CreateAISection(const FActorInsightData& Data)
{
	TSharedRef<SVerticalBox> Container = SNew(SVerticalBox);

	// Behavior Tree
	if (Data.BehaviorTree.bIsRunning)
	{
		Container->AddSlot().AutoHeight()[CreateKeyValueRow(TEXT("Tree"), Data.BehaviorTree.TreeName)];
		Container->AddSlot().AutoHeight()[CreateKeyValueRow(TEXT("Current Node"), Data.BehaviorTree.CurrentNodeName)];

		if (Data.BehaviorTree.ActiveServices.Num() > 0)
		{
			Container->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 4.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ActiveServices", "Active Services:"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
			];

			for (const FString& Service : Data.BehaviorTree.ActiveServices)
			{
				Container->AddSlot()
				.AutoHeight()
				.Padding(16.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("• ") + Service))
				];
			}
		}
	}
	else
	{
		Container->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoBehaviorTree", "Behavior Tree 非実行"))
			.ColorAndOpacity(FLinearColor::Gray)
		];
	}

	// Blackboard
	if (Data.Blackboard.KeyValues.Num() > 0)
	{
		Container->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 8.0f, 0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Blackboard", "Blackboard:"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
		];

		for (const auto& Pair : Data.Blackboard.KeyValues)
		{
			Container->AddSlot()
			.AutoHeight()
			.Padding(16.0f, 1.0f, 0.0f, 1.0f)
			[
				CreateKeyValueRow(Pair.Key, Pair.Value)
			];
		}
	}

	return Container;
}

TSharedRef<SWidget> SUnifiedDebugPanel::CreateTickSection(const FActorInsightData& Data)
{
	TSharedRef<SVerticalBox> Container = SNew(SVerticalBox);

	if (Data.TickInfo.Num() == 0)
	{
		Container->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoTickInfo", "ティック情報なし"))
			.ColorAndOpacity(FLinearColor::Gray)
		];
	}
	else
	{
		for (const FTickDebugInfo& Tick : Data.TickInfo)
		{
			FLinearColor StatusColor = Tick.bIsEnabled ? FLinearColor::Green : FLinearColor::Gray;

			Container->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 1.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.4f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Tick.Name))
					.ColorAndOpacity(StatusColor)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.3f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Tick.TickGroup))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.3f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Tick.bIsEnabled ? TEXT("Enabled") : TEXT("Disabled")))
					.ColorAndOpacity(StatusColor)
				]
			];
		}
	}

	return Container;
}

TSharedRef<SWidget> SUnifiedDebugPanel::CreateGameplayTagsSection(const FActorInsightData& Data)
{
	TSharedRef<SVerticalBox> Container = SNew(SVerticalBox);

	if (Data.OwnedGameplayTags.IsEmpty())
	{
		Container->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoTags", "GameplayTags なし"))
			.ColorAndOpacity(FLinearColor::Gray)
		];
	}
	else
	{
		TSharedRef<SWrapBox> TagsWrap = SNew(SWrapBox)
			.UseAllottedSize(true);

		for (const FGameplayTag& Tag : Data.OwnedGameplayTags)
		{
			TagsWrap->AddSlot()
			.Padding(2.0f)
			[
				CreateStatusBadge(Tag.ToString(), FLinearColor(0.3f, 0.5f, 0.7f))
			];
		}

		Container->AddSlot()
		.AutoHeight()
		[
			TagsWrap
		];
	}

	return Container;
}

TSharedRef<SWidget> SUnifiedDebugPanel::CreateExpandableSection(const FText& Title, TSharedRef<SWidget> Content, bool bInitiallyExpanded)
{
	return SNew(SExpandableArea)
		.AreaTitle(Title)
		.InitiallyCollapsed(!bInitiallyExpanded)
		.Padding(FMargin(8.0f, 4.0f))
		.BodyContent()
		[
			Content
		];
}

TSharedRef<SWidget> SUnifiedDebugPanel::CreateKeyValueRow(const FString& Key, const FString& Value, FLinearColor ValueColor)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 8.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Key + TEXT(":")))
			.ColorAndOpacity(FLinearColor::Gray)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Value))
			.ColorAndOpacity(ValueColor)
		];
}

TSharedRef<SWidget> SUnifiedDebugPanel::CreateStatusBadge(const FString& Text, FLinearColor Color)
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
		.Padding(FMargin(6.0f, 2.0f))
		.BorderBackgroundColor(Color * 0.3f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Text))
			.ColorAndOpacity(Color)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		];
}

TSharedRef<SWidget> SUnifiedDebugPanel::CreateProgressBar(float Progress, const FString& Label)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.Padding(0.0f, 2.0f)
		[
			SNew(SBox)
			.HeightOverride(16.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ProgressBar.Background"))
				.Padding(0)
				[
					SNew(SBox)
					.WidthOverride(FMath::Clamp(Progress, 0.0f, 1.0f) * 200.0f)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("ProgressBar.Background"))
						.BorderBackgroundColor(FLinearColor(0.2f, 0.6f, 1.0f))
					]
				]
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Label))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
		];
}

FReply SUnifiedDebugPanel::OnRefreshClicked()
{
	TimeSinceLastRefresh = RefreshInterval; // 即座に更新をトリガー
	return FReply::Handled();
}

void SUnifiedDebugPanel::OnAutoRefreshChanged(ECheckBoxState NewState)
{
	bAutoRefresh = (NewState == ECheckBoxState::Checked);
}

FReply SUnifiedDebugPanel::OnWatchPlayerPawnClicked()
{
	if (UDebugDataCollectorSubsystem* Subsystem = GetDebugSubsystem())
	{
		Subsystem->WatchPlayerPawn(0);
	}
	return FReply::Handled();
}

FReply SUnifiedDebugPanel::OnClearAllClicked()
{
	if (UDebugDataCollectorSubsystem* Subsystem = GetDebugSubsystem())
	{
		Subsystem->ClearAllWatches();
	}
	SelectedActor.Reset();
	return FReply::Handled();
}

void SUnifiedDebugPanel::OnActorSelectionChanged(TWeakObjectPtr<AActor> NewSelection)
{
	SelectedActor = NewSelection;

	// 詳細パネルを更新
	if (DetailPanelContainer.IsValid())
	{
		DetailPanelContainer->ClearChildren();

		// 選択されたアクターのInsightデータを検索
		const FActorInsightData* SelectedData = nullptr;
		for (const FActorInsightData& Data : CachedInsightData)
		{
			if (Data.Actor == SelectedActor)
			{
				SelectedData = &Data;
				break;
			}
		}

		if (SelectedData)
		{
			// サマリー更新
			if (SummaryText.IsValid())
			{
				SummaryText->SetText(FText::FromString(SelectedData->HumanReadableSummary));
				SummaryText->SetColorAndOpacity(FLinearColor::White);
			}

			// 各セクションを追加
			DetailPanelContainer->AddSlot().AutoHeight().Padding(4.0f)
			[
				CreateExpandableSection(LOCTEXT("BasicInfo", "基本情報"), CreateBasicInfoSection(*SelectedData))
			];

			DetailPanelContainer->AddSlot().AutoHeight().Padding(4.0f)
			[
				CreateExpandableSection(LOCTEXT("Abilities", "アビリティ"), CreateAbilitySection(*SelectedData))
			];

			DetailPanelContainer->AddSlot().AutoHeight().Padding(4.0f)
			[
				CreateExpandableSection(LOCTEXT("Effects", "エフェクト"), CreateEffectSection(*SelectedData))
			];

			DetailPanelContainer->AddSlot().AutoHeight().Padding(4.0f)
			[
				CreateExpandableSection(LOCTEXT("Animation", "アニメーション"), CreateAnimationSection(*SelectedData))
			];

			DetailPanelContainer->AddSlot().AutoHeight().Padding(4.0f)
			[
				CreateExpandableSection(LOCTEXT("AI", "AI / Behavior Tree"), CreateAISection(*SelectedData))
			];

			DetailPanelContainer->AddSlot().AutoHeight().Padding(4.0f)
			[
				CreateExpandableSection(LOCTEXT("Tick", "ティック情報"), CreateTickSection(*SelectedData), false)
			];

			DetailPanelContainer->AddSlot().AutoHeight().Padding(4.0f)
			[
				CreateExpandableSection(LOCTEXT("Tags", "GameplayTags"), CreateGameplayTagsSection(*SelectedData))
			];
		}
	}

	// リストを再描画して選択状態を反映
	TimeSinceLastRefresh = RefreshInterval;
}

UDebugDataCollectorSubsystem* SUnifiedDebugPanel::GetDebugSubsystem() const
{
	if (GEditor)
	{
		if (UWorld* World = GEditor->GetEditorWorldContext().World())
		{
			return World->GetSubsystem<UDebugDataCollectorSubsystem>();
		}

		// PIE ワールドを取得
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE && Context.World())
			{
				return Context.World()->GetSubsystem<UDebugDataCollectorSubsystem>();
			}
		}
	}
	return nullptr;
}

#undef LOCTEXT_NAMESPACE
