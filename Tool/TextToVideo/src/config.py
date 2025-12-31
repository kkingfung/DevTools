"""
設定管理
YAMLまたは環境変数から設定を読み込む
"""

import os
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


@dataclass
class ProviderConfig:
    """プロバイダー設定"""
    enabled: bool = True
    api_key: str = ""
    model: str = ""
    extra: dict = field(default_factory=dict)


@dataclass
class AppConfig:
    """アプリケーション設定"""
    output_dir: str = "./outputs"
    default_provider: str = "replicate"
    ffmpeg_path: str = "ffmpeg"

    # プロバイダー設定
    replicate: ProviderConfig = field(default_factory=ProviderConfig)
    fal: ProviderConfig = field(default_factory=ProviderConfig)
    runway: ProviderConfig = field(default_factory=ProviderConfig)
    comfyui: ProviderConfig = field(default_factory=lambda: ProviderConfig(
        enabled=True,
        extra={"url": "http://127.0.0.1:8188"}
    ))

    # 生成デフォルト
    default_width: int = 1280
    default_height: int = 720
    default_fps: int = 24


def load_config(config_path: str | None = None) -> AppConfig:
    """
    設定を読み込む

    優先順位:
    1. 環境変数
    2. 設定ファイル
    3. デフォルト値
    """
    config = AppConfig()

    # 設定ファイルから読み込み
    if config_path:
        config = _load_from_file(config_path, config)
    else:
        # デフォルトの設定ファイルパスをチェック
        default_paths = [
            Path("config.yaml"),
            Path("config.yml"),
            Path.home() / ".config" / "text-to-video" / "config.yaml",
        ]
        for path in default_paths:
            if path.exists():
                config = _load_from_file(str(path), config)
                break

    # 環境変数で上書き
    config = _load_from_env(config)

    return config


def _load_from_file(path: str, config: AppConfig) -> AppConfig:
    """YAMLファイルから設定を読み込む"""
    try:
        import yaml
    except ImportError:
        print("Warning: PyYAML not installed. Using default config.")
        return config

    file_path = Path(path)
    if not file_path.exists():
        return config

    with open(file_path, "r", encoding="utf-8") as f:
        data = yaml.safe_load(f) or {}

    # 基本設定
    config.output_dir = data.get("output_dir", config.output_dir)
    config.default_provider = data.get("default_provider", config.default_provider)
    config.ffmpeg_path = data.get("ffmpeg_path", config.ffmpeg_path)

    # プロバイダー設定
    providers = data.get("providers", {})

    if "replicate" in providers:
        config.replicate = _parse_provider_config(providers["replicate"])

    if "fal" in providers:
        config.fal = _parse_provider_config(providers["fal"])

    if "runway" in providers:
        config.runway = _parse_provider_config(providers["runway"])

    if "comfyui" in providers:
        config.comfyui = _parse_provider_config(providers["comfyui"])

    # デフォルト生成設定
    defaults = data.get("defaults", {})
    config.default_width = defaults.get("width", config.default_width)
    config.default_height = defaults.get("height", config.default_height)
    config.default_fps = defaults.get("fps", config.default_fps)

    return config


def _parse_provider_config(data: dict) -> ProviderConfig:
    """プロバイダー設定をパース"""
    return ProviderConfig(
        enabled=data.get("enabled", True),
        api_key=data.get("api_key", ""),
        model=data.get("model", ""),
        extra=data.get("extra", {})
    )


def _load_from_env(config: AppConfig) -> AppConfig:
    """環境変数から設定を読み込む"""

    # 出力ディレクトリ
    if env_val := os.environ.get("T2V_OUTPUT_DIR"):
        config.output_dir = env_val

    # デフォルトプロバイダー
    if env_val := os.environ.get("T2V_DEFAULT_PROVIDER"):
        config.default_provider = env_val

    # FFmpegパス
    if env_val := os.environ.get("T2V_FFMPEG_PATH"):
        config.ffmpeg_path = env_val

    # Replicate
    if env_val := os.environ.get("REPLICATE_API_TOKEN"):
        config.replicate.api_key = env_val
        config.replicate.enabled = True

    # Fal
    if env_val := os.environ.get("FAL_KEY"):
        config.fal.api_key = env_val
        config.fal.enabled = True

    # Runway
    if env_val := os.environ.get("RUNWAY_API_KEY"):
        config.runway.api_key = env_val
        config.runway.enabled = True

    # ComfyUI URL
    if env_val := os.environ.get("COMFYUI_URL"):
        config.comfyui.extra["url"] = env_val

    return config


def save_config(config: AppConfig, path: str):
    """設定をYAMLファイルに保存"""
    try:
        import yaml
    except ImportError:
        raise ImportError("PyYAML required. Run: pip install pyyaml")

    data = {
        "output_dir": config.output_dir,
        "default_provider": config.default_provider,
        "ffmpeg_path": config.ffmpeg_path,
        "providers": {
            "replicate": {
                "enabled": config.replicate.enabled,
                "api_key": config.replicate.api_key,
                "model": config.replicate.model,
            },
            "fal": {
                "enabled": config.fal.enabled,
                "api_key": config.fal.api_key,
                "model": config.fal.model,
            },
            "runway": {
                "enabled": config.runway.enabled,
                "api_key": config.runway.api_key,
            },
            "comfyui": {
                "enabled": config.comfyui.enabled,
                "url": config.comfyui.extra.get("url", "http://127.0.0.1:8188"),
            },
        },
        "defaults": {
            "width": config.default_width,
            "height": config.default_height,
            "fps": config.default_fps,
        },
    }

    file_path = Path(path)
    file_path.parent.mkdir(parents=True, exist_ok=True)

    with open(file_path, "w", encoding="utf-8") as f:
        yaml.dump(data, f, default_flow_style=False, allow_unicode=True)
