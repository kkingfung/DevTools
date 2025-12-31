# ComfyUI Workflow Templates

このディレクトリには、ローカルビデオ生成用のComfyUIワークフローテンプレートが含まれています。

## 必要なモデル

### AnimateDiff用
- **Checkpoint**: `dreamshaper_8.safetensors` または任意のSD1.5互換モデル
- **Motion Model**: `mm_sd_v15_v2.ckpt`
  - ダウンロード: https://huggingface.co/guoyww/animatediff

### SVD (Stable Video Diffusion) 用
- **Checkpoint**: `svd_xt.safetensors`
  - ダウンロード: https://huggingface.co/stabilityai/stable-video-diffusion-img2vid-xt

## 必要なカスタムノード

ComfyUI Managerで以下をインストール:

1. **AnimateDiff Evolved**
   - `ADE_LoadAnimateDiffModel`
   - `ADE_ApplyAnimateDiffModel`

2. **VideoHelperSuite**
   - `VHS_VideoCombine`

## ワークフロー一覧

### animatediff_txt2vid.json
テキストからビデオを生成（Text-to-Video）

**パラメータ:**
- `width`, `height`: 出力解像度（512x512推奨）
- `batch_size`: フレーム数（16 = 約2秒 @ 8fps）
- `seed`: ランダムシード
- `text` (node 4): ポジティブプロンプト
- `text` (node 5): ネガティブプロンプト

### animatediff_img2vid.json
画像からビデオを生成（Image-to-Video）

**パラメータ:**
- `image` (node 4): 入力画像パス
- `denoise`: 変化の強さ（0.4-0.8推奨）
- `amount`: フレーム数

### svd_img2vid.json
Stable Video Diffusionによる高品質Image-to-Video

**パラメータ:**
- `image` (node 2): 入力画像パス
- `motion_bucket_id`: モーション強度（1-255、127がデフォルト）
- `video_frames`: フレーム数（14 or 25）
- `fps`: 出力FPS

## 使用方法

### ComfyUIで直接使用
1. ComfyUIを起動
2. ワークフローをドラッグ&ドロップ
3. パラメータを調整
4. Queue Prompt

### text-to-videoツールから使用
```python
from src.providers.comfyui_provider import ComfyUIProvider

provider = ComfyUIProvider(
    comfy_url="http://127.0.0.1:8188",
    workflow_path="workflows/animatediff_txt2vid.json"
)
```

## カスタマイズ

ワークフローをComfyUIで開き、以下を調整可能:
- Checkpoint（モデル）の変更
- LoRAの追加
- ControlNetの追加
- アップスケーラーの追加
- フレーム補間の追加

保存したワークフローは `workflow_path` パラメータで指定可能です。
