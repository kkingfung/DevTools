"""
ComfyUI ローカルプロバイダー実装
ローカルGPUでAnimateDiff等を実行
"""

import json
import time
import urllib.request
import urllib.error
from pathlib import Path
from typing import Callable
from uuid import uuid4

from .base import (
    VideoProvider,
    ProviderType,
    ProviderCapabilities,
    GenerationRequest,
    GenerationResult,
)


class ComfyUIProvider(VideoProvider):
    """ComfyUI APIを使用したローカルビデオ生成プロバイダー"""

    def __init__(
        self,
        comfy_url: str = "http://127.0.0.1:8188",
        workflow_path: str | None = None,
        output_dir: str = "./outputs"
    ):
        """
        Args:
            comfy_url: ComfyUI APIのURL
            workflow_path: カスタムワークフローJSONのパス
            output_dir: 出力ディレクトリ
        """
        self.comfy_url = comfy_url.rstrip("/")
        self.workflow_path = workflow_path
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)

        self._default_workflow = self._get_default_workflow()

    @property
    def name(self) -> str:
        return "ComfyUI (Local)"

    @property
    def provider_type(self) -> ProviderType:
        return ProviderType.COMFYUI

    def get_capabilities(self) -> ProviderCapabilities:
        return ProviderCapabilities(
            max_clip_duration=6,  # AnimateDiffのデフォルト
            supports_image_ref=True,
            supports_negative_prompt=True,
            supported_resolutions=[
                (512, 512),
                (768, 512),
                (512, 768),
                (1024, 576),
                (576, 1024),
            ],
            cost_per_second=None,  # ローカルなのでコストなし
        )

    def is_available(self) -> bool:
        """ComfyUIサーバーが起動しているかチェック"""
        try:
            req = urllib.request.Request(f"{self.comfy_url}/system_stats")
            with urllib.request.urlopen(req, timeout=5) as response:
                return response.status == 200
        except (urllib.error.URLError, TimeoutError):
            return False

    def generate_clip(
        self,
        request: GenerationRequest,
        on_progress: Callable[[float], None] | None = None
    ) -> GenerationResult:
        """ComfyUIでクリップを生成"""

        if not self.is_available():
            raise RuntimeError(
                f"ComfyUI server not available at {self.comfy_url}. "
                "Please start ComfyUI first."
            )

        # ワークフローを読み込み・設定
        workflow = self._load_workflow()
        workflow = self._configure_workflow(workflow, request)

        if on_progress:
            on_progress(0.1)

        # プロンプトをキューに追加
        prompt_id = self._queue_prompt(workflow)

        # 完了を待機
        video_path = self._wait_for_completion(prompt_id, on_progress)

        return GenerationResult(
            video_path=str(video_path),
            duration=float(self.get_capabilities().max_clip_duration),
            provider=self.name,
            seed=request.seed,
            metadata={"prompt_id": prompt_id}
        )

    def _load_workflow(self) -> dict:
        """ワークフローを読み込む"""
        if self.workflow_path:
            path = Path(self.workflow_path)
            if path.exists():
                with open(path, "r", encoding="utf-8") as f:
                    return json.load(f)
        return self._default_workflow.copy()

    def _configure_workflow(
        self,
        workflow: dict,
        request: GenerationRequest
    ) -> dict:
        """ワークフローにリクエストパラメータを設定"""

        # ノードを探して設定を更新
        for node_id, node in workflow.items():
            class_type = node.get("class_type", "")
            inputs = node.get("inputs", {})

            # テキストプロンプトノード
            if class_type in ("CLIPTextEncode", "CLIPTextEncodeSDXL"):
                if "positive" in node_id.lower() or inputs.get("text", ""):
                    inputs["text"] = request.prompt
                elif "negative" in node_id.lower():
                    inputs["text"] = request.negative_prompt

            # サンプラーノード
            if "Sampler" in class_type or "KSampler" in class_type:
                if request.seed is not None:
                    inputs["seed"] = request.seed

            # 解像度設定
            if class_type == "EmptyLatentImage":
                inputs["width"] = request.width
                inputs["height"] = request.height

        return workflow

    def _queue_prompt(self, workflow: dict) -> str:
        """ワークフローをキューに追加"""
        prompt_id = str(uuid4())

        data = json.dumps({
            "prompt": workflow,
            "client_id": prompt_id
        }).encode("utf-8")

        req = urllib.request.Request(
            f"{self.comfy_url}/prompt",
            data=data,
            headers={"Content-Type": "application/json"},
            method="POST"
        )

        with urllib.request.urlopen(req) as response:
            result = json.loads(response.read())
            return result.get("prompt_id", prompt_id)

    def _wait_for_completion(
        self,
        prompt_id: str,
        on_progress: Callable[[float], None] | None = None
    ) -> Path:
        """生成完了を待機"""

        max_wait = 600  # 最大10分
        start_time = time.time()

        while time.time() - start_time < max_wait:
            # 履歴をチェック
            req = urllib.request.Request(f"{self.comfy_url}/history/{prompt_id}")

            try:
                with urllib.request.urlopen(req) as response:
                    history = json.loads(response.read())

                    if prompt_id in history:
                        outputs = history[prompt_id].get("outputs", {})

                        # ビデオ出力を探す
                        for node_id, node_output in outputs.items():
                            if "gifs" in node_output:
                                # AnimateDiffの出力
                                gif_info = node_output["gifs"][0]
                                return self._download_from_comfy(gif_info)
                            elif "videos" in node_output:
                                video_info = node_output["videos"][0]
                                return self._download_from_comfy(video_info)

                        # 出力が見つからない場合
                        raise RuntimeError("No video output found in workflow")

            except urllib.error.HTTPError as e:
                if e.code != 404:
                    raise

            # 進捗更新
            elapsed = time.time() - start_time
            if on_progress:
                on_progress(min(0.1 + (elapsed / max_wait) * 0.8, 0.9))

            time.sleep(2)

        raise TimeoutError(f"Generation timed out after {max_wait}s")

    def _download_from_comfy(self, file_info: dict) -> Path:
        """ComfyUIから生成ファイルをダウンロード"""
        filename = file_info["filename"]
        subfolder = file_info.get("subfolder", "")

        url = f"{self.comfy_url}/view?filename={filename}"
        if subfolder:
            url += f"&subfolder={subfolder}"

        output_path = self.output_dir / f"clip_{int(time.time() * 1000)}.mp4"
        urllib.request.urlretrieve(url, output_path)

        return output_path

    def _get_default_workflow(self) -> dict:
        """デフォルトのAnimateDiffワークフローを返す"""
        # 簡略化したワークフロー構造
        # 実際のワークフローはComfyUIからエクスポートして使用する
        return {
            "1": {
                "class_type": "CheckpointLoaderSimple",
                "inputs": {"ckpt_name": "sd_xl_base_1.0.safetensors"}
            },
            "2": {
                "class_type": "CLIPTextEncode",
                "inputs": {"text": "", "clip": ["1", 1]},
                "_meta": {"title": "positive"}
            },
            "3": {
                "class_type": "CLIPTextEncode",
                "inputs": {"text": "", "clip": ["1", 1]},
                "_meta": {"title": "negative"}
            },
            "4": {
                "class_type": "EmptyLatentImage",
                "inputs": {"width": 1024, "height": 576, "batch_size": 16}
            },
            "5": {
                "class_type": "KSampler",
                "inputs": {
                    "seed": 0,
                    "steps": 20,
                    "cfg": 7.0,
                    "sampler_name": "euler",
                    "scheduler": "normal",
                    "denoise": 1.0,
                    "model": ["1", 0],
                    "positive": ["2", 0],
                    "negative": ["3", 0],
                    "latent_image": ["4", 0]
                }
            }
        }
