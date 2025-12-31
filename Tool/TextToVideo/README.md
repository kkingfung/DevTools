# Text to Video Generator

AIを使用してテキストプロンプトからビデオを生成するツールです。複数のクラウドAPI（Replicate、Fal.ai）とローカル推論（ComfyUI）をサポートし、オーディオの追加やAI音楽生成も可能です。

## 特徴

- **マルチプロバイダー対応**: クラウド（Replicate、Fal.ai）とローカル（ComfyUI）を切り替え可能
- **長時間ビデオ生成**: 5秒〜90秒のビデオを自動クリップチェイニングで生成
- **画像参照**: 参照画像からビデオを生成（Image-to-Video）
- **オーディオ機能**: BGM、ナレーション、効果音の追加
- **AI音楽生成**: プロンプトからBGMを自動生成（MusicGen）
- **Web UI**: Gradioベースの使いやすいインターフェース

## 必要環境

- Python 3.10+
- FFmpeg（システムにインストール済み）
- GPU（ローカル推論を使用する場合）

## インストール

### 1. 依存パッケージのインストール

```bash
cd TextToVideo
pip install -r requirements.txt
```

### 2. FFmpegのインストール

**Windows:**
```bash
# Chocolateyを使用
choco install ffmpeg

# または Scoopを使用
scoop install ffmpeg
```

**macOS:**
```bash
brew install ffmpeg
```

**Linux:**
```bash
sudo apt install ffmpeg
```

### 3. APIキーの設定（3つの方法）

ビデオ生成には、以下のいずれかのプロバイダーが必要です。

#### 方法A: 環境変数で設定（一時的）

```bash
# Windows (PowerShell)
$env:REPLICATE_API_TOKEN = "r8_xxxxxxxxxxxxxxxx"
python run.py

# Windows (Command Prompt)
set REPLICATE_API_TOKEN=r8_xxxxxxxxxxxxxxxx
python run.py

# macOS / Linux
export REPLICATE_API_TOKEN="r8_xxxxxxxxxxxxxxxx"
python run.py
```

#### 方法B: 設定ファイルで設定（永続的・推奨）

1. `config.example.yaml` をコピーして `config.yaml` を作成:

```bash
copy config.example.yaml config.yaml
```

2. `config.yaml` をテキストエディタで開き、APIキーを設定:

```yaml
providers:
  replicate:
    enabled: true
    api_key: "r8_xxxxxxxxxxxxxxxx"  # ← ここにAPIキーを入力
    model: "minimax"
```

3. 保存して起動:

```bash
python run.py
```

#### 方法C: ComfyUI（ローカル・無料）

1. ComfyUIをインストール: https://github.com/comfyanonymous/ComfyUI
2. ComfyUIを起動（デフォルトで http://127.0.0.1:8188 で起動）
3. 本ツールを起動すると自動的にComfyUIを検出

### APIキーの取得方法

| プロバイダー | 取得URL | 備考 |
|-------------|---------|------|
| Replicate | https://replicate.com/account/api-tokens | 新規登録で無料クレジット付与 |
| Fal.ai | https://fal.ai/dashboard/keys | 高速推論 |

## 使い方

### Web UIの起動

```bash
python run.py
```

起動後、ブラウザで以下のURLを開きます:

```
http://127.0.0.1:7860
```

（または `http://localhost:7860`）

### オプション

```bash
python run.py --port 8080      # ポート番号を変更
python run.py --share          # 公開リンクを生成（外部からアクセス可能）
python run.py --config my.yaml # 設定ファイルを指定
```

## Web UI タブ

### 🎬 ビデオ生成

テキストプロンプトからビデオを生成します。

1. **プロンプト**: 生成したいビデオの説明を入力
2. **ネガティブプロンプト**: 避けたい要素を入力（オプション）
3. **参照画像**: 最初のフレームとなる画像をアップロード（オプション）
4. **プロバイダー**: 使用するAIサービスを選択
5. **長さ**: 5秒〜90秒で指定
6. **詳細設定**: 解像度、シード、トランジションを設定

### 🔊 オーディオ追加

生成したビデオにオーディオを追加します。

- **BGM**: バックグラウンドミュージック（ループ、フェードアウト対応）
- **ナレーション**: ボイスオーバー音声
- **効果音**: 指定した秒数で再生

### 🎵 AI音楽生成

プロンプトからAI音楽を生成します（MusicGen使用）。

```
例: upbeat electronic music with synth leads, energetic and modern
例: calm piano melody, peaceful and relaxing ambient
例: epic orchestral soundtrack, cinematic and dramatic
```

## 対応プロバイダー

### クラウド

| プロバイダー | モデル | 最大長 | 画像参照 |
|-------------|--------|--------|----------|
| Replicate | minimax | 6秒 | ✅ |
| Replicate | luma | 5秒 | ✅ |
| Replicate | stable-video | 4秒 | ✅（必須） |
| Fal.ai | minimax | 6秒 | ✅ |
| Fal.ai | kling | 5秒 | ❌ |
| Fal.ai | luma | 5秒 | ✅ |
| Fal.ai | hunyuan | 5秒 | ❌ |

### ローカル (ComfyUI)

ComfyUIを起動した状態で使用可能。付属のワークフローテンプレートを使用:

- `workflows/animatediff_txt2vid.json` - Text-to-Video
- `workflows/animatediff_img2vid.json` - Image-to-Video
- `workflows/svd_img2vid.json` - Stable Video Diffusion

## Pythonからの使用

```python
from src import (
    ReplicateProvider,
    GenerationManager,
    GenerationRequest,
    AudioProcessor,
    AudioTrack,
    AudioConfig,
)

# プロバイダーを初期化
provider = ReplicateProvider(
    api_key="your_api_key",
    model_name="minimax"
)

# マネージャーを作成
manager = GenerationManager(provider=provider)

# ビデオを生成
request = GenerationRequest(
    prompt="A beautiful sunset over the ocean, cinematic",
    duration=10,  # 10秒
    width=1280,
    height=720
)

result = manager.generate(request)
print(f"Generated: {result.video_path}")

# オーディオを追加
audio_processor = AudioProcessor()
config = AudioConfig(
    background_music=AudioTrack(
        path="bgm.mp3",
        volume=0.3,
        loop=True,
        fade_out=2.0
    )
)

final_video = audio_processor.add_audio_to_video(
    video_path=result.video_path,
    output_path="output_with_audio.mp4",
    config=config
)
```

## 設定ファイル

`config.yaml` で各種設定が可能:

```yaml
# 出力ディレクトリ
output_dir: "./outputs"

# デフォルトプロバイダー
default_provider: "replicate"

# FFmpegパス
ffmpeg_path: "ffmpeg"

# プロバイダー設定
providers:
  replicate:
    enabled: true
    api_key: ""  # 環境変数 REPLICATE_API_TOKEN でも可
    model: "minimax"

  fal:
    enabled: true
    api_key: ""  # 環境変数 FAL_KEY でも可
    model: "minimax"

  comfyui:
    enabled: true
    url: "http://127.0.0.1:8188"
    workflow_path: ""  # カスタムワークフローのパス

# 生成デフォルト値
defaults:
  width: 1280
  height: 720
  fps: 24
```

## プロジェクト構造

```
TextToVideo/
├── src/
│   ├── providers/          # ビデオ生成プロバイダー
│   │   ├── base.py         # 抽象インターフェース
│   │   ├── replicate_provider.py
│   │   ├── fal_provider.py
│   │   └── comfyui_provider.py
│   ├── core/               # コア機能
│   │   ├── manager.py      # 生成マネージャー
│   │   ├── stitcher.py     # ビデオ結合
│   │   └── audio.py        # オーディオ処理
│   ├── ui/
│   │   └── gradio_app.py   # Web UI
│   └── config.py           # 設定管理
├── workflows/              # ComfyUIワークフロー
│   ├── animatediff_txt2vid.json
│   ├── animatediff_img2vid.json
│   └── svd_img2vid.json
├── outputs/                # 生成ファイル出力先
├── config.example.yaml     # 設定ファイルテンプレート
├── requirements.txt
├── run.py                  # 起動スクリプト
└── README.md
```

## トラブルシューティング

### FFmpegが見つからない

```
RuntimeError: FFmpeg not found
```

→ FFmpegをインストールし、PATHに追加してください。または `config.yaml` で `ffmpeg_path` を絶対パスで指定。

### プロバイダーが利用できない

```
利用可能なプロバイダーがありません
```

→ APIキーが正しく設定されているか確認。環境変数または `config.yaml` で設定。

### ComfyUIに接続できない

```
ComfyUI server not available
```

→ ComfyUIが起動しているか確認。デフォルトは `http://127.0.0.1:8188`。

### 長いビデオの生成に時間がかかる

90秒のビデオは約15クリップを生成・結合するため、数分〜十数分かかります。進捗バーで状況を確認できます。

## ライセンス

MIT License

## 謝辞

- [Replicate](https://replicate.com/) - クラウドAIプラットフォーム
- [Fal.ai](https://fal.ai/) - 高速AI推論
- [ComfyUI](https://github.com/comfyanonymous/ComfyUI) - ローカル推論
- [Gradio](https://gradio.app/) - Web UIフレームワーク
- [MusicGen](https://github.com/facebookresearch/audiocraft) - AI音楽生成
