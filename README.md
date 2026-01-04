# DevTools - ゲーム開発支援ツール集
*複数プロジェクトで再利用可能な開発支援ツールのライブラリ*

<img src="/Result1.PNG" width="300"><img src="/Result2.PNG" width="300"><img src="/Result3.PNG" width="300">

## 概要

このリポジトリは、ゲーム開発を支援する **再利用可能なツール** を管理しています。
Unity用エディタ拡張、UE5プラグイン、スタンドアロンのデスクトップアプリまで、様々なツールを収録しています。

必要に応じて `Tool/` フォルダから他プロジェクトにコピーして利用できます。

---

## ツール一覧

### Unity エディタ拡張

| ツール名 | 説明 | 使い方 |
|---------|------|--------|
| **UnusedAssetDetector** | 未使用アセット検出ツール。プロジェクト内で参照されていないアセットを検出し、一覧表示・削除が可能 | `Editor` フォルダをプロジェクトにコピー → メニュー `Tools > Unused Asset Detector` |
| **BuildSizeAnalyzer** | ビルドサイズ分析ツール。ビルド完了時に自動でサイズレポートをJSONに保存。アセット別・カテゴリ別のサイズを記録し、ビルド間の比較が可能 | `Editor` フォルダをプロジェクトにコピー → 自動でビルド時にレポート生成。設定は `Project Settings > Build Size Analyzer` |
| **RuntimeAssetTracker** | ランタイムアセット追跡ツール。シーン遷移時にTexture/AudioClipのスナップショットを取得し、残存アセット（メモリリーク候補）を検出 | `Runtime` と `Editor` フォルダをプロジェクトにコピー → シーンに `RuntimeAssetTrackerManager` を配置 → メニュー `Tools > Runtime Asset Tracker` |
| **InputLatencyAnalyzer** | 入力遅延計測ツール。入力から画面変化/アニメーション/オーディオまでの遅延をms・フレーム単位で計測。統計情報とセッション比較機能付き | `Runtime` と `Editor` フォルダをプロジェクトにコピー → シーンに `InputLatencyMeasurer` を配置 → F9で計測開始 → メニュー `Tools > Input Latency Analyzer` |
| **LogicPresentationSync** | ロジック/プレゼンテーション同期分析ツール。ゲームロジック（ヒット判定等）とプレゼンテーション（VFX、SE、アニメ）のタイミングずれを検出 | `Runtime` と `Editor` フォルダをプロジェクトにコピー → コードに `SyncEventMarker.MarkLogic/MarkVFX` 等を追加 → メニュー `Tools > Logic Presentation Sync` |

### UE5 プラグイン

| ツール名 | 説明 | 使い方 |
|---------|------|--------|
| **UE5UnifiedDebugPanel** | UE5の内部を"人間の言葉"に翻訳する統合デバッグパネル。Gameplay State、Ability (GAS)、Animation、AI、Tick情報を1画面で表示 | `Plugins` フォルダにコピー → `Window > Unified Debug Panel` → Watch Player で監視開始 |
| **BlueprintComplexityAnalyzer** | BPが死にかけているかを数値で警告。ノード数、依存深度、Tick使用率、循環参照、C++化推奨度を信号機表示で可視化 | `Plugins` フォルダにコピー → `Window > BP Complexity Analyzer` → Analyze Selected |
| **AssetDependencyCostInspector** | 「このアセット、実際いくら払ってる？」依存チェーン、メモリコスト、Streaming影響、読み込みタイミングを可視化。Nanite/Lumen時代のコスト感覚を取り戻す | `Plugins` フォルダにコピー → `Window > Asset Cost Inspector` または アセット右クリック → 「コストを分析」 |

### スタンドアロンアプリ

| ツール名 | 説明 | 使い方 |
|---------|------|--------|
| **GameDevScheduler** | ゲーム開発チーム向けスケジュール管理ツール。ガントチャート、カレンダー、テーブルビューでタスク管理。Tauri 2.0ベースのデスクトップアプリ | `Tool/GameDevScheduler` ディレクトリで `npm install && npm run tauri dev` を実行 |
| **TextToVideo** | AIテキスト→ビデオ生成ツール。Replicate/Fal.ai/ComfyUI対応。5秒〜90秒のビデオ生成、BGM/効果音追加、AI音楽生成機能付き。Gradio Web UI | `Tool/TextToVideo` ディレクトリで `pip install -r requirements.txt && python run.py` → ブラウザで http://127.0.0.1:7860 を開く |
| **RhythmNoteGenerator** | リズムゲーム用ノート譜面自動生成ツール。音声解析でビート・オンセット検出、ピッチ検出でカリンバタブ生成。1〜6キー、5段階難易度対応。PySide6デスクトップアプリ | `Tool/RhythmNoteGenerator` ディレクトリで `pip install -r requirements.txt && python run.py` を実行 |

---

## ディレクトリ構造

```
DevTools/
├── README.md
├── Tool/
│   ├── UnusedAssetDetector/
│   │   └── Editor/                 # Unity Editor拡張
│   ├── BuildSizeAnalyzer/
│   │   └── Editor/                 # Unity Editor拡張
│   ├── RuntimeAssetTracker/
│   │   ├── Runtime/                # ランタイムコンポーネント
│   │   └── Editor/                 # Unity Editor拡張
│   ├── InputLatencyAnalyzer/
│   │   ├── Runtime/                # ランタイムコンポーネント
│   │   └── Editor/                 # Unity Editor拡張
│   ├── LogicPresentationSync/
│   │   ├── Runtime/                # ランタイムコンポーネント
│   │   └── Editor/                 # Unity Editor拡張
│   ├── UE5UnifiedDebugPanel/       # UE5 統合デバッグパネル
│   │   ├── Source/                 # C++ ソースコード
│   │   └── README.md               # 詳細ドキュメント
│   ├── BlueprintComplexityAnalyzer/ # UE5 BP複雑度アナライザー
│   │   ├── Source/                 # C++ ソースコード
│   │   └── README.md               # 詳細ドキュメント
│   ├── AssetDependencyCostInspector/ # UE5 アセットコスト分析ツール
│   │   ├── Source/                 # C++ ソースコード
│   │   └── README.md               # 詳細ドキュメント
│   ├── GameDevScheduler/           # Tauri 2.0 デスクトップアプリ
│   │   ├── src/                    # React フロントエンド
│   │   ├── src-tauri/              # Rust バックエンド
│   │   └── README.md               # 詳細ドキュメント
│   ├── TextToVideo/                # AI ビデオ生成ツール
│   │   ├── src/                    # Python ソースコード
│   │   ├── workflows/              # ComfyUI ワークフロー
│   │   └── README.md               # 詳細ドキュメント
│   └── RhythmNoteGenerator/        # リズムゲームノート生成ツール
│       ├── src/                    # Python ソースコード
│       ├── templates/              # エクスポートテンプレート
│       └── README.md               # 詳細ドキュメント
```

---

## 使い方

### Unity ツールの導入

1. 必要なツールのフォルダを Unity プロジェクトの `Assets/` 配下にコピー
2. Runtime フォルダがある場合は、必要なコンポーネントをシーンに配置
3. メニューからツールウィンドウを開いて使用

### UE5 プラグインの導入

1. 必要なプラグインフォルダを UE5 プロジェクトの `Plugins/` 配下にコピー
2. エディタを再起動
3. `Window` メニューからツールパネルを開く

#### UE5UnifiedDebugPanel

```
Window > Unified Debug Panel
```

- **Watch Player**: プレイヤーPawnを監視
- リアルタイムでAbility、Animation、AI状態を表示
- 人間向けサマリーで「今何をしているか」が即座に分かる

#### BlueprintComplexityAnalyzer

```
Window > BP Complexity Analyzer
```

- **Analyze Selected**: 選択中のBPを分析
- **Analyze Project**: プロジェクト全体を分析
- 信号機表示（Green/Yellow/Red）で健全性を可視化

#### AssetDependencyCostInspector

```
Window > Asset Cost Inspector
```

- **アセット右クリック** → 「コストを分析」で個別分析
- **フォルダ右クリック** → 「フォルダのコストを分析」で一括分析
- 依存チェーン、メモリコスト、Streaming影響、Nanite/Lumenコストを可視化
- 問題検出と最適化推奨を自動生成

### GameDevScheduler の起動

```bash
cd Tool/GameDevScheduler
npm install
npm run tauri dev
```

ビルドする場合:
```bash
npm run tauri build
```

### TextToVideo の起動

```bash
cd Tool/TextToVideo
pip install -r requirements.txt
python run.py
```

ブラウザで http://127.0.0.1:7860 を開く。

APIキーの設定（初回のみ）:
```bash
# config.yaml を作成してAPIキーを設定
copy config.example.yaml config.yaml
# config.yaml を編集してReplicate/Fal.aiのAPIキーを追加
```

### RhythmNoteGenerator の起動

```bash
cd Tool/RhythmNoteGenerator
pip install -r requirements.txt
python run.py
```

機能:
- 音声ファイルからリズムゲーム用ノート譜面を自動生成
- ピッチ検出でカリンバタブを生成
- 複数フォーマットでエクスポート（JSON, YAML, CSV等）

---

## ツールの追加方法

1. `Tool/` フォルダ内に新しいツール用ディレクトリを作成
2. ツールのスクリプト類を配置
3. この README の一覧表に追記

---

## ライセンス

MIT License
