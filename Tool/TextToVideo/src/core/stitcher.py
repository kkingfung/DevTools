"""
FFmpegを使用したビデオスティッチング
複数のクリップを1つのビデオに結合
"""

import subprocess
import tempfile
from pathlib import Path


class VideoStitcher:
    """FFmpegを使用してビデオクリップを結合"""

    def __init__(self, ffmpeg_path: str = "ffmpeg"):
        """
        Args:
            ffmpeg_path: FFmpegの実行パス
        """
        self.ffmpeg_path = ffmpeg_path
        self._verify_ffmpeg()

    def _verify_ffmpeg(self):
        """FFmpegが利用可能かチェック"""
        try:
            result = subprocess.run(
                [self.ffmpeg_path, "-version"],
                capture_output=True,
                text=True
            )
            if result.returncode != 0:
                raise RuntimeError("FFmpeg returned non-zero exit code")
        except FileNotFoundError:
            raise RuntimeError(
                f"FFmpeg not found at '{self.ffmpeg_path}'. "
                "Please install FFmpeg and ensure it's in your PATH."
            )

    def stitch_clips(
        self,
        clip_paths: list[str],
        output_path: str,
        transition: str = "none",
        transition_duration: float = 0.5
    ) -> str:
        """
        複数のクリップを1つのビデオに結合

        Args:
            clip_paths: クリップファイルパスのリスト
            output_path: 出力ファイルパス
            transition: トランジション種類 (none, fade, dissolve)
            transition_duration: トランジション時間（秒）

        Returns:
            出力ファイルパス
        """
        if len(clip_paths) == 0:
            raise ValueError("No clips provided")

        if len(clip_paths) == 1:
            # 1クリップのみの場合はコピー
            return self._copy_video(clip_paths[0], output_path)

        if transition == "none":
            return self._concat_simple(clip_paths, output_path)
        else:
            return self._concat_with_transition(
                clip_paths, output_path, transition, transition_duration
            )

    def _copy_video(self, input_path: str, output_path: str) -> str:
        """ビデオをコピー（再エンコードなし）"""
        cmd = [
            self.ffmpeg_path,
            "-y",  # 上書き確認なし
            "-i", input_path,
            "-c", "copy",
            output_path
        ]

        self._run_ffmpeg(cmd)
        return output_path

    def _concat_simple(
        self,
        clip_paths: list[str],
        output_path: str
    ) -> str:
        """シンプルな結合（トランジションなし）"""

        # concatファイルを作成
        with tempfile.NamedTemporaryFile(
            mode="w",
            suffix=".txt",
            delete=False
        ) as f:
            for path in clip_paths:
                # パスをエスケープ
                escaped_path = str(Path(path).absolute()).replace("'", "'\\''")
                f.write(f"file '{escaped_path}'\n")
            concat_file = f.name

        try:
            cmd = [
                self.ffmpeg_path,
                "-y",
                "-f", "concat",
                "-safe", "0",
                "-i", concat_file,
                "-c", "copy",
                output_path
            ]

            self._run_ffmpeg(cmd)
            return output_path

        finally:
            Path(concat_file).unlink(missing_ok=True)

    def _concat_with_transition(
        self,
        clip_paths: list[str],
        output_path: str,
        transition: str,
        duration: float
    ) -> str:
        """トランジション付き結合"""

        # フィルターグラフを構築
        inputs = []
        filter_parts = []

        for i, path in enumerate(clip_paths):
            inputs.extend(["-i", path])

        # 各クリップのストリームを準備
        n = len(clip_paths)

        if transition == "fade":
            # フェードイン/アウト
            for i in range(n):
                if i == 0:
                    filter_parts.append(f"[{i}:v]fade=t=out:st=4:d={duration}[v{i}]")
                elif i == n - 1:
                    filter_parts.append(f"[{i}:v]fade=t=in:st=0:d={duration}[v{i}]")
                else:
                    filter_parts.append(
                        f"[{i}:v]fade=t=in:st=0:d={duration},"
                        f"fade=t=out:st=4:d={duration}[v{i}]"
                    )

            # 結合
            stream_refs = "".join(f"[v{i}]" for i in range(n))
            filter_parts.append(f"{stream_refs}concat=n={n}:v=1:a=0[outv]")

        elif transition == "dissolve":
            # クロスディゾルブ
            if n == 2:
                filter_parts.append(
                    f"[0:v][1:v]xfade=transition=fade:duration={duration}:offset=4[outv]"
                )
            else:
                # 複数クリップの場合は順次適用
                prev = "0:v"
                for i in range(1, n):
                    out_label = "outv" if i == n - 1 else f"xf{i}"
                    offset = 4 * i - duration * (i - 1)
                    filter_parts.append(
                        f"[{prev}][{i}:v]xfade=transition=fade:"
                        f"duration={duration}:offset={offset}[{out_label}]"
                    )
                    prev = out_label
        else:
            # 不明なトランジションはシンプル結合にフォールバック
            return self._concat_simple(clip_paths, output_path)

        filter_complex = ";".join(filter_parts)

        cmd = [
            self.ffmpeg_path,
            "-y",
            *inputs,
            "-filter_complex", filter_complex,
            "-map", "[outv]",
            "-c:v", "libx264",
            "-preset", "medium",
            "-crf", "23",
            output_path
        ]

        self._run_ffmpeg(cmd)
        return output_path

    def _run_ffmpeg(self, cmd: list[str]):
        """FFmpegコマンドを実行"""
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True
        )

        if result.returncode != 0:
            raise RuntimeError(
                f"FFmpeg failed: {result.stderr}"
            )

    def get_video_duration(self, video_path: str) -> float:
        """ビデオの長さを取得（秒）"""
        cmd = [
            self.ffmpeg_path.replace("ffmpeg", "ffprobe"),
            "-v", "error",
            "-show_entries", "format=duration",
            "-of", "default=noprint_wrappers=1:nokey=1",
            video_path
        ]

        result = subprocess.run(cmd, capture_output=True, text=True)

        if result.returncode != 0:
            raise RuntimeError(f"Failed to get duration: {result.stderr}")

        return float(result.stdout.strip())

    def extract_last_frame(self, video_path: str, output_path: str) -> str:
        """ビデオの最後のフレームを抽出"""

        # まず長さを取得
        duration = self.get_video_duration(video_path)

        # 最後のフレーム位置（少し手前）
        seek_time = max(0, duration - 0.1)

        cmd = [
            self.ffmpeg_path,
            "-y",
            "-ss", str(seek_time),
            "-i", video_path,
            "-vframes", "1",
            "-q:v", "2",
            output_path
        ]

        self._run_ffmpeg(cmd)
        return output_path
