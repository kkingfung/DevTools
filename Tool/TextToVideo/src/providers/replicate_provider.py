"""
Replicate APIプロバイダー実装
複数のビデオ生成モデルをサポート
"""

import base64
import time
import urllib.request
from pathlib import Path
from typing import Callable

from .base import (
    VideoProvider,
    ProviderType,
    ProviderCapabilities,
    GenerationRequest,
    GenerationResult,
)


class ReplicateProvider(VideoProvider):
    """Replicate APIを使用したビデオ生成プロバイダー"""

    # 利用可能なモデル設定
    MODELS = {
        "minimax": {
            "id": "minimax/video-01",
            "max_duration": 6,
            "supports_image_ref": True,
        },
        "luma": {
            "id": "luma/ray",
            "max_duration": 5,
            "supports_image_ref": True,
        },
        "stable-video": {
            "id": "stability-ai/stable-video-diffusion",
            "max_duration": 4,
            "supports_image_ref": True,  # 実際は必須
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
            api_key: Replicate APIキー
            model_name: 使用するモデル名 (minimax, luma, stable-video)
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
        return f"Replicate ({self.model_name})"

    @property
    def provider_type(self) -> ProviderType:
        return ProviderType.REPLICATE

    def _get_client(self):
        """遅延初期化でReplicateクライアントを取得"""
        if self._client is None:
            try:
                import replicate
                self._client = replicate.Client(api_token=self.api_key)
            except ImportError:
                raise ImportError(
                    "replicate package not installed. "
                    "Run: pip install replicate"
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
            ],
            cost_per_second=0.05,  # 概算
        )

    def is_available(self) -> bool:
        """APIキーが有効かチェック"""
        if not self.api_key:
            return False
        try:
            client = self._get_client()
            # 簡単なAPIコールでチェック
            return True
        except Exception:
            return False

    def generate_clip(
        self,
        request: GenerationRequest,
        on_progress: Callable[[float], None] | None = None
    ) -> GenerationResult:
        """Replicate APIでクリップを生成"""

        # リクエスト検証
        errors = self.validate_request(request)
        if errors:
            raise ValueError(f"Invalid request: {', '.join(errors)}")

        client = self._get_client()

        # モデル固有の入力を構築
        model_input = self._build_model_input(request)

        if on_progress:
            on_progress(0.1)

        # 生成実行
        try:
            output = client.run(
                self._model_config["id"],
                input=model_input
            )

            if on_progress:
                on_progress(0.8)

            # 結果をダウンロード
            video_url = self._extract_video_url(output)
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
                input_data["first_frame_image"] = self._prepare_image(
                    request.reference_image
                )

        elif self.model_name == "luma":
            input_data = {
                "prompt": request.prompt,
            }
            if request.reference_image:
                input_data["start_image"] = self._prepare_image(
                    request.reference_image
                )

        elif self.model_name == "stable-video":
            # SVDは画像必須
            if not request.reference_image:
                raise ValueError("stable-video requires a reference image")
            input_data = {
                "input_image": self._prepare_image(request.reference_image),
                "motion_bucket_id": 127,
                "fps": request.fps,
            }
        else:
            input_data = {"prompt": request.prompt}

        return input_data

    def _prepare_image(self, image_path: str) -> str:
        """画像をAPIに送信可能な形式に変換"""
        path = Path(image_path)

        if path.exists():
            # ローカルファイルの場合、base64エンコード
            with open(path, "rb") as f:
                data = base64.b64encode(f.read()).decode()
            suffix = path.suffix.lower()
            mime = "image/png" if suffix == ".png" else "image/jpeg"
            return f"data:{mime};base64,{data}"
        elif image_path.startswith(("http://", "https://", "data:")):
            # URLまたはdata URIの場合はそのまま
            return image_path
        else:
            raise ValueError(f"Invalid image path: {image_path}")

    def _extract_video_url(self, output) -> str:
        """API出力からビデオURLを抽出"""
        if isinstance(output, str):
            return output
        elif isinstance(output, list) and len(output) > 0:
            return output[0]
        elif hasattr(output, "url"):
            return output.url
        else:
            raise ValueError(f"Unexpected output format: {type(output)}")

    def _download_video(self, url: str) -> Path:
        """ビデオをダウンロードしてローカルに保存"""
        timestamp = int(time.time() * 1000)
        filename = f"clip_{timestamp}.mp4"
        output_path = self.output_dir / filename

        urllib.request.urlretrieve(url, output_path)

        return output_path
