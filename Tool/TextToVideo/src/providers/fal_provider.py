"""
Fal.ai APIプロバイダー実装
高速推論に最適化されたビデオ生成
"""

import time
import urllib.request
import base64
from pathlib import Path
from typing import Callable

from .base import (
    VideoProvider,
    ProviderType,
    ProviderCapabilities,
    GenerationRequest,
    GenerationResult,
)


class FalProvider(VideoProvider):
    """Fal.ai APIを使用したビデオ生成プロバイダー"""

    # 利用可能なモデル設定
    MODELS = {
        "minimax": {
            "id": "fal-ai/minimax-video",
            "max_duration": 6,
            "supports_image_ref": True,
        },
        "kling": {
            "id": "fal-ai/kling-video/v1/standard/text-to-video",
            "max_duration": 5,
            "supports_image_ref": False,
        },
        "luma": {
            "id": "fal-ai/luma-dream-machine",
            "max_duration": 5,
            "supports_image_ref": True,
        },
        "hunyuan": {
            "id": "fal-ai/hunyuan-video",
            "max_duration": 5,
            "supports_image_ref": False,
        },
    }

    def __init__(
        self,
        api_key: str,
        model_name: str = "minimax",
        output_dir: str = "./outputs"
    ):
        """
        Args:
            api_key: Fal.ai APIキー
            model_name: 使用するモデル名
            output_dir: 出力ディレクトリ
        """
        self.api_key = api_key
        self.model_name = model_name
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)

        if model_name not in self.MODELS:
            raise ValueError(
                f"Unknown model: {model_name}. "
                f"Available: {list(self.MODELS.keys())}"
            )

        self._model_config = self.MODELS[model_name]
        self._client = None

    @property
    def name(self) -> str:
        return f"Fal.ai ({self.model_name})"

    @property
    def provider_type(self) -> ProviderType:
        return ProviderType.FAL

    def _get_client(self):
        """遅延初期化でFalクライアントを取得"""
        if self._client is None:
            try:
                import fal_client
                self._client = fal_client
            except ImportError:
                raise ImportError(
                    "fal-client package not installed. "
                    "Run: pip install fal-client"
                )
        return self._client

    def get_capabilities(self) -> ProviderCapabilities:
        return ProviderCapabilities(
            max_clip_duration=self._model_config["max_duration"],
            supports_image_ref=self._model_config["supports_image_ref"],
            supports_negative_prompt=True,
            supported_resolutions=[
                (1280, 720),
                (720, 1280),
                (1024, 1024),
                (768, 768),
            ],
            cost_per_second=0.04,  # 概算
        )

    def is_available(self) -> bool:
        """APIキーが有効かチェック"""
        if not self.api_key:
            return False
        try:
            import os
            os.environ["FAL_KEY"] = self.api_key
            return True
        except Exception:
            return False

    def generate_clip(
        self,
        request: GenerationRequest,
        on_progress: Callable[[float], None] | None = None
    ) -> GenerationResult:
        """Fal.ai APIでクリップを生成"""

        # リクエスト検証
        errors = self.validate_request(request)
        if errors:
            raise ValueError(f"Invalid request: {', '.join(errors)}")

        import os
        os.environ["FAL_KEY"] = self.api_key

        client = self._get_client()

        # モデル固有の入力を構築
        model_input = self._build_model_input(request)

        if on_progress:
            on_progress(0.1)

        try:
            # 進捗ハンドラ
            def handle_queue_update(update):
                if on_progress and hasattr(update, 'logs'):
                    # ログから進捗を推定
                    on_progress(0.5)

            # 生成実行（subscribe方式で進捗追跡）
            result = client.subscribe(
                self._model_config["id"],
                arguments=model_input,
                with_logs=True,
                on_queue_update=handle_queue_update
            )

            if on_progress:
                on_progress(0.8)

            # 結果をダウンロード
            video_url = self._extract_video_url(result)
            video_path = self._download_video(video_url)

            if on_progress:
                on_progress(1.0)

            return GenerationResult(
                video_path=str(video_path),
                duration=float(self._model_config["max_duration"]),
                provider=self.name,
                seed=request.seed,
                metadata={"model_id": self._model_config["id"]}
            )

        except Exception as e:
            raise RuntimeError(f"Generation failed: {e}")

    def _build_model_input(self, request: GenerationRequest) -> dict:
        """モデル固有の入力パラメータを構築"""

        if self.model_name == "minimax":
            input_data = {
                "prompt": request.prompt,
            }
            if request.reference_image:
                input_data["first_frame_image_url"] = self._prepare_image(
                    request.reference_image
                )

        elif self.model_name == "kling":
            input_data = {
                "prompt": request.prompt,
                "duration": "5",
                "aspect_ratio": self._get_aspect_ratio(request.width, request.height),
            }
            if request.negative_prompt:
                input_data["negative_prompt"] = request.negative_prompt

        elif self.model_name == "luma":
            input_data = {
                "prompt": request.prompt,
            }
            if request.reference_image:
                input_data["image_url"] = self._prepare_image(
                    request.reference_image
                )

        elif self.model_name == "hunyuan":
            input_data = {
                "prompt": request.prompt,
                "resolution": f"{request.width}x{request.height}",
            }
            if request.seed:
                input_data["seed"] = request.seed

        else:
            input_data = {"prompt": request.prompt}

        return input_data

    def _get_aspect_ratio(self, width: int, height: int) -> str:
        """アスペクト比を文字列で返す"""
        ratio = width / height
        if abs(ratio - 16/9) < 0.1:
            return "16:9"
        elif abs(ratio - 9/16) < 0.1:
            return "9:16"
        elif abs(ratio - 1.0) < 0.1:
            return "1:1"
        else:
            return "16:9"  # デフォルト

    def _prepare_image(self, image_path: str) -> str:
        """画像をAPIに送信可能な形式に変換"""
        path = Path(image_path)

        if path.exists():
            # ローカルファイルの場合、data URIに変換
            with open(path, "rb") as f:
                data = base64.b64encode(f.read()).decode()
            suffix = path.suffix.lower()
            mime = "image/png" if suffix == ".png" else "image/jpeg"
            return f"data:{mime};base64,{data}"
        elif image_path.startswith(("http://", "https://", "data:")):
            return image_path
        else:
            raise ValueError(f"Invalid image path: {image_path}")

    def _extract_video_url(self, result) -> str:
        """API出力からビデオURLを抽出"""
        if isinstance(result, dict):
            # 一般的なレスポンス形式
            if "video" in result:
                video = result["video"]
                if isinstance(video, dict) and "url" in video:
                    return video["url"]
                return video
            if "video_url" in result:
                return result["video_url"]
            if "url" in result:
                return result["url"]

        raise ValueError(f"Unexpected output format: {result}")

    def _download_video(self, url: str) -> Path:
        """ビデオをダウンロードしてローカルに保存"""
        timestamp = int(time.time() * 1000)
        filename = f"clip_{timestamp}.mp4"
        output_path = self.output_dir / filename

        urllib.request.urlretrieve(url, output_path)

        return output_path
