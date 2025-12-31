"""
Audio analysis module for rhythm game note generation.
Handles beat detection, onset detection, and energy analysis.
"""

import numpy as np
import librosa
from dataclasses import dataclass
from typing import Optional, Tuple, List
from pathlib import Path


@dataclass
class PitchNote:
    """A detected pitch/note from audio."""
    time: float       # Start time in seconds
    duration: float   # Duration in seconds
    frequency: float  # Frequency in Hz
    midi_note: int    # MIDI note number (60 = C4)
    note_name: str    # Note name like "C4", "D#5"
    confidence: float # Detection confidence (0-1)


@dataclass
class AudioAnalysisResult:
    """Result of audio analysis."""
    # Basic info
    duration: float  # seconds
    sample_rate: int
    tempo: float  # BPM

    # Beat and onset times (in seconds)
    beat_times: np.ndarray
    onset_times: np.ndarray

    # Energy envelope
    times: np.ndarray
    energy: np.ndarray

    # Raw audio data for waveform display
    waveform: np.ndarray
    waveform_sr: int

    # Pitch detection results (optional)
    pitch_notes: Optional[list] = None  # List of PitchNote


class AudioAnalyzer:
    """Analyzes audio files for rhythm game note generation."""

    def __init__(self, hop_length: int = 512):
        self.hop_length = hop_length

    def load_audio(self, file_path: str | Path) -> Tuple[np.ndarray, int]:
        """Load audio file and return waveform and sample rate."""
        y, sr = librosa.load(str(file_path), sr=None, mono=True)
        return y, sr

    def analyze(self, file_path: str | Path,
                progress_callback: Optional[callable] = None) -> AudioAnalysisResult:
        """Perform full analysis on an audio file.

        Args:
            file_path: Path to audio file
            progress_callback: Optional callback(percent, message) for progress updates
        """
        if progress_callback:
            progress_callback(5, "Loading audio file...")

        # Load audio
        y, sr = self.load_audio(file_path)
        duration = librosa.get_duration(y=y, sr=sr)

        if progress_callback:
            progress_callback(20, "Detecting tempo and beats...")

        # Detect tempo and beats
        tempo, beat_frames = librosa.beat.beat_track(y=y, sr=sr, hop_length=self.hop_length)
        beat_times = librosa.frames_to_time(beat_frames, sr=sr, hop_length=self.hop_length)

        # Handle tempo as array (newer librosa versions)
        if isinstance(tempo, np.ndarray):
            tempo = float(tempo[0]) if len(tempo) > 0 else 120.0

        if progress_callback:
            progress_callback(45, "Detecting onsets...")

        # Detect onsets
        onset_frames = librosa.onset.onset_detect(
            y=y, sr=sr, hop_length=self.hop_length,
            backtrack=True
        )
        onset_times = librosa.frames_to_time(onset_frames, sr=sr, hop_length=self.hop_length)

        if progress_callback:
            progress_callback(65, "Calculating energy envelope...")

        # Calculate energy envelope (RMS)
        rms = librosa.feature.rms(y=y, hop_length=self.hop_length)[0]
        times = librosa.frames_to_time(np.arange(len(rms)), sr=sr, hop_length=self.hop_length)

        # Normalize energy
        if rms.max() > 0:
            energy = rms / rms.max()
        else:
            energy = rms

        if progress_callback:
            progress_callback(80, "Preparing waveform display...")

        # Downsample waveform for display (keep it manageable)
        display_sr = min(sr, 22050)
        if sr != display_sr:
            waveform = librosa.resample(y, orig_sr=sr, target_sr=display_sr)
        else:
            waveform = y

        if progress_callback:
            progress_callback(95, "Finalizing analysis...")

        result = AudioAnalysisResult(
            duration=duration,
            sample_rate=sr,
            tempo=tempo,
            beat_times=beat_times,
            onset_times=onset_times,
            times=times,
            energy=energy,
            waveform=waveform,
            waveform_sr=display_sr
        )

        if progress_callback:
            progress_callback(100, "Complete!")

        return result

    def get_onset_strengths(self, file_path: str | Path) -> Tuple[np.ndarray, np.ndarray]:
        """Get onset strength envelope for more detailed analysis."""
        y, sr = self.load_audio(file_path)
        onset_env = librosa.onset.onset_strength(y=y, sr=sr, hop_length=self.hop_length)
        times = librosa.frames_to_time(np.arange(len(onset_env)), sr=sr, hop_length=self.hop_length)
        return times, onset_env

    def get_spectral_features(self, file_path: str | Path) -> dict:
        """Extract spectral features for advanced note placement."""
        y, sr = self.load_audio(file_path)

        # Spectral centroid (brightness)
        centroid = librosa.feature.spectral_centroid(y=y, sr=sr, hop_length=self.hop_length)[0]

        # Spectral bandwidth
        bandwidth = librosa.feature.spectral_bandwidth(y=y, sr=sr, hop_length=self.hop_length)[0]

        # Mel-frequency bands for lane assignment
        mel_spec = librosa.feature.melspectrogram(y=y, sr=sr, hop_length=self.hop_length, n_mels=6)

        times = librosa.frames_to_time(np.arange(mel_spec.shape[1]), sr=sr, hop_length=self.hop_length)

        return {
            'times': times,
            'centroid': centroid,
            'bandwidth': bandwidth,
            'mel_bands': mel_spec  # 6 bands for 6-key mapping
        }

    def detect_pitches(self, file_path: str | Path,
                       fmin: float = 65.0,  # C2
                       fmax: float = 2100.0,  # C7
                       min_duration: float = 0.05,
                       progress_callback: Optional[callable] = None) -> List[PitchNote]:
        """Detect pitches/notes from audio using pyin algorithm.

        Args:
            file_path: Path to audio file
            fmin: Minimum frequency to detect (Hz)
            fmax: Maximum frequency to detect (Hz)
            min_duration: Minimum note duration in seconds
            progress_callback: Optional callback(percent, message) for progress updates

        Returns:
            List of PitchNote objects
        """
        if progress_callback:
            progress_callback(5, "Loading audio file...")

        y, sr = self.load_audio(file_path)

        if progress_callback:
            progress_callback(15, "Running pitch detection algorithm...")

        # Use pyin for pitch detection (better for monophonic audio)
        f0, voiced_flag, voiced_probs = librosa.pyin(
            y, fmin=fmin, fmax=fmax, sr=sr, hop_length=self.hop_length
        )

        if progress_callback:
            progress_callback(70, "Processing detected pitches...")

        times = librosa.frames_to_time(np.arange(len(f0)), sr=sr, hop_length=self.hop_length)

        # Convert to notes by grouping consecutive similar pitches
        notes = []
        current_note = None
        current_start = None
        current_freqs = []
        current_probs = []

        for i, (time, freq, voiced, prob) in enumerate(zip(times, f0, voiced_flag, voiced_probs)):
            if voiced and not np.isnan(freq):
                midi = librosa.hz_to_midi(freq)

                if current_note is None:
                    # Start new note
                    current_note = round(midi)
                    current_start = time
                    current_freqs = [freq]
                    current_probs = [prob]
                elif abs(round(midi) - current_note) <= 0.5:
                    # Continue same note
                    current_freqs.append(freq)
                    current_probs.append(prob)
                else:
                    # Different note - save current and start new
                    duration = time - current_start
                    if duration >= min_duration:
                        notes.append(self._create_pitch_note(
                            current_start, duration, current_freqs, current_probs
                        ))
                    current_note = round(midi)
                    current_start = time
                    current_freqs = [freq]
                    current_probs = [prob]
            else:
                # Unvoiced - end current note if exists
                if current_note is not None:
                    duration = time - current_start
                    if duration >= min_duration:
                        notes.append(self._create_pitch_note(
                            current_start, duration, current_freqs, current_probs
                        ))
                    current_note = None
                    current_start = None
                    current_freqs = []
                    current_probs = []

        # Handle last note
        if current_note is not None:
            duration = times[-1] - current_start
            if duration >= min_duration:
                notes.append(self._create_pitch_note(
                    current_start, duration, current_freqs, current_probs
                ))

        if progress_callback:
            progress_callback(95, f"Found {len(notes)} notes, finalizing...")

        if progress_callback:
            progress_callback(100, "Complete!")

        return notes

    def _create_pitch_note(self, start_time: float, duration: float,
                           frequencies: List[float], probabilities: List[float]) -> PitchNote:
        """Create a PitchNote from accumulated data."""
        avg_freq = np.median(frequencies)  # Use median for stability
        avg_prob = np.mean(probabilities)
        midi = round(librosa.hz_to_midi(avg_freq))
        note_name = librosa.midi_to_note(midi)

        return PitchNote(
            time=start_time,
            duration=duration,
            frequency=avg_freq,
            midi_note=midi,
            note_name=note_name,
            confidence=avg_prob
        )

    def analyze_with_pitch(self, file_path: str | Path) -> AudioAnalysisResult:
        """Perform full analysis including pitch detection."""
        result = self.analyze(file_path)
        result.pitch_notes = self.detect_pitches(file_path)
        return result
