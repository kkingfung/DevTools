// Copyright DevTools. All Rights Reserved.

#include "STuningDashboardPanel.h"
#include "TuningSubsystem.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"
#include "EditorStyleSet.h"
#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/FileHelper.h"

#define LOCTEXT_NAMESPACE "TuningDashboard"

void STuningDashboardPanel::Construct(const FArguments& InArgs)
{
	// サブシステム取得
	TuningSubsystem = UTuningSubsystem::Get();

	// サンプルパラメータを登録（デモ用）
	if (TuningSubsystem && TuningSubsystem->GetAllParameters().Num() == 0)
	{
		// Character パラメータ
		{
			FTuningParameter Param;
			Param.ParameterId = FName("Character.Health");
			Param.DisplayName = TEXT("最大体力");
			Param.Description = TEXT("キャラクターの最大HP");
			Param.Layer = ETuningLayer::Character;
			Param.Category = TEXT("基本ステータス");
			Param.CurrentValue.ValueType = ETuningValueType::Float;
			Param.CurrentValue.FloatValue = 100.0f;
			Param.DefaultValue = Param.CurrentValue;
			Param.Threshold.MinValue = 50.0f;
			Param.Threshold.MaxValue = 500.0f;
			Param.Threshold.CriticalMinValue = 10.0f;
			Param.Threshold.CriticalMaxValue = 1000.0f;
			TuningSubsystem->RegisterParameter(Param);
		}
		{
			FTuningParameter Param;
			Param.ParameterId = FName("Character.MoveSpeed");
			Param.DisplayName = TEXT("移動速度");
			Param.Layer = ETuningLayer::Character;
			Param.Category = TEXT("基本ステータス");
			Param.CurrentValue.ValueType = ETuningValueType::Float;
			Param.CurrentValue.FloatValue = 600.0f;
			Param.DefaultValue = Param.CurrentValue;
			Param.Threshold.MinValue = 200.0f;
			Param.Threshold.MaxValue = 1200.0f;
			TuningSubsystem->RegisterParameter(Param);
		}
		// Weapon パラメータ
		{
			FTuningParameter Param;
			Param.ParameterId = FName("Weapon.Damage");
			Param.DisplayName = TEXT("基礎ダメージ");
			Param.Layer = ETuningLayer::Weapon;
			Param.Category = TEXT("攻撃");
			Param.CurrentValue.ValueType = ETuningValueType::Float;
			Param.CurrentValue.FloatValue = 25.0f;
			Param.DefaultValue = Param.CurrentValue;
			Param.Threshold.MinValue = 5.0f;
			Param.Threshold.MaxValue = 100.0f;
			TuningSubsystem->RegisterParameter(Param);
		}
		{
			FTuningParameter Param;
			Param.ParameterId = FName("Weapon.FireRate");
			Param.DisplayName = TEXT("発射速度");
			Param.Layer = ETuningLayer::Weapon;
			Param.Category = TEXT("攻撃");
			Param.CurrentValue.ValueType = ETuningValueType::Float;
			Param.CurrentValue.FloatValue = 10.0f;
			Param.DefaultValue = Param.CurrentValue;
			TuningSubsystem->RegisterParameter(Param);
		}
		// Skill パラメータ
		{
			FTuningParameter Param;
			Param.ParameterId = FName("Skill.Cooldown");
			Param.DisplayName = TEXT("クールダウン");
			Param.Layer = ETuningLayer::Skill;
			Param.Category = TEXT("スキル設定");
			Param.CurrentValue.ValueType = ETuningValueType::Float;
			Param.CurrentValue.FloatValue = 5.0f;
			Param.DefaultValue = Param.CurrentValue;
			TuningSubsystem->RegisterParameter(Param);
		}
	}

	ChildSlot
	[
		SNew(SVerticalBox)

		// ツールバー
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			BuildToolbar()
		]

		// レイヤータブ
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f, 0.0f)
		[
			BuildLayerTabs()
		]

		// メインコンテンツ
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SSplitter)
			.Orientation(Orient_Horizontal)

			// 左: パラメータリスト
			+ SSplitter::Slot()
			.Value(0.35f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(4.0f)
				[
					BuildParameterList()
				]
			]

			// 中央: 詳細 + 履歴
			+ SSplitter::Slot()
			.Value(0.35f)
			[
				SNew(SSplitter)
				.Orientation(Orient_Vertical)

				+ SSplitter::Slot()
				.Value(0.6f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(4.0f)
					[
						BuildDetailPanel()
					]
				]

				+ SSplitter::Slot()
				.Value(0.4f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(4.0f)
					[
						BuildHistoryPanel()
					]
				]
			]

			// 右: 比較 + ベンチマーク
			+ SSplitter::Slot()
			.Value(0.3f)
			[
				SNew(SSplitter)
				.Orientation(Orient_Vertical)

				+ SSplitter::Slot()
				.Value(0.5f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(4.0f)
					[
						BuildComparisonPanel()
					]
				]

				+ SSplitter::Slot()
				.Value(0.5f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(4.0f)
					[
						BuildBenchmarkPanel()
					]
				]
			]
		]
	];

	// 初期データ読み込み
	RefreshParameterList();
	RefreshHistoryList();
	UpdateLayerSummaries();

	// イベント購読
	if (TuningSubsystem)
	{
		OnParameterChangedHandle = TuningSubsystem->OnParameterChanged.AddLambda(
			[this](FName ParameterId, const FTuningValue& NewValue)
			{
				RefreshParameterList();
				RefreshHistoryList();
				UpdateLayerSummaries();
			}
		);
	}
}

STuningDashboardPanel::~STuningDashboardPanel()
{
	if (TuningSubsystem)
	{
		TuningSubsystem->OnParameterChanged.Remove(OnParameterChangedHandle);
	}
}

TSharedRef<SWidget> STuningDashboardPanel::BuildToolbar()
{
	return SNew(SHorizontalBox)

		// セッション管理
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SAssignNew(SessionNameInput, SEditableTextBox)
			.MinDesiredWidth(150.0f)
			.HintText(LOCTEXT("SessionName", "セッション名"))
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("StartSession", "セッション開始"))
			.OnClicked(this, &STuningDashboardPanel::OnStartSessionClicked)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("EndSession", "セッション終了"))
			.OnClicked(this, &STuningDashboardPanel::OnEndSessionClicked)
		]

		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNullWidget::NullWidget
		]

		// Undo/Redo
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Undo", "↩ 元に戻す"))
			.OnClicked(this, &STuningDashboardPanel::OnUndoClicked)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Redo", "やり直し ↪"))
			.OnClicked(this, &STuningDashboardPanel::OnRedoClicked)
		]

		// プリセット
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SAssignNew(PresetNameInput, SEditableTextBox)
			.MinDesiredWidth(120.0f)
			.HintText(LOCTEXT("PresetName", "プリセット名"))
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("SavePreset", "プリセット保存"))
			.OnClicked(this, &STuningDashboardPanel::OnSavePresetClicked)
		]

		// エクスポート/インポート
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Export", "エクスポート"))
			.OnClicked(this, &STuningDashboardPanel::OnExportClicked)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Import", "インポート"))
			.OnClicked(this, &STuningDashboardPanel::OnImportClicked)
		];
}

TSharedRef<SWidget> STuningDashboardPanel::BuildLayerTabs()
{
	TSharedRef<SHorizontalBox> TabBox = SNew(SHorizontalBox);

	// 各レイヤーのタブを作成
	TArray<ETuningLayer> Layers = {
		ETuningLayer::Character,
		ETuningLayer::Weapon,
		ETuningLayer::Skill,
		ETuningLayer::Stage,
		ETuningLayer::AI,
		ETuningLayer::Economy,
		ETuningLayer::Custom
	};

	for (ETuningLayer Layer : Layers)
	{
		TSharedPtr<SButton> TabButton;
		TSharedPtr<STextBlock> SummaryText;

		TabBox->AddSlot()
			.AutoWidth()
			.Padding(2.0f, 0.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(TabButton, SButton)
					.Text(FText::FromString(GetLayerName(Layer)))
					.ButtonColorAndOpacity_Lambda([this, Layer]()
					{
						return CurrentLayer == Layer
							? FLinearColor(0.2f, 0.4f, 0.8f)
							: FLinearColor(0.3f, 0.3f, 0.3f);
					})
					.OnClicked_Lambda([this, Layer]() -> FReply
					{
						OnLayerTabChanged(Layer);
						return FReply::Handled();
					})
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(SummaryText, STextBlock)
					.Text(LOCTEXT("NoParams", "0 params"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
					.ColorAndOpacity(FSlateColor(FLinearColor::Gray))
				]
			];

		LayerTabButtons.Add(Layer, TabButton);
		LayerSummaryTexts.Add(Layer, SummaryText);
	}

	return TabBox;
}

TSharedRef<SWidget> STuningDashboardPanel::BuildParameterList()
{
	return SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Parameters", "パラメータ"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(ParameterListView, SListView<TSharedPtr<FTuningParameterItem>>)
			.ListItemsSource(&ParameterItems)
			.OnGenerateRow(this, &STuningDashboardPanel::OnGenerateParameterRow)
			.OnSelectionChanged(this, &STuningDashboardPanel::OnParameterSelectionChanged)
			.SelectionMode(ESelectionMode::Single)
		];
}

TSharedRef<SWidget> STuningDashboardPanel::BuildDetailPanel()
{
	return SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Details", "詳細"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SScrollBox)

			+ SScrollBox::Slot()
			[
				SAssignNew(DetailContainer, SVerticalBox)
			]
		];
}

TSharedRef<SWidget> STuningDashboardPanel::BuildHistoryPanel()
{
	return SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("History", "変更履歴"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(HistoryListView, SListView<TSharedPtr<FTuningHistoryItem>>)
			.ListItemsSource(&HistoryItems)
			.OnGenerateRow(this, &STuningDashboardPanel::OnGenerateHistoryRow)
			.SelectionMode(ESelectionMode::None)
		];
}

TSharedRef<SWidget> STuningDashboardPanel::BuildComparisonPanel()
{
	return SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Comparison", "Before vs After"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SScrollBox)

			+ SScrollBox::Slot()
			[
				SAssignNew(ComparisonContainer, SVerticalBox)
			]
		];
}

TSharedRef<SWidget> STuningDashboardPanel::BuildBenchmarkPanel()
{
	return SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 4.0f)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Benchmark", "安全ベンチマーク"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("RunBenchmark", "実行"))
				.OnClicked(this, &STuningDashboardPanel::OnRunBenchmarkClicked)
			]
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SScrollBox)

			+ SScrollBox::Slot()
			[
				SAssignNew(BenchmarkContainer, SVerticalBox)
			]
		];
}

TSharedRef<ITableRow> STuningDashboardPanel::OnGenerateParameterRow(TSharedPtr<FTuningParameterItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	// 変更状態を更新
	if (TuningSubsystem)
	{
		FTuningParameter Param;
		if (TuningSubsystem->GetParameter(Item->Parameter.ParameterId, Param))
		{
			Item->Parameter = Param;
			Item->bIsModified = !FMath::IsNearlyZero(Param.CurrentValue.GetDifference(Param.DefaultValue));
			Item->WarningLevel = Param.Threshold.CheckValue(Param.CurrentValue.GetAsFloat());
		}
	}

	return SNew(STableRow<TSharedPtr<FTuningParameterItem>>, OwnerTable)
		[
			SNew(SHorizontalBox)

			// 警告インジケータ
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(8.0f)
				.HeightOverride(8.0f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor(GetWarningColor(Item->WarningLevel))
				]
			]

			// パラメータ名
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item->Parameter.DisplayName))
				.Font(Item->bIsModified
					? FCoreStyle::GetDefaultFontStyle("Bold", 10)
					: FCoreStyle::GetDefaultFontStyle("Regular", 10))
			]

			// 現在値
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.MinDesiredWidth(80.0f)
				[
					CreateValueEditor(Item)
				]
			]

			// リセットボタン
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.Text(LOCTEXT("Reset", "↺"))
				.ToolTipText(LOCTEXT("ResetTooltip", "デフォルトに戻す"))
				.IsEnabled(Item->bIsModified)
				.OnClicked_Lambda([this, Item]() -> FReply
				{
					OnResetClicked(Item->Parameter.ParameterId);
					return FReply::Handled();
				})
			]
		];
}

TSharedRef<SWidget> STuningDashboardPanel::CreateValueEditor(TSharedPtr<FTuningParameterItem> Item)
{
	const FTuningValue& Value = Item->Parameter.CurrentValue;
	FName ParamId = Item->Parameter.ParameterId;

	switch (Value.ValueType)
	{
	case ETuningValueType::Float:
		return SNew(SSpinBox<float>)
			.Value(Value.FloatValue)
			.MinValue(Item->Parameter.Threshold.CriticalMinValue)
			.MaxValue(Item->Parameter.Threshold.CriticalMaxValue)
			.OnValueChanged_Lambda([this, ParamId](float NewValue)
			{
				OnParameterValueChanged(ParamId, NewValue);
			})
			.OnValueCommitted_Lambda([this, ParamId](float NewValue, ETextCommit::Type CommitType)
			{
				OnParameterValueCommitted(ParamId, NewValue, CommitType);
			});

	case ETuningValueType::Integer:
		return SNew(SSpinBox<int32>)
			.Value(Value.IntValue)
			.OnValueChanged_Lambda([this, ParamId](int32 NewValue)
			{
				if (TuningSubsystem)
				{
					TuningSubsystem->SetIntValue(ParamId, NewValue);
				}
			});

	case ETuningValueType::Boolean:
		return SNew(SCheckBox)
			.IsChecked(Value.BoolValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			.OnCheckStateChanged_Lambda([this, ParamId](ECheckBoxState State)
			{
				if (TuningSubsystem)
				{
					TuningSubsystem->SetBoolValue(ParamId, State == ECheckBoxState::Checked);
				}
			});

	default:
		return SNew(STextBlock).Text(FText::FromString(Value.ToString()));
	}
}

TSharedRef<ITableRow> STuningDashboardPanel::OnGenerateHistoryRow(TSharedPtr<FTuningHistoryItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	float PercentChange = Item->Entry.GetPercentChange();
	FLinearColor ChangeColor = PercentChange >= 0
		? FLinearColor(0.2f, 0.8f, 0.2f)
		: FLinearColor(0.8f, 0.2f, 0.2f);

	return SNew(STableRow<TSharedPtr<FTuningHistoryItem>>, OwnerTable)
		[
			SNew(SHorizontalBox)

			// 時刻
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FormatTimestamp(Item->Entry.Timestamp)))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FSlateColor(FLinearColor::Gray))
			]

			// パラメータ名
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(4.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item->Entry.ParameterId.ToString()))
			]

			// Before → After
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%s → %s"),
					*Item->Entry.OldValue.ToString(),
					*Item->Entry.NewValue.ToString())))
			]

			// 変化率
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%+.1f%%"), PercentChange)))
				.ColorAndOpacity(FSlateColor(ChangeColor))
			]
		];
}

void STuningDashboardPanel::OnParameterSelectionChanged(TSharedPtr<FTuningParameterItem> Item, ESelectInfo::Type SelectInfo)
{
	SelectedParameter = Item;
	RefreshDetailPanel();
}

void STuningDashboardPanel::OnLayerTabChanged(ETuningLayer NewLayer)
{
	CurrentLayer = NewLayer;
	RefreshParameterList();
}

void STuningDashboardPanel::OnParameterValueChanged(FName ParameterId, float NewValue)
{
	// リアルタイムプレビュー（コミット前）
}

void STuningDashboardPanel::OnParameterValueCommitted(FName ParameterId, float NewValue, ETextCommit::Type CommitType)
{
	if (TuningSubsystem)
	{
		TuningSubsystem->SetFloatValue(ParameterId, NewValue);
	}
}

void STuningDashboardPanel::OnResetClicked(FName ParameterId)
{
	if (TuningSubsystem)
	{
		TuningSubsystem->ResetToDefault(ParameterId);
	}
}

FReply STuningDashboardPanel::OnUndoClicked()
{
	if (TuningSubsystem)
	{
		TuningSubsystem->UndoLastChange();
	}
	return FReply::Handled();
}

FReply STuningDashboardPanel::OnRedoClicked()
{
	if (TuningSubsystem)
	{
		TuningSubsystem->RedoChange();
	}
	return FReply::Handled();
}

FReply STuningDashboardPanel::OnSavePresetClicked()
{
	if (TuningSubsystem && PresetNameInput.IsValid())
	{
		FString Name = PresetNameInput->GetText().ToString();
		if (!Name.IsEmpty())
		{
			TuningSubsystem->SaveAsPreset(Name);
		}
	}
	return FReply::Handled();
}

FReply STuningDashboardPanel::OnRunBenchmarkClicked()
{
	if (TuningSubsystem)
	{
		LastBenchmarkResult = TuningSubsystem->RunSafetyBenchmark();
		RefreshBenchmarkPanel();
	}
	return FReply::Handled();
}

FReply STuningDashboardPanel::OnExportClicked()
{
	if (!TuningSubsystem) return FReply::Handled();

	TArray<FString> SaveFilenames;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

	if (DesktopPlatform)
	{
		bool bOpened = DesktopPlatform->SaveFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			TEXT("Export Tuning Data"),
			FPaths::ProjectSavedDir(),
			TEXT("TuningData.json"),
			TEXT("JSON Files (*.json)|*.json"),
			EFileDialogFlags::None,
			SaveFilenames
		);

		if (bOpened && SaveFilenames.Num() > 0)
		{
			TuningSubsystem->SaveToFile(SaveFilenames[0]);
		}
	}

	return FReply::Handled();
}

FReply STuningDashboardPanel::OnImportClicked()
{
	if (!TuningSubsystem) return FReply::Handled();

	TArray<FString> OpenFilenames;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

	if (DesktopPlatform)
	{
		bool bOpened = DesktopPlatform->OpenFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			TEXT("Import Tuning Data"),
			FPaths::ProjectSavedDir(),
			TEXT(""),
			TEXT("JSON Files (*.json)|*.json"),
			EFileDialogFlags::None,
			OpenFilenames
		);

		if (bOpened && OpenFilenames.Num() > 0)
		{
			TuningSubsystem->LoadFromFile(OpenFilenames[0]);
			RefreshParameterList();
		}
	}

	return FReply::Handled();
}

FReply STuningDashboardPanel::OnStartSessionClicked()
{
	if (TuningSubsystem && SessionNameInput.IsValid())
	{
		FString Name = SessionNameInput->GetText().ToString();
		if (Name.IsEmpty())
		{
			Name = FString::Printf(TEXT("Session_%s"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
		}
		TuningSubsystem->StartSession(Name);
	}
	return FReply::Handled();
}

FReply STuningDashboardPanel::OnEndSessionClicked()
{
	if (TuningSubsystem)
	{
		TuningSubsystem->EndCurrentSession();
	}
	return FReply::Handled();
}

void STuningDashboardPanel::RefreshParameterList()
{
	ParameterItems.Empty();

	if (TuningSubsystem)
	{
		TArray<FTuningParameter> Params = TuningSubsystem->GetParametersByLayer(CurrentLayer);
		for (const FTuningParameter& Param : Params)
		{
			ParameterItems.Add(MakeShared<FTuningParameterItem>(Param));
		}
	}

	if (ParameterListView.IsValid())
	{
		ParameterListView->RequestListRefresh();
	}

	// 比較パネルも更新
	if (TuningSubsystem && ComparisonContainer.IsValid())
	{
		ComparisonContainer->ClearChildren();
		TArray<FTuningComparison> Comparisons = TuningSubsystem->CompareWithDefault();

		for (const FTuningComparison& Comp : Comparisons)
		{
			ComparisonContainer->AddSlot()
				.AutoHeight()
				.Padding(2.0f)
				[
					CreateComparisonWidget(Comp)
				];
		}
	}
}

void STuningDashboardPanel::RefreshHistoryList()
{
	HistoryItems.Empty();

	if (TuningSubsystem)
	{
		TArray<FTuningHistoryEntry> History = TuningSubsystem->GetHistory(50);
		for (const FTuningHistoryEntry& Entry : History)
		{
			HistoryItems.Add(MakeShared<FTuningHistoryItem>(Entry));
		}
	}

	if (HistoryListView.IsValid())
	{
		HistoryListView->RequestListRefresh();
	}
}

void STuningDashboardPanel::RefreshDetailPanel()
{
	if (!DetailContainer.IsValid() || !SelectedParameter.IsValid())
	{
		return;
	}

	DetailContainer->ClearChildren();
	const FTuningParameter& Param = SelectedParameter->Parameter;

	// 基本情報
	DetailContainer->AddSlot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Param.DisplayName))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
		];

	DetailContainer->AddSlot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Param.Description))
			.AutoWrapText(true)
		];

	// 値情報
	DetailContainer->AddSlot()
		.AutoHeight()
		.Padding(4.0f, 8.0f, 4.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("現在値: %s"), *Param.CurrentValue.ToString())))
		];

	DetailContainer->AddSlot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("デフォルト: %s"), *Param.DefaultValue.ToString())))
		];

	// 閾値情報
	DetailContainer->AddSlot()
		.AutoHeight()
		.Padding(4.0f, 8.0f, 4.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Thresholds", "閾値設定"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
		];

	DetailContainer->AddSlot()
		.AutoHeight()
		.Padding(8.0f, 2.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("警告範囲: %.1f 〜 %.1f"),
				Param.Threshold.MinValue, Param.Threshold.MaxValue)))
		];

	DetailContainer->AddSlot()
		.AutoHeight()
		.Padding(8.0f, 2.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("危険範囲: %.1f 〜 %.1f"),
				Param.Threshold.CriticalMinValue, Param.Threshold.CriticalMaxValue)))
		];
}

void STuningDashboardPanel::RefreshBenchmarkPanel()
{
	if (!BenchmarkContainer.IsValid())
	{
		return;
	}

	BenchmarkContainer->ClearChildren();

	// 結果サマリー
	FLinearColor ResultColor = LastBenchmarkResult.bPassed
		? FLinearColor(0.2f, 0.8f, 0.2f)
		: FLinearColor(0.8f, 0.2f, 0.2f);

	BenchmarkContainer->AddSlot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(LastBenchmarkResult.bPassed ? TEXT("✓ PASSED") : TEXT("✗ FAILED")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
			.ColorAndOpacity(FSlateColor(ResultColor))
		];

	BenchmarkContainer->AddSlot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("チェック: %d パラメータ"),
				LastBenchmarkResult.CheckedParameterCount)))
		];

	// 致命的警告
	if (LastBenchmarkResult.CriticalWarnings.Num() > 0)
	{
		BenchmarkContainer->AddSlot()
			.AutoHeight()
			.Padding(4.0f, 8.0f, 4.0f, 4.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("🔴 致命的警告: %d"),
					LastBenchmarkResult.CriticalWarnings.Num())))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.2f, 0.2f)))
			];

		for (const FTuningComparison& Comp : LastBenchmarkResult.CriticalWarnings)
		{
			BenchmarkContainer->AddSlot()
				.AutoHeight()
				.Padding(8.0f, 2.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("• %s: %+.1f%%"),
						*Comp.Parameter.DisplayName, Comp.PercentChange)))
				];
		}
	}

	// 警告
	if (LastBenchmarkResult.Warnings.Num() > 0)
	{
		BenchmarkContainer->AddSlot()
			.AutoHeight()
			.Padding(4.0f, 8.0f, 4.0f, 4.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("🟡 警告: %d"),
					LastBenchmarkResult.Warnings.Num())))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.9f, 0.2f)))
			];

		for (const FTuningComparison& Comp : LastBenchmarkResult.Warnings)
		{
			BenchmarkContainer->AddSlot()
				.AutoHeight()
				.Padding(8.0f, 2.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("• %s: %+.1f%%"),
						*Comp.Parameter.DisplayName, Comp.PercentChange)))
				];
		}
	}
}

TSharedRef<SWidget> STuningDashboardPanel::CreateComparisonWidget(const FTuningComparison& Comparison)
{
	FLinearColor ChangeColor = Comparison.PercentChange >= 0
		? FLinearColor(0.2f, 0.8f, 0.2f)
		: FLinearColor(0.8f, 0.2f, 0.2f);

	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FSlateColor(GetWarningColor(Comparison.WarningLevel).GetSpecifiedColor() * 0.2f))
		.Padding(FMargin(4.0f))
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Comparison.Parameter.DisplayName))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%s → %s"),
					*Comparison.BeforeValue.ToString(),
					*Comparison.AfterValue.ToString())))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("(%+.1f%%)"), Comparison.PercentChange)))
				.ColorAndOpacity(FSlateColor(ChangeColor))
			]
		];
}

void STuningDashboardPanel::UpdateLayerSummaries()
{
	if (!TuningSubsystem)
	{
		return;
	}

	TArray<FTuningLayerSummary> Summaries = TuningSubsystem->GetLayerSummaries();

	for (const FTuningLayerSummary& Summary : Summaries)
	{
		TSharedPtr<STextBlock>* TextBlock = LayerSummaryTexts.Find(Summary.Layer);
		if (TextBlock && TextBlock->IsValid())
		{
			FString Text;
			if (Summary.CriticalCount > 0)
			{
				Text = FString::Printf(TEXT("%d params (🔴%d)"), Summary.ParameterCount, Summary.CriticalCount);
			}
			else if (Summary.WarningCount > 0)
			{
				Text = FString::Printf(TEXT("%d params (🟡%d)"), Summary.ParameterCount, Summary.WarningCount);
			}
			else if (Summary.ModifiedCount > 0)
			{
				Text = FString::Printf(TEXT("%d params (*%d)"), Summary.ParameterCount, Summary.ModifiedCount);
			}
			else
			{
				Text = FString::Printf(TEXT("%d params"), Summary.ParameterCount);
			}

			(*TextBlock)->SetText(FText::FromString(Text));
		}
	}
}

FSlateColor STuningDashboardPanel::GetWarningColor(ETuningWarningLevel Level) const
{
	switch (Level)
	{
	case ETuningWarningLevel::Critical:
		return FSlateColor(FLinearColor(0.9f, 0.2f, 0.2f));
	case ETuningWarningLevel::Warning:
		return FSlateColor(FLinearColor(0.9f, 0.9f, 0.2f));
	case ETuningWarningLevel::Info:
		return FSlateColor(FLinearColor(0.2f, 0.6f, 0.9f));
	default:
		return FSlateColor(FLinearColor(0.3f, 0.8f, 0.3f));
	}
}

FString STuningDashboardPanel::GetLayerName(ETuningLayer Layer) const
{
	switch (Layer)
	{
	case ETuningLayer::Character: return TEXT("キャラクター");
	case ETuningLayer::Weapon: return TEXT("武器");
	case ETuningLayer::Skill: return TEXT("スキル");
	case ETuningLayer::Stage: return TEXT("ステージ");
	case ETuningLayer::AI: return TEXT("AI");
	case ETuningLayer::Economy: return TEXT("経済");
	case ETuningLayer::Custom: return TEXT("カスタム");
	default: return TEXT("不明");
	}
}

FString STuningDashboardPanel::FormatTimestamp(const FDateTime& Time) const
{
	return Time.ToString(TEXT("%H:%M:%S"));
}

#undef LOCTEXT_NAMESPACE
