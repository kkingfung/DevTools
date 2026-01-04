# Blueprint Complexity Analyzer

🧩 **BPが死にかけているかを数値で警告**

![UE5](https://img.shields.io/badge/UE5-5.0%2B-blue)
![License](https://img.shields.io/badge/license-MIT-green)

## 概要

UE5はBlueprintが強力すぎて、気づいたら破綻している…そんな経験はありませんか？

このプラグインは、Blueprintの健全性を**信号機表示（Green / Yellow / Red）**で可視化し、問題になる前に警告します。

### なぜ必要か

| 問題 | 結果 |
|------|------|
| BPの可視化がない | 事故は確定演出 |
| ノード数の増加に気づかない | パフォーマンス低下 |
| 循環参照が見えない | ビルド/ロード時間増大 |
| Tick乱用 | フレームレート崩壊 |

**BP破綻は全UE5ユーザーが一度は踏む地雷です。**

## 機能

### 🚦 信号機表示

| レベル | 意味 | 対応 |
|--------|------|------|
| 🟢 **Green** | 健全 | そのまま維持 |
| 🟡 **Yellow** | 警告 | 注意が必要 |
| 🔴 **Red** | 危険 | 即座に対応 |

### 📊 分析メトリクス

1. **ノード数分析**
   - 総ノード数
   - 関数呼び出し数
   - 変数アクセス数
   - 最大グラフサイズ

2. **依存深度分析**
   - 直接依存数
   - 推移的依存数
   - 依存チェーン深度
   - **循環参照検出**

3. **Tick使用分析**
   - Tickイベント使用有無
   - Tick内ノード数
   - 重い処理の警告

4. **C++化推奨度**
   - 移行推奨スコア（0-100%）
   - 移行難易度（1-5）
   - 推奨理由リスト

## インストール

1. `BlueprintComplexityAnalyzer` フォルダを `YourProject/Plugins/` にコピー
2. プロジェクトを再起動
3. **Window** → **BP Complexity Analyzer** でパネルを開く

## 使い方

### 単一Blueprint分析

1. コンテンツブラウザでBlueprintを選択
2. パネルで「**Analyze Selected**」をクリック
3. 信号機表示と詳細スコアを確認

### プロジェクト全体分析

1. パネルで「**Analyze Project**」をクリック
2. 全Blueprintがスキャンされる
3. 最も複雑なTop10が表示される

### コンテンツブラウザから

1. Blueprintを右クリック
2. 「**Analyze BP Complexity**」を選択

## UI パネル

```
┌──────────────────────────────────────────────────────────┐
│ [Analyze Selected] [Analyze Project]      [Export Report] │
├──────────────────────────────────────────────────────────┤
│                                                          │
│  🔴  ┌─────────────────────────────────────────────────┐ │
│  🟡  │ 複雑度スコア                                      │ │
│  🟢  │ ████████████████░░░░░░░░░░░░░░  72 / 100        │ │
│      └─────────────────────────────────────────────────┘ │
│                                                          │
│ [BP_PlayerCharacter] | 危険 (Red) | ノード: 342 | Tick使用 │
│                                                          │
│ ▼ ノード数分析                                            │
│   総ノード数:      342                                    │
│   関数呼び出し:    89                                     │
│   変数アクセス:    156                                    │
│   最大グラフ:      EventGraph (187 nodes)                 │
│                                                          │
│ ▼ 依存関係分析                                            │
│   直接依存数:      15                                     │
│   依存深度:        7                                      │
│   循環参照:        2 ⚠️                                   │
│                                                          │
│ ▼ Tick使用分析                                            │
│   Tick:            使用中 (45 nodes) ⚠️                   │
│   Tickイベント数:   2                                     │
│                                                          │
│ ▼ C++化推奨度                                             │
│   推奨度スコア:    78%                                    │
│   移行難易度:      4 / 5                                  │
│                                                          │
│ ▼ 検出された問題 (4)                                       │
│   ┌────────────────────────────────────────────────────┐ │
│   │ [ノード数] 総ノード数が300を超えています (342)        │ │
│   │ → 機能を複数のBPに分割するか、C++への移行を検討       │ │
│   └────────────────────────────────────────────────────┘ │
│                                                          │
│ ▼ 推奨アクション                                          │
│   • 【緊急】このBlueprintは即座にリファクタリングが必要    │
│   • Tick処理をタイマーまたはイベント駆動に置き換え         │
│   • 循環参照を解消するためにインターフェースを使用          │
│   • パフォーマンス向上のためC++への移行を検討              │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

## 閾値設定

デフォルトの閾値（カスタマイズ可能）:

| メトリクス | Yellow | Red |
|-----------|--------|-----|
| 総ノード数 | 100 | 300 |
| 単一グラフノード数 | 50 | 100 |
| 直接依存数 | 10 | 20 |
| 依存深度 | 5 | 10 |
| Tick内ノード数 | 10 | 30 |

## レポートエクスポート

「**Export Report**」ボタンでCSV形式でエクスポート:

```csv
Blueprint,Overall Score,Health Level,Node Count,Tick Usage,Circular Refs,Cpp Score
BP_Player,72.5,Red,342,Yes,2,78.0
BP_Enemy,45.2,Yellow,156,No,0,35.0
BP_Item,12.0,Green,45,No,0,10.0
```

## ファイル構成

```
BlueprintComplexityAnalyzer/
├── BlueprintComplexityAnalyzer.uplugin
├── README.md
└── Source/
    └── BlueprintComplexityAnalyzer/
        ├── BlueprintComplexityAnalyzer.Build.cs
        ├── Public/
        │   ├── BlueprintComplexityAnalyzerModule.h
        │   ├── BPComplexityTypes.h
        │   ├── BPComplexityAnalyzer.h
        │   └── SBPComplexityPanel.h
        └── Private/
            ├── BlueprintComplexityAnalyzerModule.cpp
            ├── BPComplexityAnalyzer.cpp
            └── SBPComplexityPanel.cpp
```

## API リファレンス

### UBPComplexityAnalyzer

```cpp
// Blueprintを分析
FBPAnalysisReport AnalyzeBlueprint(UBlueprint* Blueprint);

// プロジェクト全体を分析
FBPProjectAnalysisSummary AnalyzeProject(const FString& PathFilter = "");

// 個別メトリクス取得
FBPNodeMetrics AnalyzeNodeCount(UBlueprint* Blueprint);
FBPDependencyMetrics AnalyzeDependencies(UBlueprint* Blueprint);
FBPTickMetrics AnalyzeTickUsage(UBlueprint* Blueprint);

// 閾値設定
void SetThresholds(const FBPComplexityThresholds& NewThresholds);
```

### FBPAnalysisReport

```cpp
USTRUCT()
struct FBPAnalysisReport
{
    FString BlueprintName;
    float OverallComplexityScore;      // 0-100
    EBPHealthLevel OverallHealthLevel; // Green/Yellow/Red

    FBPNodeMetrics NodeMetrics;
    FBPDependencyMetrics DependencyMetrics;
    FBPTickMetrics TickMetrics;
    FBPCppMigrationMetrics CppMigrationMetrics;

    TArray<FBPIssue> Issues;
    TArray<FString> RecommendedActions;
    FString HumanReadableSummary;
};
```

## ベストプラクティス

### Green を維持するために

1. **関数に分割**: 大きなイベントグラフは関数に分割
2. **マクロを活用**: 繰り返し処理はマクロ化
3. **Tickを避ける**: タイマーまたはイベント駆動を使用
4. **依存を減らす**: インターフェースでデカップリング
5. **定期的にチェック**: 開発中に定期的に分析

### Red からの回復

1. 機能を複数のBPに分割
2. 共通処理をC++に移行
3. 循環参照をインターフェースで解消
4. Tick処理を最適化または排除

## トラブルシューティング

### 分析が遅い

- プロジェクト全体分析は大規模プロジェクトでは時間がかかります
- パスフィルタを使用して範囲を絞ってください

### 循環参照が検出されない

- アセットレジストリが完全に更新されていない可能性があります
- エディタを再起動してください

## ライセンス

MIT License

## 作者

DevTools Project

---

**BPが破綻する前に、信号機で警告。**
