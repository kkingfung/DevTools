"""
Gradio Web UI
テキスト→ビデオ生成インターフェース（オーディオ機能付き）
"""

import tempfile
from pathlib import Path

try:
    import gradio as gr
except ImportError:
    raise ImportError("Gradio required. Run: pip install gradio")

from ..config import load_config, AppConfig
from ..providers.base import GenerationRequest, ProviderType
from ..providers.replicate_provider import ReplicateProvider
from ..providers.comfyui_provider import ComfyUIProvider
from ..core.manager import GenerationManager
from ..core.audio import AudioProcessor, AudioTrack, AudioConfig, AIAudioGenerator


class TextToVideoApp:
    """Gradio UIアプリケーション"""

    def __init__(self, config: AppConfig | None = None):
        self.config = config or load_config()
        self.providers: dict[str, any] = {}
        self.current_manager: GenerationManager | None = None
        self.audio_processor = AudioProcessor(self.config.ffmpeg_path)

        self._init_providers()

    def _init_providers(self):
        """利用可能なプロバイダーを初期化"""

        # Replicate
        if self.config.replicate.enabled and self.config.replicate.api_key:
            try:
                model = self.config.replicate.model or "minimax"
                provider = ReplicateProvider(
                    api_key=self.config.replicate.api_key,
                    model_name=model,
                    output_dir=self.config.output_dir
                )
                self.providers["Replicate"] = provider
            except Exception as e:
                print(f"Failed to init Replicate: {e}")

        # Fal.ai
        if self.config.fal.enabled and self.config.fal.api_key:
            try:
                from ..providers.fal_provider import FalProvider
                model = self.config.fal.model or "minimax"
                provider = FalProvider(
                    api_key=self.config.fal.api_key,
                    model_name=model,
                    output_dir=self.config.output_dir
                )
                self.providers["Fal.ai"] = provider
            except Exception as e:
                print(f"Failed to init Fal.ai: {e}")

        # ComfyUI (ローカル)
        if self.config.comfyui.enabled:
            try:
                # ワークフローパスを取得
                workflow_path = self.config.comfyui.extra.get("workflow_path")

                provider = ComfyUIProvider(
                    comfy_url=self.config.comfyui.extra.get(
                        "url", "http://127.0.0.1:8188"
                    ),
                    workflow_path=workflow_path,
                    output_dir=self.config.output_dir
                )
                # 利用可能かチェック
                if provider.is_available():
                    self.providers["ComfyUI (Local)"] = provider
            except Exception as e:
                print(f"Failed to init ComfyUI: {e}")

    def get_available_providers(self) -> list[str]:
        """利用可能なプロバイダー名のリストを返す"""
        return list(self.providers.keys())

    def generate_video(
        self,
        prompt: str,
        provider_name: str,
        duration: int,
        reference_image: str | None,
        negative_prompt: str,
        width: int,
        height: int,
        seed: int | None,
        transition: str,
        progress=gr.Progress()
    ) -> tuple[str | None, str]:
        """
        ビデオを生成

        Returns:
            (video_path, status_message)
        """
        if not prompt.strip():
            return None, "❌ プロンプトを入力してください"

        if provider_name not in self.providers:
            return None, f"❌ プロバイダー '{provider_name}' が利用できません"

        provider = self.providers[provider_name]
        manager = GenerationManager(
            provider=provider,
            output_dir=self.config.output_dir
        )

        # リクエスト作成
        request = GenerationRequest(
            prompt=prompt,
            duration=duration,
            reference_image=reference_image if reference_image else None,
            negative_prompt=negative_prompt,
            seed=seed if seed and seed > 0 else None,
            width=int(width),
            height=int(height),
            fps=self.config.default_fps
        )

        # 進捗コールバック
        def on_progress(current_clip: int, total_clips: int, clip_progress: float):
            overall = ((current_clip - 1) + clip_progress) / total_clips
            progress(overall, desc=f"クリップ {current_clip}/{total_clips} 生成中...")

        try:
            # コスト見積もり
            estimated_cost = manager.estimate_cost(request)
            cost_str = f"${estimated_cost:.2f}" if estimated_cost else "N/A"

            progress(0, desc="生成開始...")

            result = manager.generate(
                request=request,
                on_progress=on_progress,
                transition=transition
            )

            status = (
                f"✅ 生成完了!\n"
                f"- 長さ: {result.total_duration:.1f}秒\n"
                f"- クリップ数: {result.num_clips}\n"
                f"- 生成時間: {result.generation_time:.1f}秒\n"
                f"- 推定コスト: {cost_str}"
            )

            return result.video_path, status

        except Exception as e:
            return None, f"❌ エラー: {str(e)}"

    def add_audio(
        self,
        video_path: str | None,
        bgm_file: str | None,
        bgm_volume: float,
        bgm_fade_out: float,
        vo_file: str | None,
        vo_volume: float,
        sfx_file: str | None,
        sfx_volume: float,
        sfx_timestamp: float,
        progress=gr.Progress()
    ) -> tuple[str | None, str]:
        """
        ビデオにオーディオを追加

        Returns:
            (video_path, status_message)
        """
        if not video_path:
            return None, "❌ 先にビデオを生成してください"

        progress(0.1, desc="オーディオ処理中...")

        # オーディオ設定を構築
        config = AudioConfig()

        # BGM
        if bgm_file:
            config.background_music = AudioTrack(
                path=bgm_file,
                volume=bgm_volume,
                loop=True,
                fade_out=bgm_fade_out
            )

        # ナレーション
        if vo_file:
            config.voice_over = AudioTrack(
                path=vo_file,
                volume=vo_volume
            )

        # 効果音
        if sfx_file:
            config.sound_effects = [
                (AudioTrack(path=sfx_file, volume=sfx_volume), sfx_timestamp)
            ]

        # オーディオがない場合
        if not any([bgm_file, vo_file, sfx_file]):
            return video_path, "⚠️ オーディオファイルが指定されていません"

        try:
            progress(0.3, desc="オーディオをミックス中...")

            # 出力パス
            output_dir = Path(self.config.output_dir)
            output_path = output_dir / f"video_with_audio_{Path(video_path).stem}.mp4"

            # オーディオ追加
            def audio_progress(p: float):
                progress(0.3 + p * 0.6, desc="オーディオをミックス中...")

            result_path = self.audio_processor.add_audio_to_video(
                video_path=video_path,
                output_path=str(output_path),
                config=config,
                on_progress=audio_progress
            )

            progress(1.0, desc="完了!")

            return result_path, "✅ オーディオを追加しました"

        except Exception as e:
            return video_path, f"❌ オーディオ追加エラー: {str(e)}"

    def generate_ai_music(
        self,
        music_prompt: str,
        music_duration: int,
        progress=gr.Progress()
    ) -> tuple[str | None, str]:
        """
        AIで音楽を生成

        Returns:
            (audio_path, status_message)
        """
        if not music_prompt.strip():
            return None, "❌ 音楽のプロンプトを入力してください"

        if not self.config.replicate.api_key:
            return None, "❌ REPLICATE_API_TOKEN が設定されていません"

        try:
            import os
            os.environ["REPLICATE_API_TOKEN"] = self.config.replicate.api_key

            progress(0.1, desc="AI音楽を生成中...")

            generator = AIAudioGenerator()

            output_dir = Path(self.config.output_dir)
            output_dir.mkdir(parents=True, exist_ok=True)

            import time
            output_path = output_dir / f"music_{int(time.time())}.wav"

            result = generator.generate_music(
                prompt=music_prompt,
                output_path=str(output_path),
                duration=music_duration
            )

            progress(1.0, desc="完了!")

            return result, f"✅ 音楽を生成しました ({music_duration}秒)"

        except Exception as e:
            return None, f"❌ 音楽生成エラー: {str(e)}"

    def build_ui(self) -> gr.Blocks:
        """Gradio UIを構築"""

        available_providers = self.get_available_providers()

        with gr.Blocks(
            title="Text to Video Generator"
        ) as app:

            gr.Markdown("# 🎬 Text to Video Generator")
            gr.Markdown("テキストプロンプトからビデオを生成し、オーディオを追加できます")

            with gr.Tabs():
                # タブ1: ビデオ生成
                with gr.TabItem("🎬 ビデオ生成"):
                    with gr.Row():
                        # 左カラム: 入力
                        with gr.Column(scale=1):
                            prompt = gr.Textbox(
                                label="プロンプト",
                                placeholder="生成したいビデオの説明を入力...",
                                lines=3
                            )

                            negative_prompt = gr.Textbox(
                                label="ネガティブプロンプト",
                                placeholder="避けたい要素...",
                                lines=2
                            )

                            reference_image = gr.Image(
                                label="参照画像（オプション）",
                                type="filepath",
                                height=200
                            )

                            with gr.Row():
                                provider = gr.Dropdown(
                                    choices=available_providers,
                                    value=available_providers[0] if available_providers else None,
                                    label="プロバイダー"
                                )

                                duration = gr.Slider(
                                    minimum=5,
                                    maximum=90,
                                    value=5,
                                    step=5,
                                    label="長さ（秒）"
                                )

                            with gr.Accordion("詳細設定", open=False):
                                with gr.Row():
                                    width = gr.Number(
                                        value=self.config.default_width,
                                        label="幅",
                                        precision=0
                                    )
                                    height = gr.Number(
                                        value=self.config.default_height,
                                        label="高さ",
                                        precision=0
                                    )

                                seed = gr.Number(
                                    value=0,
                                    label="シード (0=ランダム)",
                                    precision=0
                                )

                                transition = gr.Dropdown(
                                    choices=["none", "fade", "dissolve"],
                                    value="none",
                                    label="トランジション"
                                )

                            generate_btn = gr.Button(
                                "🎬 ビデオ生成",
                                variant="primary",
                                size="lg"
                            )

                        # 右カラム: 出力
                        with gr.Column(scale=1):
                            output_video = gr.Video(
                                label="生成されたビデオ",
                                height=400
                            )

                            status = gr.Textbox(
                                label="ステータス",
                                lines=5,
                                interactive=False
                            )

                    # プロバイダーが無い場合の警告
                    if not available_providers:
                        gr.Markdown(
                            "⚠️ **利用可能なプロバイダーがありません**\n\n"
                            "以下のいずれかを設定してください:\n"
                            "- `REPLICATE_API_TOKEN` 環境変数を設定\n"
                            "- `FAL_KEY` 環境変数を設定\n"
                            "- ComfyUIをローカルで起動\n"
                            "- `config.yaml` で設定"
                        )

                # タブ2: オーディオ追加
                with gr.TabItem("🔊 オーディオ追加"):
                    with gr.Row():
                        with gr.Column(scale=1):
                            gr.Markdown("### BGM（バックグラウンドミュージック）")
                            bgm_file = gr.Audio(
                                label="BGMファイル",
                                type="filepath"
                            )
                            with gr.Row():
                                bgm_volume = gr.Slider(
                                    minimum=0,
                                    maximum=1,
                                    value=0.3,
                                    step=0.05,
                                    label="BGM音量"
                                )
                                bgm_fade_out = gr.Slider(
                                    minimum=0,
                                    maximum=5,
                                    value=2,
                                    step=0.5,
                                    label="フェードアウト（秒）"
                                )

                            gr.Markdown("### ナレーション/ボイスオーバー")
                            vo_file = gr.Audio(
                                label="ナレーションファイル",
                                type="filepath"
                            )
                            vo_volume = gr.Slider(
                                minimum=0,
                                maximum=2,
                                value=1.0,
                                step=0.1,
                                label="ナレーション音量"
                            )

                            gr.Markdown("### 効果音")
                            sfx_file = gr.Audio(
                                label="効果音ファイル",
                                type="filepath"
                            )
                            with gr.Row():
                                sfx_volume = gr.Slider(
                                    minimum=0,
                                    maximum=2,
                                    value=1.0,
                                    step=0.1,
                                    label="効果音音量"
                                )
                                sfx_timestamp = gr.Number(
                                    value=0,
                                    label="再生位置（秒）",
                                    precision=1
                                )

                            add_audio_btn = gr.Button(
                                "🔊 オーディオを追加",
                                variant="primary",
                                size="lg"
                            )

                        with gr.Column(scale=1):
                            output_video_audio = gr.Video(
                                label="オーディオ付きビデオ",
                                height=400
                            )
                            audio_status = gr.Textbox(
                                label="ステータス",
                                lines=3,
                                interactive=False
                            )

                # タブ3: AI音楽生成
                with gr.TabItem("🎵 AI音楽生成"):
                    with gr.Row():
                        with gr.Column(scale=1):
                            music_prompt = gr.Textbox(
                                label="音楽プロンプト",
                                placeholder="例: upbeat electronic music with synth leads, energetic and modern",
                                lines=3
                            )

                            music_duration = gr.Slider(
                                minimum=10,
                                maximum=60,
                                value=30,
                                step=5,
                                label="長さ（秒）"
                            )

                            generate_music_btn = gr.Button(
                                "🎵 音楽を生成",
                                variant="primary",
                                size="lg"
                            )

                            gr.Markdown(
                                "**Note:** MusicGen (Meta) を使用。"
                                "REPLICATE_API_TOKEN が必要です。"
                            )

                        with gr.Column(scale=1):
                            output_music = gr.Audio(
                                label="生成された音楽",
                                type="filepath"
                            )
                            music_status = gr.Textbox(
                                label="ステータス",
                                lines=3,
                                interactive=False
                            )

            # イベントハンドラ
            generate_btn.click(
                fn=self.generate_video,
                inputs=[
                    prompt,
                    provider,
                    duration,
                    reference_image,
                    negative_prompt,
                    width,
                    height,
                    seed,
                    transition
                ],
                outputs=[output_video, status]
            )

            add_audio_btn.click(
                fn=self.add_audio,
                inputs=[
                    output_video,
                    bgm_file,
                    bgm_volume,
                    bgm_fade_out,
                    vo_file,
                    vo_volume,
                    sfx_file,
                    sfx_volume,
                    sfx_timestamp
                ],
                outputs=[output_video_audio, audio_status]
            )

            generate_music_btn.click(
                fn=self.generate_ai_music,
                inputs=[music_prompt, music_duration],
                outputs=[output_music, music_status]
            )

        return app

    def launch(self, **kwargs):
        """アプリを起動"""
        app = self.build_ui()
        # Gradio 6.0+: theme is passed to launch()
        if "theme" not in kwargs:
            kwargs["theme"] = gr.themes.Soft()
        app.launch(**kwargs)


def main():
    """エントリーポイント"""
    import argparse

    parser = argparse.ArgumentParser(description="Text to Video Generator")
    parser.add_argument(
        "--config",
        type=str,
        help="設定ファイルパス"
    )
    parser.add_argument(
        "--share",
        action="store_true",
        help="公開リンクを生成"
    )
    parser.add_argument(
        "--port",
        type=int,
        default=7860,
        help="ポート番号"
    )

    args = parser.parse_args()

    config = load_config(args.config)
    app = TextToVideoApp(config)

    providers = app.get_available_providers()
    if providers:
        print(f"Available providers: {', '.join(providers)}")
    else:
        print("Warning: No providers available. Set API keys or start ComfyUI.")
        print("  - REPLICATE_API_TOKEN for Replicate")
        print("  - FAL_KEY for Fal.ai")
        print("  - Start ComfyUI on localhost:8188 for local inference")
    print()

    app.launch(share=args.share, server_port=args.port)


if __name__ == "__main__":
    main()
