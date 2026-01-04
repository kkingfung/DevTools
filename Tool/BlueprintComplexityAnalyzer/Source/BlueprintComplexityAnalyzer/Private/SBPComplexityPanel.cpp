// Copyright DevTools. All Rights Reserved.

#include "SBPComplexityPanel.h"
#include "BPComplexityAnalyzer.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"
#include "EditorStyleSet.h"
#include "Engine/Blueprint.h"
#include "Editor.h"
#include "Selection.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"
#include "DesktopPlatformModule.h"

#define LOCTEXT_NAMESPACE "BPComplexityPanel"

void SBPComplexityPanel::Construct(const FArguments& InArgs)
{
	// アナライザーを作成
	Analyzer = NewObject<UBPComplexityAnalyzer>();
	Analyzer->AddToRoot(); // GC防止

	ChildSlot
	[
		BuildMainLayout()
	];
}

TSharedRef<SWidget> SBPComplexityPanel::BuildMainLayout()
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

		// メインコンテンツ
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(MainContainer, SVerticalBox)
				// 信号機とスコア
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(8.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						BuildTrafficLight()
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(16.0f, 0.0f, 0.0f, 0.0f)
					[
						BuildScoreDisplay()
					]
				]

				// サマリー
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(8.0f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(8.0f)
					[
						SAssignNew(SummaryText, STextBlock)
						.Text(LOCTEXT("SelectBlueprint", "Blueprintを選択して「Analyze Selected」をクリックしてください"))
						.AutoWrapText(true)
					]
				]

				// 詳細パネル
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(8.0f)
				[
					SAssignNew(DetailContainer, SVerticalBox)
				]
			]
		];
}

TSharedRef<SWidget> SBPComplexityPanel::BuildToolbar()
{
	return SNew(SHorizontalBox)
		// Analyze Selected ボタン
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("AnalyzeSelected", "Analyze Selected"))
			.ToolTipText(LOCTEXT("AnalyzeSelectedTooltip", "コンテンツブラウザで選択中のBlueprintを分析"))
			.OnClicked(this, &SBPComplexityPanel::OnAnalyzeSelectedClicked)
		]

		// Analyze Project ボタン
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("AnalyzeProject", "Analyze Project"))
			.ToolTipText(LOCTEXT("AnalyzeProjectTooltip", "プロジェクト内の全Blueprintを分析"))
			.OnClicked(this, &SBPComplexityPanel::OnAnalyzeProjectClicked)
		]

		// スペーサー
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNullWidget::NullWidget
		]

		// Export ボタン
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Export", "Export Report"))
			.ToolTipText(LOCTEXT("ExportTooltip", "分析結果をCSV形式でエクスポート"))
			.OnClicked(this, &SBPComplexityPanel::OnExportReportClicked)
		];
}

TSharedRef<SWidget> SBPComplexityPanel::BuildTrafficLight()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
		.Padding(8.0f)
		[
			SAssignNew(TrafficLightContainer, SHorizontalBox)
			// 縦に3つの信号を配置
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(2.0f)
				[
					CreateTrafficLightCircle(EBPHealthLevel::Red, false)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(2.0f)
				[
					CreateTrafficLightCircle(EBPHealthLevel::Yellow, false)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(2.0f)
				[
					CreateTrafficLightCircle(EBPHealthLevel::Green, false)
				]
			]
		];
}

TSharedRef<SWidget> SBPComplexityPanel::CreateTrafficLightCircle(EBPHealthLevel Level, bool bIsActive)
{
	FLinearColor Color = UBPComplexityAnalyzer::GetHealthLevelColor(Level);
	if (!bIsActive)
	{
		Color = Color * 0.3f; // 非アクティブは暗くする
	}

	return SNew(SBox)
		.WidthOverride(30.0f)
		.HeightOverride(30.0f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(Color)
			.Padding(0)
			[
				// 円形に見せるためのオーバーレイ（簡易実装）
				SNullWidget::NullWidget
			]
		];
}

TSharedRef<SWidget> SBPComplexityPanel::BuildScoreDisplay()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ScoreTitle", "複雑度スコア"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 8.0f)
		[
			CreateScoreBar(0.0f, TEXT("総合スコア"), EBPHealthLevel::Green)
		];
}

TSharedRef<SWidget> SBPComplexityPanel::CreateScoreBar(float Score, const FString& Label, EBPHealthLevel Level)
{
	FLinearColor Color = UBPComplexityAnalyzer::GetHealthLevelColor(Level);

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("%.0f / 100"), Score)))
				.ColorAndOpacity(Color)
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 2.0f)
		[
			SNew(SBox)
			.HeightOverride(8.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ProgressBar.Background"))
				.Padding(0)
				[
					SNew(SBox)
					.WidthOverride(FMath::Clamp(Score / 100.0f, 0.0f, 1.0f) * 300.0f)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("ProgressBar.Background"))
						.BorderBackgroundColor(Color)
					]
				]
			]
		];
}

TSharedRef<SWidget> SBPComplexityPanel::BuildDetailPanel()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f)
		[
			BuildNodeMetricsSection()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f)
		[
			BuildDependencySection()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f)
		[
			BuildTickSection()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f)
		[
			BuildCppMigrationSection()
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f)
		[
			BuildIssuesList()
		];
}

TSharedRef<SWidget> SBPComplexityPanel::BuildNodeMetricsSection()
{
	return CreateExpandableSection(
		LOCTEXT("NodeMetrics", "ノード数分析"),
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			CreateKeyValueRow(TEXT("総ノード数"), FString::FromInt(CurrentReport.NodeMetrics.TotalNodeCount))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			CreateKeyValueRow(TEXT("関数呼び出し"), FString::FromInt(CurrentReport.NodeMetrics.FunctionCallCount))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			CreateKeyValueRow(TEXT("変数アクセス"), FString::FromInt(CurrentReport.NodeMetrics.VariableAccessCount))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			CreateKeyValueRow(TEXT("制御フロー"), FString::FromInt(CurrentReport.NodeMetrics.ControlFlowCount))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			CreateKeyValueRow(TEXT("最大グラフ"), FString::Printf(TEXT("%s (%d nodes)"),
				*CurrentReport.NodeMetrics.LargestGraphName, CurrentReport.NodeMetrics.LargestGraphNodeCount))
		]
	);
}

TSharedRef<SWidget> SBPComplexityPanel::BuildDependencySection()
{
	return CreateExpandableSection(
		LOCTEXT("Dependencies", "依存関係分析"),
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			CreateKeyValueRow(TEXT("直接依存数"), FString::FromInt(CurrentReport.DependencyMetrics.DirectDependencyCount))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			CreateKeyValueRow(TEXT("依存深度"), FString::FromInt(CurrentReport.DependencyMetrics.MaxDependencyDepth))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			CreateKeyValueRow(TEXT("循環参照"),
				FString::FromInt(CurrentReport.DependencyMetrics.CircularReferenceCount),
				CurrentReport.DependencyMetrics.CircularReferenceCount > 0 ? FLinearColor::Red : FLinearColor::Green)
		]
	);
}

TSharedRef<SWidget> SBPComplexityPanel::BuildTickSection()
{
	FString TickStatus = CurrentReport.TickMetrics.bUsesTick ?
		FString::Printf(TEXT("使用中 (%d nodes)"), CurrentReport.TickMetrics.TotalNodesInTick) :
		TEXT("未使用");

	FLinearColor TickColor = CurrentReport.TickMetrics.bUsesTick ?
		(CurrentReport.TickMetrics.HealthLevel == EBPHealthLevel::Red ? FLinearColor::Red : FLinearColor::Yellow) :
		FLinearColor::Green;

	return CreateExpandableSection(
		LOCTEXT("TickUsage", "Tick使用分析"),
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			CreateKeyValueRow(TEXT("Tick"), TickStatus, TickColor)
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			CreateKeyValueRow(TEXT("Tickイベント数"), FString::FromInt(CurrentReport.TickMetrics.TickEventCount))
		]
	);
}

TSharedRef<SWidget> SBPComplexityPanel::BuildCppMigrationSection()
{
	return CreateExpandableSection(
		LOCTEXT("CppMigration", "C++化推奨度"),
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			CreateKeyValueRow(TEXT("推奨度スコア"),
				FString::Printf(TEXT("%.0f%%"), CurrentReport.CppMigrationMetrics.MigrationScore),
				UBPComplexityAnalyzer::GetHealthLevelColor(CurrentReport.CppMigrationMetrics.Priority))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			CreateKeyValueRow(TEXT("移行難易度"),
				FString::Printf(TEXT("%d / 5"), CurrentReport.CppMigrationMetrics.MigrationDifficulty))
		]
	);
}

TSharedRef<SWidget> SBPComplexityPanel::BuildIssuesList()
{
	TSharedRef<SVerticalBox> IssuesContainer = SNew(SVerticalBox);

	if (CurrentReport.Issues.Num() == 0)
	{
		IssuesContainer->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoIssues", "問題は検出されませんでした"))
			.ColorAndOpacity(FLinearColor::Green)
		];
	}
	else
	{
		for (const FBPIssue& Issue : CurrentReport.Issues)
		{
			IssuesContainer->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 2.0f)
			[
				CreateIssueRow(Issue)
			];
		}
	}

	return CreateExpandableSection(
		FText::Format(LOCTEXT("IssuesFmt", "検出された問題 ({0})"), FText::AsNumber(CurrentReport.Issues.Num())),
		IssuesContainer
	);
}

TSharedRef<SWidget> SBPComplexityPanel::BuildProjectSummary()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			CreateKeyValueRow(TEXT("総Blueprint数"), FString::FromInt(ProjectSummary.TotalBlueprintCount))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			CreateKeyValueRow(TEXT("Green"), FString::FromInt(ProjectSummary.GreenCount), FLinearColor::Green)
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			CreateKeyValueRow(TEXT("Yellow"), FString::FromInt(ProjectSummary.YellowCount), FLinearColor::Yellow)
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			CreateKeyValueRow(TEXT("Red"), FString::FromInt(ProjectSummary.RedCount), FLinearColor::Red)
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			CreateKeyValueRow(TEXT("平均スコア"), FString::Printf(TEXT("%.1f"), ProjectSummary.AverageComplexityScore))
		];
}

TSharedRef<SWidget> SBPComplexityPanel::CreateExpandableSection(const FText& Title, TSharedRef<SWidget> Content, bool bInitiallyExpanded)
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

TSharedRef<SWidget> SBPComplexityPanel::CreateKeyValueRow(const FString& Key, const FString& Value, FLinearColor ValueColor)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(0.4f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Key))
			.ColorAndOpacity(FLinearColor::Gray)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(0.6f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Value))
			.ColorAndOpacity(ValueColor)
		];
}

TSharedRef<SWidget> SBPComplexityPanel::CreateIssueRow(const FBPIssue& Issue)
{
	FLinearColor SeverityColor = UBPComplexityAnalyzer::GetHealthLevelColor(Issue.Severity);

	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.BorderBackgroundColor(SeverityColor * 0.2f)
		.Padding(4.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					CreateStatusBadge(Issue.Category, Issue.Severity)
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(8.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Issue.Description))
					.AutoWrapText(true)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("→ ") + Issue.SuggestedFix))
				.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f))
				.AutoWrapText(true)
				.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
			]
		];
}

TSharedRef<SWidget> SBPComplexityPanel::CreateStatusBadge(const FString& Text, EBPHealthLevel Level)
{
	FLinearColor Color = UBPComplexityAnalyzer::GetHealthLevelColor(Level);

	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
		.Padding(FMargin(6.0f, 2.0f))
		.BorderBackgroundColor(Color * 0.3f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Text))
			.ColorAndOpacity(Color)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
		];
}

void SBPComplexityPanel::AnalyzeBlueprint(UBlueprint* Blueprint)
{
	if (!Blueprint || !Analyzer)
	{
		return;
	}

	bProjectMode = false;
	CurrentBlueprintPath = Blueprint->GetPathName();
	CurrentReport = Analyzer->AnalyzeBlueprint(Blueprint);

	RefreshUI();
}

void SBPComplexityPanel::AnalyzeProject(const FString& PathFilter)
{
	if (!Analyzer)
	{
		return;
	}

	bProjectMode = true;
	ProjectSummary = Analyzer->AnalyzeProject(PathFilter);

	// 最も複雑なBPのレポートを表示
	if (ProjectSummary.MostComplexBlueprints.Num() > 0)
	{
		CurrentReport = ProjectSummary.MostComplexBlueprints[0];
	}

	RefreshUI();
}

FReply SBPComplexityPanel::OnAnalyzeSelectedClicked()
{
	// コンテンツブラウザで選択中のアセットを取得
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

	for (const FAssetData& AssetData : SelectedAssets)
	{
		UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
		if (Blueprint)
		{
			AnalyzeBlueprint(Blueprint);
			break;
		}
	}

	return FReply::Handled();
}

FReply SBPComplexityPanel::OnAnalyzeProjectClicked()
{
	AnalyzeProject();
	return FReply::Handled();
}

FReply SBPComplexityPanel::OnOpenSettingsClicked()
{
	// TODO: 設定ウィンドウを開く
	return FReply::Handled();
}

FReply SBPComplexityPanel::OnExportReportClicked()
{
	// ファイル保存ダイアログ
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return FReply::Handled();
	}

	TArray<FString> SaveFilenames;
	bool bOpened = DesktopPlatform->SaveFileDialog(
		nullptr,
		TEXT("Export Analysis Report"),
		FPaths::ProjectDir(),
		TEXT("BPAnalysisReport.csv"),
		TEXT("CSV Files (*.csv)|*.csv"),
		EFileDialogFlags::None,
		SaveFilenames
	);

	if (bOpened && SaveFilenames.Num() > 0)
	{
		FString CSVContent;

		// ヘッダー
		CSVContent += TEXT("Blueprint,Overall Score,Health Level,Node Count,Tick Usage,Circular Refs,Cpp Score\n");

		if (bProjectMode)
		{
			for (const FBPAnalysisReport& Report : ProjectSummary.MostComplexBlueprints)
			{
				CSVContent += FString::Printf(TEXT("%s,%.1f,%s,%d,%s,%d,%.1f\n"),
					*Report.BlueprintName,
					Report.OverallComplexityScore,
					*UBPComplexityAnalyzer::GetHealthLevelString(Report.OverallHealthLevel),
					Report.NodeMetrics.TotalNodeCount,
					Report.TickMetrics.bUsesTick ? TEXT("Yes") : TEXT("No"),
					Report.DependencyMetrics.CircularReferenceCount,
					Report.CppMigrationMetrics.MigrationScore
				);
			}
		}
		else
		{
			CSVContent += FString::Printf(TEXT("%s,%.1f,%s,%d,%s,%d,%.1f\n"),
				*CurrentReport.BlueprintName,
				CurrentReport.OverallComplexityScore,
				*UBPComplexityAnalyzer::GetHealthLevelString(CurrentReport.OverallHealthLevel),
				CurrentReport.NodeMetrics.TotalNodeCount,
				CurrentReport.TickMetrics.bUsesTick ? TEXT("Yes") : TEXT("No"),
				CurrentReport.DependencyMetrics.CircularReferenceCount,
				CurrentReport.CppMigrationMetrics.MigrationScore
			);
		}

		FFileHelper::SaveStringToFile(CSVContent, *SaveFilenames[0]);
	}

	return FReply::Handled();
}

void SBPComplexityPanel::RefreshUI()
{
	if (!DetailContainer.IsValid())
	{
		return;
	}

	// 詳細パネルをクリアして再構築
	DetailContainer->ClearChildren();

	// サマリー更新
	if (SummaryText.IsValid())
	{
		SummaryText->SetText(FText::FromString(CurrentReport.HumanReadableSummary));
	}

	// 信号機更新
	if (TrafficLightContainer.IsValid())
	{
		TrafficLightContainer->ClearChildren();
		TrafficLightContainer->AddSlot()
		.AutoWidth()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(2.0f)
			[
				CreateTrafficLightCircle(EBPHealthLevel::Red, CurrentReport.OverallHealthLevel == EBPHealthLevel::Red)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(2.0f)
			[
				CreateTrafficLightCircle(EBPHealthLevel::Yellow, CurrentReport.OverallHealthLevel == EBPHealthLevel::Yellow)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(2.0f)
			[
				CreateTrafficLightCircle(EBPHealthLevel::Green, CurrentReport.OverallHealthLevel == EBPHealthLevel::Green)
			]
		];
	}

	// プロジェクトモードの場合はサマリーを追加
	if (bProjectMode)
	{
		DetailContainer->AddSlot()
		.AutoHeight()
		.Padding(4.0f)
		[
			CreateExpandableSection(LOCTEXT("ProjectSummary", "プロジェクトサマリー"), BuildProjectSummary())
		];
	}

	// 各セクションを追加
	DetailContainer->AddSlot().AutoHeight().Padding(4.0f)[BuildNodeMetricsSection()];
	DetailContainer->AddSlot().AutoHeight().Padding(4.0f)[BuildDependencySection()];
	DetailContainer->AddSlot().AutoHeight().Padding(4.0f)[BuildTickSection()];
	DetailContainer->AddSlot().AutoHeight().Padding(4.0f)[BuildCppMigrationSection()];
	DetailContainer->AddSlot().AutoHeight().Padding(4.0f)[BuildIssuesList()];

	// 推奨アクション
	if (CurrentReport.RecommendedActions.Num() > 0)
	{
		TSharedRef<SVerticalBox> ActionsBox = SNew(SVerticalBox);
		for (const FString& Action : CurrentReport.RecommendedActions)
		{
			ActionsBox->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 2.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("• ") + Action))
				.AutoWrapText(true)
			];
		}

		DetailContainer->AddSlot()
		.AutoHeight()
		.Padding(4.0f)
		[
			CreateExpandableSection(LOCTEXT("RecommendedActions", "推奨アクション"), ActionsBox)
		];
	}
}

#undef LOCTEXT_NAMESPACE
