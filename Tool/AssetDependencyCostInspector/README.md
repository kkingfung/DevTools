# Asset Dependency & Cost Inspector

**「このアセット、実際いくら払ってる？」**

UE5時代のアセットコスト感覚を取り戻すためのエディタプラグイン。依存チェーン、メモリコスト、Streaming影響、読み込みタイミングを一画面で可視化します。

## 課題

UE5でNanite/Lumen移行後、従来のアセットコスト感覚が通用しなくなりました：

- **見えないコスト**: Naniteメッシュの真のメモリコストが不明
- **依存地獄**: 1つのアセットが芋づる式に大量の依存を引っ張る
- **Streaming盲点**: 常駐サイズ vs ストリーミングサイズの把握困難
- **読み込み遅延**: どのアセットがロード時間に影響しているか不明

## 機能

### 依存チェーン可視化
- 再帰的な依存関係ツリー表示
- 深度別のコスト累積
- 循環参照の自動検出
- サブツリー単位のコスト集計

### メモリコスト分析
| メトリクス | 説明 |
|-----------|------|
| ディスクサイズ | .uasset ファイルサイズ |
| メモリサイズ | ランタイムメモリ使用量 |
| GPUメモリ | VRAM使用量 |
| Naniteデータ | Nanite固有のデータサイズ |
| 依存含む合計 | 全依存を含むトータルコスト |

### Streaming情報
- Streamable判定
- 常駐サイズ vs ストリームサイズ
- Mipレベル情報
- Streaming優先度

### 読み込みタイミング
- 読み込みフェーズ（Startup/GameLoad/AsyncLoad/OnDemand）
- 推定読み込み時間
- ブロッキングロード判定
- 依存読み込み順序

### UE5固有コスト
- **Nanite**: 有効/無効、三角形数、フォールバックサイズ
- **Lumen**: 対応状況
- **VSM (Virtual Shadow Maps)**: 対応状況
- **World Partition**: セル情報

### 問題検出
自動で以下の問題を検出：
- 巨大アセット（閾値超過）
- 深すぎる依存チェーン
- 循環参照
- 過大な依存数
- 非効率なStreaming設定
- Naniteフォールバック警告

### 最適化推奨
検出された問題に基づき、具体的な最適化提案を生成：
- テクスチャ解像度削減
- LOD設定見直し
- Nanite有効化推奨
- 依存関係の分離
- Streaming設定の最適化

## 使い方

### 1. ウィンドウから開く
`Window > Asset Cost Inspector`

### 2. コンテンツブラウザから
- **アセット右クリック** → 「コストを分析」
- **フォルダ右クリック** → 「フォルダのコストを分析」

### 3. パス入力
ウィンドウ上部のテキストボックスにアセットパスを入力：
```
/Game/Characters/Hero/SK_Hero
```

## UI構成

```
┌─────────────────────────────────────────────────────────────┐
│ [アセットパス入力] [☐フォルダ分析] [分析] [選択を分析] [Export] │
├─────────────────────┬───────────────────────────────────────┤
│                     │ ▼ 概要                                │
│   依存ツリー        │   アセット: SK_Hero                   │
│                     │   種別: SkeletalMesh                  │
│   📁 SK_Hero 45MB   │   コストレベル: High                  │
│   ├─ M_Skin 12MB    │   依存数: 23 (直接: 5)                │
│   │  └─ T_Skin 8MB  ├───────────────────────────────────────┤
│   ├─ M_Eyes 3MB     │ ▼ コスト内訳                          │
│   └─ SK_Rig 2MB     │   メモリコスト:                       │
│                     │     ディスク: 45.2 MB                 │
│                     │     メモリ: 128.5 MB                  │
│                     │     GPU: 64.0 MB                      │
│                     ├───────────────────────────────────────┤
│                     │ ▼ 検出された問題                      │
│                     │   🔴 巨大テクスチャ: T_Skin (8192x8192)│
│                     │   🟡 深い依存チェーン (深度: 8)        │
│                     ├───────────────────────────────────────┤
│                     │ ▼ 最適化推奨                          │
│                     │   💡 T_Skinを4096x4096に削減検討      │
│                     │   💡 マテリアルインスタンス化を検討    │
└─────────────────────┴───────────────────────────────────────┘
```

## コストレベル

| レベル | 色 | メモリ閾値 |
|--------|-----|----------|
| Low | 🟢 緑 | < 10 MB |
| Medium | 🟡 黄 | 10-50 MB |
| High | 🟠 オレンジ | 50-100 MB |
| Critical | 🔴 赤 | > 100 MB |

## エクスポート

分析結果を以下の形式でエクスポート可能：
- **テキスト形式**: 人間可読なサマリーレポート
- **CSV形式**: スプレッドシート分析用

## 閾値設定

`UAssetCostAnalyzer::SetThresholds()` でカスタム閾値を設定可能：

```cpp
FAssetCostThresholds Thresholds;
Thresholds.LowMemoryThreshold = 5 * 1024 * 1024;      // 5MB
Thresholds.MediumMemoryThreshold = 25 * 1024 * 1024;  // 25MB
Thresholds.HighMemoryThreshold = 75 * 1024 * 1024;    // 75MB
Thresholds.MaxDependencyDepth = 8;
Thresholds.MaxDependencyCount = 50;
Analyzer->SetThresholds(Thresholds);
```

## Blueprint API

```cpp
// アセット分析
UFUNCTION(BlueprintCallable)
FAssetCostReport AnalyzeAsset(const FString& AssetPath);

// フォルダ分析
UFUNCTION(BlueprintCallable)
FProjectCostSummary AnalyzeFolder(const FString& FolderPath);

// メモリコスト計算
UFUNCTION(BlueprintCallable)
FAssetMemoryCost CalculateMemoryCost(const FString& AssetPath);

// 依存ツリー構築
UFUNCTION(BlueprintCallable)
FAssetDependencyNode BuildDependencyTree(const FString& AssetPath, int32 MaxDepth = 10);
```

## 対応アセットタイプ

- StaticMesh（Nanite対応含む）
- SkeletalMesh
- Texture（2D/Cube/Volume）
- Material / MaterialInstance
- Sound（Wave/Cue）
- Blueprint
- AnimSequence / AnimMontage
- ParticleSystem / NiagaraSystem
- その他汎用UObject

## 技術詳細

### 依存関係収集
`AssetRegistry`を使用して再帰的に依存関係を収集。循環参照を検出するため、訪問済みセットを管理。

### メモリサイズ推定
- **StaticMesh**: `GetResourceSizeBytes()` + LOD情報
- **SkeletalMesh**: `GetResourceSizeBytes()` + ボーン数考慮
- **Texture**: `CalcTextureMemorySizeEnum()` + Mip情報
- **Material**: シェーダーコンパイル状態を考慮
- **Sound**: 非圧縮/圧縮サイズ

### Nanite情報
`FStaticMeshRenderData`から以下を取得：
- `bHasNaniteData`: Nanite有効判定
- Nanite三角形数
- フォールバックメッシュサイズ

## 動作要件

- Unreal Engine 5.0+
- エディタ専用プラグイン

## インストール

1. `AssetDependencyCostInspector`フォルダをプロジェクトの`Plugins`ディレクトリにコピー
2. プロジェクトを再起動
3. `Window > Asset Cost Inspector`でツールを開く

## ライセンス

MIT License
