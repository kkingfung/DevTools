"""
Text to Video Generator
テキストプロンプトからビデオを生成するツール
"""

__version__ = "0.2.0"

from .config import AppConfig, load_config
from .providers import (
    VideoProvider,
    ProviderType,
    GenerationRequest,
    GenerationResult,
    ReplicateProvider,
    FalProvider,
    ComfyUIProvider,
)
from .core import (
    GenerationManager,
    VideoStitcher,
    AudioProcessor,
    AudioTrack,
    AudioConfig,
    AIAudioGenerator,
)

__all__ = [
    # Config
    "AppConfig",
    "load_config",
    # Providers
    "VideoProvider",
    "ProviderType",
    "GenerationRequest",
    "GenerationResult",
    "ReplicateProvider",
    "FalProvider",
    "ComfyUIProvider",
    # Core
    "GenerationManager",
    "VideoStitcher",
    # Audio
    "AudioProcessor",
    "AudioTrack",
    "AudioConfig",
    "AIAudioGenerator",
]
