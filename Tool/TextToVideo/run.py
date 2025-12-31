#!/usr/bin/env python3
"""
Text to Video Generator - 起動スクリプト

Usage:
    python run.py
    python run.py --port 8080
    python run.py --share
"""

import os
import sys
import logging
from pathlib import Path

# ログ設定
logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s [%(levelname)s] %(message)s',
    handlers=[
        logging.StreamHandler(sys.stdout),
        logging.FileHandler('debug.log', encoding='utf-8')
    ]
)
logger = logging.getLogger(__name__)

# 出力バッファリングを無効化
os.environ["PYTHONUNBUFFERED"] = "1"

logger.info("=" * 50)
logger.info("  Text to Video Generator - Starting")
logger.info("=" * 50)
logger.info(f"Python: {sys.version}")
logger.info(f"Working directory: {os.getcwd()}")

# srcをパスに追加
src_path = Path(__file__).parent / "src"
sys.path.insert(0, str(src_path.parent))
logger.info(f"Added to path: {src_path.parent}")

try:
    logger.info("Importing config...")
    from src.config import load_config
    logger.info("Config imported OK")

    logger.info("Importing providers...")
    from src.providers.base import VideoProvider
    logger.info("Providers imported OK")

    logger.info("Importing gradio...")
    import gradio as gr
    logger.info(f"Gradio version: {gr.__version__}")

    logger.info("Importing TextToVideoApp...")
    from src.ui.gradio_app import TextToVideoApp
    logger.info("TextToVideoApp imported OK")

except Exception as e:
    logger.exception(f"Import error: {e}")
    sys.exit(1)

def main():
    import argparse

    logger.info("Parsing arguments...")
    parser = argparse.ArgumentParser(description="Text to Video Generator")
    parser.add_argument("--config", type=str, help="設定ファイルパス")
    parser.add_argument("--share", action="store_true", help="公開リンクを生成")
    parser.add_argument("--port", type=int, default=7860, help="ポート番号")
    args = parser.parse_args()
    logger.info(f"Args: port={args.port}, share={args.share}, config={args.config}")

    logger.info("Loading config...")
    config = load_config(args.config)
    logger.info("Config loaded OK")

    logger.info("Creating TextToVideoApp...")
    app = TextToVideoApp(config)
    logger.info("App created OK")

    providers = app.get_available_providers()
    if providers:
        logger.info(f"Available providers: {providers}")
    else:
        logger.warning("No providers available!")
        print("\nWarning: No providers available. Set API keys or start ComfyUI.")
        print("  - REPLICATE_API_TOKEN for Replicate")
        print("  - FAL_KEY for Fal.ai")
        print("  - Start ComfyUI on localhost:8188 for local inference\n")

    logger.info(f"Launching Gradio on port {args.port}...")
    app.launch(share=args.share, server_port=args.port)

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        logger.exception(f"Fatal error: {e}")
        sys.exit(1)
