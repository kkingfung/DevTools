#nullable enable
"""
ビデオ生成プロバイダーの基底インターフェース
各プロバイダー（Replicate、Fal.ai、ComfyUI等）はこのインターフェースを実装する
"""

from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from enum import Enum
from typing import Callable


class ProviderType(Enum):
    """利用可能なプロバイダータイプ"""
    REPLICATE = "replicate"
    FAL = "fal"
    RUNWAY = "runway"
    COMFYUI = "comfyui"  # ローカル


@dataclass
class GenerationRequest:
    """ビデオ生成リクエスト"""
    prompt: str
    duration: int = 5  # 秒数
    reference_image: str | None = None  # base64またはファイルパス
    negative_prompt: str = ""
    seed: int | None = None
    width: int = 1280
    height: int = 720
    fps: int = 24


@dataclass
class GenerationResult:
    """ビデオ生成結果"""
    video_path: str
    duration: float
    provider: str
    seed: int | None = None
    metadata: dict = field(default_factory=dict)


@dataclass
class ProviderCapabilities:
    """プロバイダーの機能情報"""
    max_clip_duration: int  # 最大クリップ長（秒）
    supports_image_ref: bool  # 画像参照サポート
    supports_negative_prompt: bool
    supported_resolutions: list[tuple[int, int]]
    cost_per_second: float | None = None  # APIコスト目安


class VideoProvider(ABC):
    """ビデオ生成プロバイダーの抽象基底クラス"""

    @property
    @abstractmethod
    def name(self) -> str:
        """プロバイダー名を返す"""
        pass

    @property
    @abstractmethod
    def provider_type(self) -> ProviderType:
        """プロバイダータイプを返す"""
        pass

    @abstractmethod
    def get_capabilities(self) -> ProviderCapabilities:
        """プロバイダーの機能情報を返す"""
        pass

    @abstractmethod
    def generate_clip(
        self,
        request: GenerationRequest,
        on_progress: Callable[[float], None] | None = None
    ) -> GenerationResult:
        """
        単一クリップを生成する

        Args:
            request: 生成リクエスト
            on_progress: 進捗コールバック (0.0 - 1.0)

        Returns:
            生成結果
        """
        pass

    @abstractmethod
    def is_available(self) -> bool:
        """プロバイダーが利用可能かチェック"""
        pass

    def validate_request(self, request: GenerationRequest) -> list[str]:
        """
        リクエストを検証し、エラーリストを返す

        Returns:
            エラーメッセージのリスト（空なら有効）
        """
        errors = []
        caps = self.get_capabilities()

        if request.duration > caps.max_clip_duration:
            errors.append(
                f"Duration {request.duration}s exceeds max {caps.max_clip_duration}s"
            )

        if request.reference_image and not caps.supports_image_ref:
            errors.append("This provider does not support image references")

        resolution = (request.width, request.height)
        if resolution not in caps.supported_resolutions:
            errors.append(
                f"Resolution {resolution} not supported. "
                f"Available: {caps.supported_resolutions}"
            )

        return errors
