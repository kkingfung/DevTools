"""
Note generation module for rhythm game.
Generates playable note patterns from audio analysis.
"""

import numpy as np
from dataclasses import dataclass, field
from typing import List, Optional
from enum import IntEnum
from .audio_analyzer import AudioAnalysisResult


class Difficulty(IntEnum):
    """Difficulty levels affecting note density and complexity."""
    EASY = 1
    NORMAL = 2
    HARD = 3
    EXPERT = 4
    MASTER = 5


@dataclass
class Note:
    """A single note in the rhythm game."""
    time: float      # Time in seconds
    lane: int        # Lane number (0-indexed, 0 to num_keys-1)
    duration: float  # Duration for hold notes (0 for tap notes)

    def to_dict(self) -> dict:
        """Convert to dictionary for serialization."""
        return {
            'time': float(round(self.time, 4)),
            'lane': int(self.lane),
            'duration': float(round(self.duration, 4))
        }


@dataclass
class NoteChart:
    """A complete note chart for a song."""
    # Metadata
    title: str = ""
    artist: str = ""
    audio_file: str = ""

    # Timing info
    bpm: float = 120.0
    offset: float = 0.0  # Start offset in seconds
    duration: float = 0.0

    # Chart info
    num_keys: int = 4
    difficulty: Difficulty = Difficulty.NORMAL
    difficulty_value: int = 1  # Numeric difficulty rating

    # Notes
    notes: List[Note] = field(default_factory=list)

    def to_dict(self) -> dict:
        """Convert to dictionary for serialization."""
        return {
            'metadata': {
                'title': str(self.title),
                'artist': str(self.artist),
                'audio_file': str(self.audio_file),
            },
            'timing': {
                'bpm': float(round(self.bpm, 2)),
                'offset': float(round(self.offset, 4)),
                'duration': float(round(self.duration, 2)),
            },
            'chart': {
                'num_keys': int(self.num_keys),
                'difficulty': str(self.difficulty.name),
                'difficulty_value': int(self.difficulty_value),
            },
            'notes': [note.to_dict() for note in self.notes]
        }


class NoteGenerator:
    """Generates note charts from audio analysis."""

    # Difficulty parameters
    # onset_threshold: Higher = keeps only strong notes (fewer notes)
    # beat_weight: How much to prioritize beat-aligned notes (0-1)
    DIFFICULTY_PARAMS = {
        Difficulty.EASY: {
            'onset_threshold': 0.6,      # Only strong onsets
            'min_interval': 0.5,         # Minimum seconds between notes
            'max_simultaneous': 1,       # Max notes at same time
            'hold_probability': 0.0,     # Probability of hold notes
            'pattern_complexity': 0.2,   # Lane variation
            'beat_weight': 1.0,          # Heavily prioritize beats
            'use_beats_only': True,      # Only use beat times for notes
        },
        Difficulty.NORMAL: {
            'onset_threshold': 0.4,
            'min_interval': 0.3,
            'max_simultaneous': 2,
            'hold_probability': 0.1,
            'pattern_complexity': 0.4,
            'beat_weight': 0.8,
            'use_beats_only': False,
        },
        Difficulty.HARD: {
            'onset_threshold': 0.25,
            'min_interval': 0.2,
            'max_simultaneous': 2,
            'hold_probability': 0.2,
            'pattern_complexity': 0.6,
            'beat_weight': 0.6,
            'use_beats_only': False,
        },
        Difficulty.EXPERT: {
            'onset_threshold': 0.15,
            'min_interval': 0.1,
            'max_simultaneous': 3,
            'hold_probability': 0.3,
            'pattern_complexity': 0.8,
            'beat_weight': 0.4,
            'use_beats_only': False,
        },
        Difficulty.MASTER: {
            'onset_threshold': 0.1,
            'min_interval': 0.05,
            'max_simultaneous': 4,
            'hold_probability': 0.4,
            'pattern_complexity': 1.0,
            'beat_weight': 0.2,
            'use_beats_only': False,
        },
    }

    def __init__(self, num_keys: int = 4, difficulty: Difficulty = Difficulty.NORMAL):
        self.num_keys = min(max(num_keys, 1), 6)  # Clamp to 1-6
        self.difficulty = difficulty
        self.params = self.DIFFICULTY_PARAMS[difficulty]

    def generate(self, analysis: AudioAnalysisResult,
                 title: str = "", artist: str = "", audio_file: str = "") -> NoteChart:
        """Generate a note chart from audio analysis."""
        # Filter onsets based on difficulty
        onset_times = self._filter_onsets(analysis)

        # Generate notes with lane assignments
        notes = self._assign_lanes(onset_times, analysis)

        # Add hold notes based on difficulty
        notes = self._add_hold_notes(notes, analysis)

        # Calculate difficulty value (1-10 scale)
        difficulty_value = self._calculate_difficulty_value(notes, analysis.duration)

        return NoteChart(
            title=title,
            artist=artist,
            audio_file=audio_file,
            bpm=analysis.tempo,
            offset=0.0,
            duration=analysis.duration,
            num_keys=self.num_keys,
            difficulty=self.difficulty,
            difficulty_value=difficulty_value,
            notes=notes
        )

    def _filter_onsets(self, analysis: AudioAnalysisResult) -> np.ndarray:
        """Filter onset times based on difficulty threshold, beats, and minimum interval."""
        use_beats_only = self.params.get('use_beats_only', False)
        beat_weight = self.params.get('beat_weight', 0.5)
        threshold = self.params['onset_threshold']
        min_interval = self.params['min_interval']

        # For easy difficulty, primarily use beat times
        if use_beats_only:
            # Use beat times as the primary note positions
            note_times = analysis.beat_times.copy()
        else:
            # Combine beat times and onset times with beat priority
            onset_times = analysis.onset_times.copy()
            beat_times = analysis.beat_times.copy()

            # Get onset strengths at each onset time
            energy_at_onsets = np.interp(onset_times, analysis.times, analysis.energy)

            # Filter by threshold (higher threshold = fewer notes kept)
            if len(energy_at_onsets) > 0 and energy_at_onsets.max() > 0:
                mask = energy_at_onsets >= threshold * energy_at_onsets.max()
                onset_times = onset_times[mask]

            # Merge beat times and onset times
            # Prioritize beats by snapping nearby onsets to beat positions
            beat_tolerance = 60.0 / analysis.tempo * 0.25  # Within 1/4 beat

            merged_times = []

            # Add all beats first (they're the rhythm foundation)
            for bt in beat_times:
                merged_times.append(bt)

            # Add onsets that aren't too close to existing beats
            for ot in onset_times:
                is_near_beat = any(abs(ot - bt) < beat_tolerance for bt in beat_times)
                if not is_near_beat:
                    # Only add non-beat onsets based on beat_weight
                    # Lower beat_weight = more non-beat onsets allowed
                    if np.random.random() > beat_weight:
                        merged_times.append(ot)

            note_times = np.array(sorted(set(merged_times)))

        # Filter by minimum interval
        if len(note_times) > 0:
            filtered = [note_times[0]]
            for t in note_times[1:]:
                if t - filtered[-1] >= min_interval:
                    filtered.append(t)
            note_times = np.array(filtered)

        return note_times

    def _assign_lanes(self, onset_times: np.ndarray,
                      analysis: AudioAnalysisResult) -> List[Note]:
        """Assign lanes to notes based on spectral content and patterns."""
        notes = []
        if len(onset_times) == 0:
            return notes

        complexity = self.params['pattern_complexity']
        max_simultaneous = self.params['max_simultaneous']

        # Track recent lanes to create playable patterns
        recent_lanes = []

        for i, time in enumerate(onset_times):
            # Determine number of simultaneous notes
            num_notes = 1
            if max_simultaneous > 1 and np.random.random() < complexity * 0.3:
                num_notes = min(np.random.randint(1, max_simultaneous + 1), self.num_keys)

            # Choose lanes
            available_lanes = list(range(self.num_keys))
            chosen_lanes = []

            for _ in range(num_notes):
                if not available_lanes:
                    break

                # Prefer lanes that create smooth patterns
                if recent_lanes and np.random.random() > complexity:
                    # Stay close to recent lanes
                    last_lane = recent_lanes[-1] if recent_lanes else self.num_keys // 2
                    weights = [1.0 / (1 + abs(l - last_lane)) for l in available_lanes]
                else:
                    # Random distribution
                    weights = [1.0] * len(available_lanes)

                weights = np.array(weights) / sum(weights)
                lane = np.random.choice(available_lanes, p=weights)
                chosen_lanes.append(lane)
                available_lanes.remove(lane)

            # Create notes (convert numpy types to Python native types)
            for lane in chosen_lanes:
                notes.append(Note(time=float(time), lane=int(lane), duration=0.0))

            # Update recent lanes
            recent_lanes.extend(chosen_lanes)
            if len(recent_lanes) > 4:
                recent_lanes = recent_lanes[-4:]

        return notes

    def _add_hold_notes(self, notes: List[Note],
                        analysis: AudioAnalysisResult) -> List[Note]:
        """Convert some tap notes to hold notes based on difficulty."""
        hold_prob = self.params['hold_probability']
        if hold_prob == 0:
            return notes

        # Sort notes by time
        notes = sorted(notes, key=lambda n: (n.time, n.lane))

        for i, note in enumerate(notes):
            if np.random.random() < hold_prob:
                # Find next note in same lane or use beat interval
                min_duration = 0.2
                max_duration = 2.0

                # Look for next note in same lane
                next_time = None
                for j in range(i + 1, len(notes)):
                    if notes[j].lane == note.lane:
                        next_time = notes[j].time
                        break

                if next_time and next_time - note.time > min_duration:
                    # Make hold note end before next note
                    note.duration = min((next_time - note.time) * 0.8, max_duration)
                else:
                    # Use beat-based duration
                    beat_duration = 60.0 / analysis.tempo
                    note.duration = min(beat_duration * np.random.randint(1, 4), max_duration)

        return notes

    def _calculate_difficulty_value(self, notes: List[Note], duration: float) -> int:
        """Calculate a 1-10 difficulty rating based on note density and patterns."""
        if duration == 0 or len(notes) == 0:
            return 1

        # Notes per second
        nps = len(notes) / duration

        # Base difficulty from NPS
        # 1 NPS = easy, 10 NPS = very hard
        base_diff = min(max(int(nps * 1.5), 1), 10)

        # Adjust for hold notes
        hold_count = sum(1 for n in notes if n.duration > 0)
        hold_factor = 1 + (hold_count / len(notes)) * 0.5

        # Adjust for simultaneous notes
        times = [n.time for n in notes]
        unique_times = len(set(round(t, 3) for t in times))
        simultaneous_factor = 1 + (len(notes) - unique_times) / len(notes) * 0.5

        final_diff = int(base_diff * hold_factor * simultaneous_factor)
        return min(max(final_diff, 1), 10)

    def regenerate_section(self, notes: List[Note],
                           start_time: float, end_time: float,
                           analysis: AudioAnalysisResult) -> List[Note]:
        """Regenerate notes in a specific time range."""
        # Keep notes outside the range
        kept_notes = [n for n in notes if n.time < start_time or n.time >= end_time]

        # Get onsets in the range
        mask = (analysis.onset_times >= start_time) & (analysis.onset_times < end_time)
        section_onsets = analysis.onset_times[mask]

        # Filter and generate new notes
        section_onsets = self._filter_onsets_array(section_onsets, analysis)
        new_notes = self._assign_lanes(section_onsets, analysis)
        new_notes = self._add_hold_notes(new_notes, analysis)

        # Combine and sort
        all_notes = kept_notes + new_notes
        all_notes.sort(key=lambda n: (n.time, n.lane))

        return all_notes

    def _filter_onsets_array(self, onsets: np.ndarray,
                             analysis: AudioAnalysisResult) -> np.ndarray:
        """Filter an array of onset times with beat awareness."""
        if len(onsets) == 0:
            return onsets

        min_interval = self.params['min_interval']
        beat_weight = self.params.get('beat_weight', 0.5)
        beat_times = analysis.beat_times

        # Get beats within the onset range
        if len(onsets) > 0:
            range_beats = beat_times[(beat_times >= onsets.min()) & (beat_times <= onsets.max())]
        else:
            range_beats = np.array([])

        # Merge with beats for better rhythm
        beat_tolerance = 60.0 / analysis.tempo * 0.25
        merged = list(range_beats)

        for ot in onsets:
            is_near_beat = any(abs(ot - bt) < beat_tolerance for bt in range_beats)
            if not is_near_beat and np.random.random() > beat_weight:
                merged.append(ot)

        merged = sorted(set(merged))

        # Filter by minimum interval
        if len(merged) > 0:
            filtered = [merged[0]]
            for t in merged[1:]:
                if t - filtered[-1] >= min_interval:
                    filtered.append(t)
            return np.array(filtered)

        return np.array(merged)
