"""
オーディオ処理モジュール
ビデオへのBGM/効果音追加、AI音声生成
"""

import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Callable


@dataclass
class AudioTrack:
    """オーディオトラック情報"""
    path: str
    volume: float = 1.0  # 0.0 - 2.0
    start_time: float = 0.0  # 開始位置（秒）
    fade_in: float = 0.0  # フェードイン時間（秒）
    fade_out: float = 0.0  # フェードアウト時間（秒）
    loop: bool = False  # ループするか


@dataclass
class AudioConfig:
    """オーディオ設定"""
    background_music: AudioTrack | None = None
    sound_effects: list[tuple[AudioTrack, float]] | None = None  # (track, timestamp)
    voice_over: AudioTrack | None = None
    master_volume: float = 1.0


class AudioProcessor:
    """FFmpegを使用したオーディオ処理"""

    def __init__(self, ffmpeg_path: str = "ffmpeg"):
        self.ffmpeg_path = ffmpeg_path

    def add_audio_to_video(
        self,
        video_path: str,
        output_path: str,
        config: AudioConfig,
        on_progress: Callable[[float], None] | None = None
    ) -> str:
        """
        ビデオにオーディオを追加

        Args:
            video_path: 入力ビデオパス
            output_path: 出力ビデオパス
            config: オーディオ設定
            on_progress: 進捗コールバック

        Returns:
            出力ファイルパス
        """
        if on_progress:
            on_progress(0.1)

        # ビデオの長さを取得
        video_duration = self._get_duration(video_path)

        # フィルターとインプットを構築
        inputs = ["-i", video_path]
        filter_parts = []
        audio_streams = []
        input_index = 1

        # BGM
        if config.background_music:
            bgm = config.background_music
            inputs.extend(["-i", bgm.path])

            # BGMフィルター構築
            bgm_filter = f"[{input_index}:a]"

            # ループ処理
            if bgm.loop:
                bgm_filter += f"aloop=loop=-1:size=2e+09,"

            # トリム＆パッド
            bgm_filter += f"atrim=0:{video_duration},apad=whole_dur={video_duration},"

            # 音量調整
            bgm_filter += f"volume={bgm.volume * config.master_volume}"

            # フェード
            if bgm.fade_in > 0:
                bgm_filter += f",afade=t=in:st=0:d={bgm.fade_in}"
            if bgm.fade_out > 0:
                fade_start = video_duration - bgm.fade_out
                bgm_filter += f",afade=t=out:st={fade_start}:d={bgm.fade_out}"

            bgm_filter += "[bgm]"
            filter_parts.append(bgm_filter)
            audio_streams.append("[bgm]")
            input_index += 1

        if on_progress:
            on_progress(0.3)

        # 効果音
        if config.sound_effects:
            for i, (sfx, timestamp) in enumerate(config.sound_effects):
                inputs.extend(["-i", sfx.path])

                # 効果音フィルター
                sfx_filter = f"[{input_index}:a]"
                sfx_filter += f"volume={sfx.volume * config.master_volume},"
                sfx_filter += f"adelay={int(timestamp * 1000)}|{int(timestamp * 1000)}"
                sfx_filter += f"[sfx{i}]"

                filter_parts.append(sfx_filter)
                audio_streams.append(f"[sfx{i}]")
                input_index += 1

        if on_progress:
            on_progress(0.5)

        # ナレーション
        if config.voice_over:
            vo = config.voice_over
            inputs.extend(["-i", vo.path])

            vo_filter = f"[{input_index}:a]"
            vo_filter += f"volume={vo.volume * config.master_volume}"

            if vo.start_time > 0:
                vo_filter += f",adelay={int(vo.start_time * 1000)}|{int(vo.start_time * 1000)}"

            vo_filter += "[vo]"
            filter_parts.append(vo_filter)
            audio_streams.append("[vo]")
            input_index += 1

        if on_progress:
            on_progress(0.6)

        # オーディオがない場合はビデオをコピー
        if not audio_streams:
            self._copy_video(video_path, output_path)
            return output_path

        # ミックス
        if len(audio_streams) == 1:
            # 1トラックのみ
            mix_filter = f"{audio_streams[0]}anull[aout]"
        else:
            # 複数トラックをミックス
            streams_str = "".join(audio_streams)
            mix_filter = f"{streams_str}amix=inputs={len(audio_streams)}:duration=first:dropout_transition=2[aout]"

        filter_parts.append(mix_filter)
        filter_complex = ";".join(filter_parts)

        if on_progress:
            on_progress(0.7)

        # FFmpegコマンド構築
        cmd = [
            self.ffmpeg_path,
            "-y",
            *inputs,
            "-filter_complex", filter_complex,
            "-map", "0:v",
            "-map", "[aout]",
            "-c:v", "copy",
            "-c:a", "aac",
            "-b:a", "192k",
            output_path
        ]

        result = subprocess.run(cmd, capture_output=True, text=True)

        if result.returncode != 0:
            raise RuntimeError(f"FFmpeg failed: {result.stderr}")

        if on_progress:
            on_progress(1.0)

        return output_path

    def add_simple_bgm(
        self,
        video_path: str,
        audio_path: str,
        output_path: str,
        volume: float = 0.3,
        fade_out: float = 2.0
    ) -> str:
        """
        シンプルにBGMを追加（ヘルパーメソッド）

        Args:
            video_path: 入力ビデオ
            audio_path: BGMファイル
            output_path: 出力パス
            volume: BGM音量（0.0-1.0）
            fade_out: 最後のフェードアウト時間

        Returns:
            出力ファイルパス
        """
        config = AudioConfig(
            background_music=AudioTrack(
                path=audio_path,
                volume=volume,
                loop=True,
                fade_out=fade_out
            )
        )
        return self.add_audio_to_video(video_path, output_path, config)

    def generate_silence(self, duration: float, output_path: str) -> str:
        """無音オーディオを生成"""
        cmd = [
            self.ffmpeg_path,
            "-y",
            "-f", "lavfi",
            "-i", f"anullsrc=r=44100:cl=stereo:d={duration}",
            "-c:a", "aac",
            output_path
        ]

        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            raise RuntimeError(f"FFmpeg failed: {result.stderr}")

        return output_path

    def extract_audio(self, video_path: str, output_path: str) -> str:
        """ビデオからオーディオを抽出"""
        cmd = [
            self.ffmpeg_path,
            "-y",
            "-i", video_path,
            "-vn",
            "-c:a", "aac",
            output_path
        ]

        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            raise RuntimeError(f"FFmpeg failed: {result.stderr}")

        return output_path

    def _get_duration(self, path: str) -> float:
        """メディアファイルの長さを取得"""
        cmd = [
            self.ffmpeg_path.replace("ffmpeg", "ffprobe"),
            "-v", "error",
            "-show_entries", "format=duration",
            "-of", "default=noprint_wrappers=1:nokey=1",
            path
        ]

        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            raise RuntimeError(f"ffprobe failed: {result.stderr}")

        return float(result.stdout.strip())

    def _copy_video(self, input_path: str, output_path: str):
        """ビデオをコピー"""
        cmd = [
            self.ffmpeg_path,
            "-y",
            "-i", input_path,
            "-c", "copy",
            output_path
        ]
        subprocess.run(cmd, capture_output=True, check=True)


class AIAudioGenerator:
    """
    AI音声/音楽生成
    外部APIを使用した音声合成
    """

    def __init__(self, provider: str = "elevenlabs", api_key: str = ""):
        self.provider = provider
        self.api_key = api_key

    def generate_voice(
        self,
        text: str,
        output_path: str,
        voice_id: str = "default",
        speed: float = 1.0
    ) -> str:
        """
        テキストから音声を生成

        Args:
            text: 読み上げテキスト
            output_path: 出力パス
            voice_id: 音声ID
            speed: 読み上げ速度

        Returns:
            出力ファイルパス
        """
        if self.provider == "elevenlabs":
            return self._generate_elevenlabs(text, output_path, voice_id)
        elif self.provider == "openai":
            return self._generate_openai_tts(text, output_path, voice_id, speed)
        else:
            raise ValueError(f"Unknown provider: {self.provider}")

    def _generate_elevenlabs(
        self,
        text: str,
        output_path: str,
        voice_id: str
    ) -> str:
        """ElevenLabs APIで音声生成"""
        try:
            from elevenlabs import generate, save, set_api_key

            set_api_key(self.api_key)

            audio = generate(
                text=text,
                voice=voice_id,
                model="eleven_multilingual_v2"
            )

            save(audio, output_path)
            return output_path

        except ImportError:
            raise ImportError(
                "elevenlabs package not installed. "
                "Run: pip install elevenlabs"
            )

    def _generate_openai_tts(
        self,
        text: str,
        output_path: str,
        voice_id: str,
        speed: float
    ) -> str:
        """OpenAI TTS APIで音声生成"""
        try:
            from openai import OpenAI

            client = OpenAI(api_key=self.api_key)

            response = client.audio.speech.create(
                model="tts-1",
                voice=voice_id if voice_id != "default" else "alloy",
                input=text,
                speed=speed
            )

            response.stream_to_file(output_path)
            return output_path

        except ImportError:
            raise ImportError(
                "openai package not installed. "
                "Run: pip install openai"
            )

    def generate_music(
        self,
        prompt: str,
        output_path: str,
        duration: int = 30
    ) -> str:
        """
        プロンプトから音楽を生成

        Note: 現在はReplicate経由でMusicGenを使用
        """
        try:
            import replicate

            output = replicate.run(
                "meta/musicgen:b05b1dff1d8c6dc63d14b0cdb42135378dcb87f6373b0d3d341ede46e59e2b38",
                input={
                    "prompt": prompt,
                    "duration": duration,
                    "model_version": "stereo-melody-large"
                }
            )

            # ダウンロード
            import urllib.request
            urllib.request.urlretrieve(output, output_path)

            return output_path

        except ImportError:
            raise ImportError(
                "replicate package not installed. "
                "Run: pip install replicate"
            )
