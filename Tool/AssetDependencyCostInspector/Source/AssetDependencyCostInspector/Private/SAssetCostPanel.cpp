// Copyright DevTools. All Rights Reserved.

#include "SAssetCostPanel.h"
#include "AssetCostAnalyzer.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Views/STableRow.h"
#include "EditorStyleSet.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Misc/FileHelper.h"
#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "AssetCostPanel"

void SAssetCostPanel::Construct(const FArguments& InArgs)
{
	// アナライザー作成
	Analyzer = NewObject<UAssetCostAnalyzer>();
	Analyzer->AddToRoot(); // GC防止

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

		// メインコンテンツ
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SSplitter)
			.Orientation(Orient_Horizontal)

			// 左パネル：依存ツリー
			+ SSplitter::Slot()
			.Value(0.4f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(4.0f)
				[
					BuildDependencyTreePanel()
				]
			]

			// 右パネル：詳細
			+ SSplitter::Slot()
			.Value(0.6f)
			[
				SNew(SScrollBox)

				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)

					// 概要
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.0f)
					[
						BuildOverviewPanel()
					]

					// コスト内訳
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.0f)
					[
						BuildCostBreakdownPanel()
					]

					// 問題・推奨
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(4.0f)
					[
						BuildIssuesPanel()
					]
				]
			]
		]
	];
}

TSharedRef<SWidget> SAssetCostPanel::BuildToolbar()
{
	return SNew(SHorizontalBox)

		// アセットパス入力
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.Padding(2.0f)
		[
			SAssignNew(AssetPathInput, SEditableTextBox)
			.HintText(LOCTEXT("AssetPathHint", "アセットパスを入力 (例: /Game/Characters/Hero)"))
		]

		// フォルダモードチェック
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([this]() { return bFolderMode ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bFolderMode = (State == ECheckBoxState::Checked); })
			[
				SNew(STextBlock)
				.Text(LOCTEXT("FolderMode", "フォルダ分析"))
			]
		]

		// 分析ボタン
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Analyze", "分析"))
			.OnClicked_Lambda([this]() -> FReply
			{
				FString Path = AssetPathInput->GetText().ToString();
				if (!Path.IsEmpty())
				{
					if (bFolderMode)
					{
						AnalyzeFolder(Path);
					}
					else
					{
						AnalyzeAsset(Path);
					}
				}
				return FReply::Handled();
			})
		]

		// 選択アセット分析
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("AnalyzeSelected", "選択を分析"))
			.OnClicked_Lambda([this]() -> FReply
			{
				AnalyzeSelectedAssets();
				return FReply::Handled();
			})
		]

		// エクスポート
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Export", "エクスポート"))
			.OnClicked(this, &SAssetCostPanel::OnExportClicked)
		];
}

TSharedRef<SWidget> SAssetCostPanel::BuildOverviewPanel()
{
	return SNew(SExpandableArea)
		.AreaTitle(LOCTEXT("Overview", "概要"))
		.InitiallyCollapsed(false)
		.BodyContent()
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f)
			[
				SAssignNew(OverviewText, STextBlock)
				.Text(LOCTEXT("NoAsset", "アセットを選択して分析してください"))
			]
		];
}

TSharedRef<SWidget> SAssetCostPanel::BuildDependencyTreePanel()
{
	return SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(4.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("DependencyTree", "依存ツリー"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(DependencyTreeView, STreeView<TSharedPtr<FAssetCostTreeItem>>)
			.TreeItemsSource(&TreeItems)
			.OnGenerateRow(this, &SAssetCostPanel::OnGenerateTreeRow)
			.OnGetChildren(this, &SAssetCostPanel::OnGetTreeChildren)
			.OnSelectionChanged(this, &SAssetCostPanel::OnTreeSelectionChanged)
			.SelectionMode(ESelectionMode::Single)
		];
}

TSharedRef<SWidget> SAssetCostPanel::BuildCostBreakdownPanel()
{
	return SNew(SExpandableArea)
		.AreaTitle(LOCTEXT("CostBreakdown", "コスト内訳"))
		.InitiallyCollapsed(false)
		.BodyContent()
		[
			SNew(SVerticalBox)

			// メモリコスト
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("MemoryCost", "メモリコスト"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(8.0f, 2.0f, 0.0f, 0.0f)
				[
					SAssignNew(MemoryCostText, STextBlock)
					.Text(LOCTEXT("NoData", "-"))
				]
			]

			// Streaming情報
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("StreamingInfo", "Streaming情報"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(8.0f, 2.0f, 0.0f, 0.0f)
				[
					SAssignNew(StreamingInfoText, STextBlock)
					.Text(LOCTEXT("NoData", "-"))
				]
			]

			// 読み込みタイミング
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("LoadTiming", "読み込みタイミング"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(8.0f, 2.0f, 0.0f, 0.0f)
				[
					SAssignNew(LoadTimingText, STextBlock)
					.Text(LOCTEXT("NoData", "-"))
				]
			]

			// UE5固有コスト
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("UE5Cost", "UE5固有コスト (Nanite/Lumen)"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(8.0f, 2.0f, 0.0f, 0.0f)
				[
					SAssignNew(UE5CostText, STextBlock)
					.Text(LOCTEXT("NoData", "-"))
				]
			]
		];
}

TSharedRef<SWidget> SAssetCostPanel::BuildIssuesPanel()
{
	return SNew(SVerticalBox)

		// 問題
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SExpandableArea)
			.AreaTitle(LOCTEXT("Issues", "検出された問題"))
			.InitiallyCollapsed(false)
			.BodyContent()
			[
				SAssignNew(IssuesContainer, SVerticalBox)
			]
		]

		// 推奨事項
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 8.0f, 0.0f, 0.0f)
		[
			SNew(SExpandableArea)
			.AreaTitle(LOCTEXT("Suggestions", "最適化推奨"))
			.InitiallyCollapsed(false)
			.BodyContent()
			[
				SAssignNew(SuggestionsContainer, SVerticalBox)
			]
		];
}

TSharedRef<ITableRow> SAssetCostPanel::OnGenerateTreeRow(TSharedPtr<FAssetCostTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	const FAssetDependencyInfo& Info = Item->Node.Info;
	FString DisplayName = FPaths::GetBaseFilename(Info.AssetPath);
	FString CostStr = FormatBytes(Info.MemoryCost);

	// コストレベル色
	FLinearColor CostColor = FLinearColor::White;
	if (Info.MemoryCost > 100 * 1024 * 1024) // 100MB+
	{
		CostColor = FLinearColor(0.9f, 0.2f, 0.2f); // 赤
	}
	else if (Info.MemoryCost > 50 * 1024 * 1024) // 50MB+
	{
		CostColor = FLinearColor(0.9f, 0.6f, 0.1f); // オレンジ
	}
	else if (Info.MemoryCost > 10 * 1024 * 1024) // 10MB+
	{
		CostColor = FLinearColor(0.9f, 0.9f, 0.2f); // 黄色
	}

	return SNew(STableRow<TSharedPtr<FAssetCostTreeItem>>, OwnerTable)
		[
			SNew(SHorizontalBox)

			// アセット名
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(DisplayName))
				.ToolTipText(FText::FromString(Info.AssetPath))
			]

			// 深度
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("D%d"), Info.Depth)))
				.ColorAndOpacity(FSlateColor(FLinearColor::Gray))
			]

			// コスト
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FSlateColor(CostColor * 0.3f))
				.Padding(FMargin(4.0f, 2.0f))
				[
					SNew(STextBlock)
					.Text(FText::FromString(CostStr))
					.ColorAndOpacity(FSlateColor(CostColor))
				]
			]

			// 循環参照警告
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Info.bIsCircular ? FText::FromString(TEXT("⚠")) : FText::GetEmpty())
				.ColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.6f, 0.1f)))
				.ToolTipText(LOCTEXT("CircularRef", "循環参照"))
				.Visibility(Info.bIsCircular ? EVisibility::Visible : EVisibility::Collapsed)
			]
		];
}

void SAssetCostPanel::OnGetTreeChildren(TSharedPtr<FAssetCostTreeItem> Item, TArray<TSharedPtr<FAssetCostTreeItem>>& OutChildren)
{
	OutChildren = Item->Children;
}

void SAssetCostPanel::OnTreeSelectionChanged(TSharedPtr<FAssetCostTreeItem> Item, ESelectInfo::Type SelectInfo)
{
	if (Item.IsValid())
	{
		// 選択されたアセットの詳細を表示
		AnalyzeAsset(Item->Node.Info.AssetPath);
	}
}

void SAssetCostPanel::AnalyzeSelectedAssets()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

	if (SelectedAssets.Num() > 0)
	{
		// 最初の選択アセットを分析
		AnalyzeAsset(SelectedAssets[0].GetObjectPathString());
	}
}

void SAssetCostPanel::AnalyzeAsset(const FString& AssetPath)
{
	if (!Analyzer)
	{
		return;
	}

	bFolderMode = false;
	CurrentReport = Analyzer->AnalyzeAsset(AssetPath);
	RefreshDisplay();
}

void SAssetCostPanel::AnalyzeFolder(const FString& FolderPath)
{
	if (!Analyzer)
	{
		return;
	}

	bFolderMode = true;
	ProjectSummary = Analyzer->AnalyzeFolder(FolderPath);

	// 最初のレポートを表示
	if (ProjectSummary.AssetReports.Num() > 0)
	{
		CurrentReport = ProjectSummary.AssetReports[0];
	}

	RefreshDisplay();
}

void SAssetCostPanel::RefreshDisplay()
{
	// 概要テキスト
	if (OverviewText.IsValid())
	{
		FString OverviewStr;
		if (bFolderMode)
		{
			OverviewStr = FString::Printf(
				TEXT("フォルダ: %s\n")
				TEXT("アセット数: %d\n")
				TEXT("合計メモリ: %s\n")
				TEXT("合計ディスク: %s"),
				*ProjectSummary.RootPath,
				ProjectSummary.TotalAssetCount,
				*FormatBytes(ProjectSummary.TotalMemoryCost),
				*FormatBytes(ProjectSummary.TotalDiskSize)
			);
		}
		else
		{
			OverviewStr = FString::Printf(
				TEXT("アセット: %s\n")
				TEXT("種別: %s\n")
				TEXT("コストレベル: %s\n")
				TEXT("依存数: %d (直接: %d)"),
				*CurrentReport.AssetName,
				*UAssetCostAnalyzer::GetCategoryName(CurrentReport.Category),
				*UAssetCostAnalyzer::GetCostLevelString(CurrentReport.OverallCostLevel),
				CurrentReport.TotalDependencyCount,
				CurrentReport.DirectDependencyCount
			);
		}
		OverviewText->SetText(FText::FromString(OverviewStr));
	}

	// メモリコスト
	if (MemoryCostText.IsValid())
	{
		const FAssetMemoryCost& Mem = CurrentReport.MemoryCost;
		FString MemStr = FString::Printf(
			TEXT("ディスク: %s\n")
			TEXT("メモリ: %s\n")
			TEXT("GPU: %s\n")
			TEXT("Naniteデータ: %s\n")
			TEXT("依存含む合計: %s"),
			*FormatBytes(Mem.DiskSize),
			*FormatBytes(Mem.MemorySize),
			*FormatBytes(Mem.GPUMemorySize),
			*FormatBytes(Mem.NaniteDataSize),
			*FormatBytes(Mem.TotalWithDependencies)
		);
		MemoryCostText->SetText(FText::FromString(MemStr));
	}

	// Streaming情報
	if (StreamingInfoText.IsValid())
	{
		const FAssetStreamingInfo& Stream = CurrentReport.StreamingInfo;
		FString StreamStr = FString::Printf(
			TEXT("Streamable: %s\n")
			TEXT("常駐サイズ: %s\n")
			TEXT("Streamサイズ: %s\n")
			TEXT("Mipレベル: %d (常駐: %d)\n")
			TEXT("優先度: %d"),
			Stream.bIsStreamable ? TEXT("はい") : TEXT("いいえ"),
			*FormatBytes(Stream.ResidentSize),
			*FormatBytes(Stream.StreamedSize),
			Stream.NumMipLevels,
			Stream.NumResidentMips,
			Stream.StreamingPriority
		);
		StreamingInfoText->SetText(FText::FromString(StreamStr));
	}

	// 読み込みタイミング
	if (LoadTimingText.IsValid())
	{
		const FAssetLoadTiming& Load = CurrentReport.LoadTiming;
		FString LoadStr = FString::Printf(
			TEXT("読み込みフェーズ: %s\n")
			TEXT("推定読み込み時間: %.1f ms\n")
			TEXT("ブロッキング: %s\n")
			TEXT("依存読み込み順序: %d"),
			*Load.LoadPhase,
			Load.EstimatedLoadTimeMs,
			Load.bIsBlockingLoad ? TEXT("はい") : TEXT("いいえ"),
			Load.DependencyLoadOrder
		);
		LoadTimingText->SetText(FText::FromString(LoadStr));
	}

	// UE5コスト
	if (UE5CostText.IsValid())
	{
		const FUE5SpecificCost& UE5 = CurrentReport.UE5Cost;
		FString UE5Str = FString::Printf(
			TEXT("Nanite有効: %s\n")
			TEXT("  - 三角形: %s\n")
			TEXT("  - フォールバック: %s\n")
			TEXT("Lumen対応: %s\n")
			TEXT("VSM対応: %s"),
			UE5.bNaniteEnabled ? TEXT("はい") : TEXT("いいえ"),
			*FString::FormatAsNumber(UE5.NaniteTriangleCount),
			*FormatBytes(UE5.NaniteFallbackSize),
			UE5.bLumenCompatible ? TEXT("はい") : TEXT("いいえ"),
			UE5.bVSMCompatible ? TEXT("はい") : TEXT("いいえ")
		);
		UE5CostText->SetText(FText::FromString(UE5Str));
	}

	// 依存ツリー更新
	TreeItems.Empty();
	if (!CurrentReport.AssetPath.IsEmpty())
	{
		FAssetDependencyNode RootNode = Analyzer->BuildDependencyTree(CurrentReport.AssetPath, 5);
		TreeItems.Add(MakeShared<FAssetCostTreeItem>(RootNode));
	}
	if (DependencyTreeView.IsValid())
	{
		DependencyTreeView->RequestTreeRefresh();
	}

	// 問題リスト更新
	if (IssuesContainer.IsValid())
	{
		IssuesContainer->ClearChildren();
		for (const FAssetIssue& Issue : CurrentReport.Issues)
		{
			IssuesContainer->AddSlot()
				.AutoHeight()
				.Padding(4.0f, 2.0f)
				[
					CreateIssueItem(Issue)
				];
		}

		if (CurrentReport.Issues.Num() == 0)
		{
			IssuesContainer->AddSlot()
				.AutoHeight()
				.Padding(4.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("NoIssues", "問題は検出されませんでした"))
					.ColorAndOpacity(FSlateColor(FLinearColor(0.3f, 0.8f, 0.3f)))
				];
		}
	}

	// 推奨事項更新
	if (SuggestionsContainer.IsValid())
	{
		SuggestionsContainer->ClearChildren();
		for (const FString& Suggestion : CurrentReport.OptimizationSuggestions)
		{
			SuggestionsContainer->AddSlot()
				.AutoHeight()
				.Padding(4.0f, 2.0f)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.0f, 0.0f, 4.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("💡")))
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Suggestion))
						.AutoWrapText(true)
					]
				];
		}

		if (CurrentReport.OptimizationSuggestions.Num() == 0)
		{
			SuggestionsContainer->AddSlot()
				.AutoHeight()
				.Padding(4.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("NoSuggestions", "最適化の推奨事項はありません"))
				];
		}
	}
}

TSharedRef<SWidget> SAssetCostPanel::CreateIssueItem(const FAssetIssue& Issue)
{
	FLinearColor SeverityColor;
	FString SeverityIcon;

	switch (Issue.Severity)
	{
	case EAssetCostLevel::Critical:
		SeverityColor = FLinearColor(0.9f, 0.1f, 0.1f);
		SeverityIcon = TEXT("🔴");
		break;
	case EAssetCostLevel::High:
		SeverityColor = FLinearColor(0.9f, 0.5f, 0.1f);
		SeverityIcon = TEXT("🟠");
		break;
	case EAssetCostLevel::Medium:
		SeverityColor = FLinearColor(0.9f, 0.9f, 0.2f);
		SeverityIcon = TEXT("🟡");
		break;
	default:
		SeverityColor = FLinearColor(0.3f, 0.8f, 0.3f);
		SeverityIcon = TEXT("🟢");
		break;
	}

	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FSlateColor(SeverityColor * 0.15f))
		.Padding(FMargin(8.0f, 4.0f))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(SeverityIcon))
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Issue.IssueType))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					.ColorAndOpacity(FSlateColor(SeverityColor))
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(24.0f, 2.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Issue.Description))
				.AutoWrapText(true)
			]
		];
}

FSlateColor SAssetCostPanel::GetCostLevelColor(EAssetCostLevel Level) const
{
	return FSlateColor(UAssetCostAnalyzer::GetCostLevelColor(Level));
}

FSlateColor SAssetCostPanel::GetCostBarColor(int64 Cost, int64 MaxCost) const
{
	float Ratio = MaxCost > 0 ? static_cast<float>(Cost) / static_cast<float>(MaxCost) : 0.0f;

	if (Ratio > 0.75f)
	{
		return FSlateColor(FLinearColor(0.9f, 0.2f, 0.2f));
	}
	else if (Ratio > 0.5f)
	{
		return FSlateColor(FLinearColor(0.9f, 0.6f, 0.1f));
	}
	else if (Ratio > 0.25f)
	{
		return FSlateColor(FLinearColor(0.9f, 0.9f, 0.2f));
	}
	return FSlateColor(FLinearColor(0.3f, 0.8f, 0.3f));
}

FString SAssetCostPanel::FormatBytes(int64 Bytes) const
{
	if (Bytes >= 1024LL * 1024LL * 1024LL)
	{
		return FString::Printf(TEXT("%.2f GB"), Bytes / (1024.0 * 1024.0 * 1024.0));
	}
	else if (Bytes >= 1024LL * 1024LL)
	{
		return FString::Printf(TEXT("%.2f MB"), Bytes / (1024.0 * 1024.0));
	}
	else if (Bytes >= 1024LL)
	{
		return FString::Printf(TEXT("%.2f KB"), Bytes / 1024.0);
	}
	return FString::Printf(TEXT("%lld B"), Bytes);
}

TSharedRef<SWidget> SAssetCostPanel::CreateCostBar(int64 Cost, int64 MaxCost, FLinearColor Color)
{
	float Ratio = MaxCost > 0 ? FMath::Clamp(static_cast<float>(Cost) / static_cast<float>(MaxCost), 0.0f, 1.0f) : 0.0f;

	return SNew(SBox)
		.HeightOverride(16.0f)
		[
			SNew(SOverlay)

			// 背景
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FSlateColor(FLinearColor(0.1f, 0.1f, 0.1f)))
			]

			// バー
			+ SOverlay::Slot()
			.HAlign(HAlign_Left)
			[
				SNew(SBox)
				.WidthOverride_Lambda([Ratio]() { return Ratio * 200.0f; })
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FSlateColor(Color))
				]
			]
		];
}

FReply SAssetCostPanel::OnExportClicked()
{
	// ファイル保存ダイアログ
	TArray<FString> SaveFilenames;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

	if (DesktopPlatform)
	{
		bool bOpened = DesktopPlatform->SaveFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
			TEXT("Export Asset Cost Report"),
			FPaths::ProjectSavedDir(),
			TEXT("AssetCostReport.txt"),
			TEXT("Text Files (*.txt)|*.txt|CSV Files (*.csv)|*.csv"),
			EFileDialogFlags::None,
			SaveFilenames
		);

		if (bOpened && SaveFilenames.Num() > 0)
		{
			FString OutputPath = SaveFilenames[0];
			FString Content;

			if (OutputPath.EndsWith(TEXT(".csv")))
			{
				// CSV形式
				Content = TEXT("AssetPath,Category,CostLevel,DiskSize,MemorySize,GPUMemory,DependencyCount\n");

				if (bFolderMode)
				{
					for (const FAssetCostReport& Report : ProjectSummary.AssetReports)
					{
						Content += FString::Printf(
							TEXT("%s,%s,%s,%lld,%lld,%lld,%d\n"),
							*Report.AssetPath,
							*UAssetCostAnalyzer::GetCategoryName(Report.Category),
							*UAssetCostAnalyzer::GetCostLevelString(Report.OverallCostLevel),
							Report.MemoryCost.DiskSize,
							Report.MemoryCost.MemorySize,
							Report.MemoryCost.GPUMemorySize,
							Report.TotalDependencyCount
						);
					}
				}
				else
				{
					Content += FString::Printf(
						TEXT("%s,%s,%s,%lld,%lld,%lld,%d\n"),
						*CurrentReport.AssetPath,
						*UAssetCostAnalyzer::GetCategoryName(CurrentReport.Category),
						*UAssetCostAnalyzer::GetCostLevelString(CurrentReport.OverallCostLevel),
						CurrentReport.MemoryCost.DiskSize,
						CurrentReport.MemoryCost.MemorySize,
						CurrentReport.MemoryCost.GPUMemorySize,
						CurrentReport.TotalDependencyCount
					);
				}
			}
			else
			{
				// テキスト形式
				Content = CurrentReport.HumanReadableSummary;
			}

			FFileHelper::SaveStringToFile(Content, *OutputPath);
		}
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
