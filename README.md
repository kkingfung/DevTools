# DevTools - ゲーム開発支援ツール集
*複数プロジェクトで再利用可能な開発支援ツールのライブラリ*

その一つとして

<img src="/Result1.PNG" width="300"><img src="/Result2.PNG" width="300">

## 概要

このリポジトリは、ゲーム開発を支援する **再利用可能なツール** を管理しています。
Unity用エディタ拡張からスタンドアロンのデスクトップアプリまで、様々なツールを収録しています。

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

### スタンドアロンアプリ

| ツール名 | 説明 | 使い方 |
|---------|------|--------|
| **GameDevScheduler** | ゲーム開発チーム向けスケジュール管理ツール。ガントチャート、カレンダー、テーブルビューでタスク管理。Tauri 2.0ベースのデスクトップアプリ | `Tool/GameDevScheduler` ディレクトリで `npm install && npm run tauri dev` を実行 |
| **TextToVideo** | AIテキスト→ビデオ生成ツール。Replicate/Fal.ai/ComfyUI対応。5秒〜90秒のビデオ生成、BGM/効果音追加、AI音楽生成機能付き。Gradio Web UI | `Tool/TextToVideo` ディレクトリで `pip install -r requirements.txt && python run.py` → ブラウザで http://127.0.0.1:7860 を開く |

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
│   ├── GameDevScheduler/           # Tauri 2.0 デスクトップアプリ
│   │   ├── src/                    # React フロントエンド
│   │   ├── src-tauri/              # Rust バックエンド
│   │   └── README.md               # 詳細ドキュメント
│   └── TextToVideo/                # AI ビデオ生成ツール
│       ├── src/                    # Python ソースコード
│       ├── workflows/              # ComfyUI ワークフロー
│       └── README.md               # 詳細ドキュメント
```

---

## 使い方

### Unity ツールの導入

1. 必要なツールのフォルダを Unity プロジェクトの `Assets/` 配下にコピー
2. Runtime フォルダがある場合は、必要なコンポーネントをシーンに配置
3. メニューからツールウィンドウを開いて使用

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

---

## ツールの追加方法

1. `Tool/` フォルダ内に新しいツール用ディレクトリを作成
2. ツールのスクリプト類を配置
3. この README の一覧表に追記

---

## ライセンス

MIT License
