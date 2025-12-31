"""
ビデオ生成マネージャー
長時間ビデオ生成のためのクリップチェイニングを管理
"""

import time
from dataclasses import dataclass
from math import ceil
from pathlib import Path
from typing import Callable

from ..providers.base import (
    VideoProvider,
    GenerationRequest,
    GenerationResult,
)
from .stitcher import VideoStitcher


@dataclass
class LongVideoResult:
    """長時間ビデオ生成結果"""
    video_path: str
    total_duration: float
    num_clips: int
    clip_paths: list[str]
    provider: str
    generation_time: float


class GenerationManager:
    """
    ビデオ生成を管理するマネージャークラス
    長時間ビデオのためのクリップチェイニングを実装
    """

    def __init__(
        self,
        provider: VideoProvider,
        output_dir: str = "./outputs",
        stitcher: VideoStitcher | None = None
    ):
        """
        Args:
            provider: ビデオ生成プロバイダー
            output_dir: 出力ディレクトリ
            stitcher: ビデオスティッチャー（Noneの場合は自動作成）
        """
        self.provider = provider
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.stitcher = stitcher or VideoStitcher()

    def generate(
        self,
        request: GenerationRequest,
        on_progress: Callable[[int, int, float], None] | None = None,
        transition: str = "none"
    ) -> LongVideoResult:
        """
        ビデオを生成（必要に応じて複数クリップをチェイン）

        Args:
            request: 生成リクエスト
            on_progress: 進捗コールバック (current_clip, total_clips, clip_progress)
            transition: クリップ間のトランジション種類

        Returns:
            生成結果
        """
        start_time = time.time()

        caps = self.provider.get_capabilities()
        clip_duration = caps.max_clip_duration

        # 必要なクリップ数を計算
        num_clips = ceil(request.duration / clip_duration)

        if num_clips == 1:
            # 単一クリップの場合
            result = self._generate_single_clip(request, on_progress)
            return LongVideoResult(
                video_path=result.video_path,
                total_duration=result.duration,
                num_clips=1,
                clip_paths=[result.video_path],
                provider=self.provider.name,
                generation_time=time.time() - start_time
            )

        # 複数クリップの生成
        clips = self._generate_chained_clips(
            request, num_clips, on_progress
        )

        # クリップを結合
        timestamp = int(time.time() * 1000)
        output_path = self.output_dir / f"video_{timestamp}.mp4"

        final_video = self.stitcher.stitch_clips(
            clip_paths=clips,
            output_path=str(output_path),
            transition=transition
        )

        # 実際の長さを取得
        total_duration = self.stitcher.get_video_duration(final_video)

        return LongVideoResult(
            video_path=final_video,
            total_duration=total_duration,
            num_clips=num_clips,
            clip_paths=clips,
            provider=self.provider.name,
            generation_time=time.time() - start_time
        )

    def _generate_single_clip(
        self,
        request: GenerationRequest,
        on_progress: Callable[[int, int, float], None] | None = None
    ) -> GenerationResult:
        """単一クリップを生成"""

        def clip_progress(progress: float):
            if on_progress:
                on_progress(1, 1, progress)

        return self.provider.generate_clip(request, clip_progress)

    def _generate_chained_clips(
        self,
        request: GenerationRequest,
        num_clips: int,
        on_progress: Callable[[int, int, float], None] | None = None
    ) -> list[str]:
        """複数クリップをチェイン生成"""

        clips: list[str] = []
        current_request = GenerationRequest(
            prompt=request.prompt,
            duration=self.provider.get_capabilities().max_clip_duration,
            reference_image=request.reference_image,
            negative_prompt=request.negative_prompt,
            seed=request.seed,
            width=request.width,
            height=request.height,
            fps=request.fps
        )

        for i in range(num_clips):
            # 進捗コールバックをラップ
            def clip_progress(progress: float, clip_num=i):
                if on_progress:
                    on_progress(clip_num + 1, num_clips, progress)

            # クリップを生成
            result = self.provider.generate_clip(current_request, clip_progress)
            clips.append(result.video_path)

            # 次のクリップのために最後のフレームを抽出
            if i < num_clips - 1:
                last_frame_path = self.output_dir / f"frame_{i}.jpg"
                self.stitcher.extract_last_frame(
                    result.video_path,
                    str(last_frame_path)
                )
                current_request.reference_image = str(last_frame_path)

                # シードを変更して多様性を確保
                if current_request.seed is not None:
                    current_request.seed += 1

        return clips

    def estimate_cost(self, request: GenerationRequest) -> float | None:
        """
        生成コストを見積もる

        Returns:
            推定コスト（USD）、計算不可の場合はNone
        """
        caps = self.provider.get_capabilities()

        if caps.cost_per_second is None:
            return None

        return request.duration * caps.cost_per_second

    def estimate_time(self, request: GenerationRequest) -> float:
        """
        生成時間を見積もる（秒）

        非常に大まかな見積もり
        """
        caps = self.provider.get_capabilities()
        num_clips = ceil(request.duration / caps.max_clip_duration)

        # クリップあたり約60秒と仮定
        base_time = num_clips * 60

        # スティッチング時間を追加
        stitch_time = (num_clips - 1) * 5 if num_clips > 1 else 0

        return base_time + stitch_time
