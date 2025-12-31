"""
Kalimba tab visualization widget.
Displays detected pitches as kalimba tablature.
"""

from typing import Optional, List, Dict
from PySide6.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QLabel, QComboBox
from PySide6.QtCore import Qt, Signal, QRectF
from PySide6.QtGui import QPainter, QPen, QColor, QBrush, QFont, QPainterPath

from ..core.audio_analyzer import PitchNote


class KalimbaWidget(QWidget):
    """Widget displaying kalimba tablature from detected pitches."""

    # Signal when view range changes
    view_range_changed = Signal(float, float)

    # Standard 17-key kalimba in C major tuning
    # Tines are arranged: left side (low) - center - right side (high)
    # Physical layout alternates left-right from center
    KALIMBA_17_C = {
        # MIDI note: (tine_number, position_from_center, note_name)
        # Center tine is position 0, left is negative, right is positive
        60: (1, 0, "C4"),      # Center - Middle C
        62: (2, 1, "D4"),      # Right 1
        64: (3, -1, "E4"),     # Left 1
        65: (4, 2, "F4"),      # Right 2
        67: (5, -2, "G4"),     # Left 2
        69: (6, 3, "A4"),      # Right 3
        71: (7, -3, "B4"),     # Left 3
        72: (8, 4, "C5"),      # Right 4
        74: (9, -4, "D5"),     # Left 4
        76: (10, 5, "E5"),     # Right 5
        77: (11, -5, "F5"),    # Left 5
        79: (12, 6, "G5"),     # Right 6
        81: (13, -6, "A5"),    # Left 6
        83: (14, 7, "B5"),     # Right 7
        84: (15, -7, "C6"),    # Left 7
        86: (16, 8, "D6"),     # Right 8
        88: (17, -8, "E6"),    # Left 8
    }

    # Also support lower octave notes by transposing
    KALIMBA_RANGE_MIN = 60  # C4
    KALIMBA_RANGE_MAX = 88  # E6

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumHeight(250)
        self.setMinimumWidth(400)

        # Data
        self.pitch_notes: List[PitchNote] = []
        self.duration: float = 0.0

        # View state
        self.view_start: float = 0.0
        self.view_end: float = 10.0
        self.playback_position: float = 0.0

        # Display options
        self.show_note_names = True
        self.transpose_octave = 0  # Octave adjustment for out-of-range notes

        # Colors
        self.bg_color = QColor(30, 30, 35)
        self.tine_color = QColor(180, 140, 100)  # Wood color
        self.tine_highlight = QColor(220, 180, 140)
        self.note_colors = [
            QColor(255, 100, 100),  # Red
            QColor(255, 180, 100),  # Orange
            QColor(255, 255, 100),  # Yellow
            QColor(100, 255, 100),  # Green
            QColor(100, 200, 255),  # Cyan
            QColor(150, 100, 255),  # Purple
            QColor(255, 100, 200),  # Pink
        ]
        self.playhead_color = QColor(255, 255, 255)
        self.grid_color = QColor(60, 60, 70)

    def set_pitch_notes(self, notes: List[PitchNote], duration: float):
        """Set the pitch notes to display."""
        self.pitch_notes = notes or []
        self.duration = duration
        self.update()

    def set_view_range(self, start: float, end: float):
        """Set the visible time range."""
        self.view_start = max(0, start)
        self.view_end = min(self.duration, end) if self.duration > 0 else end
        self.update()

    def set_playback_position(self, position: float):
        """Set current playback position."""
        self.playback_position = position
        self.update()

    def set_transpose(self, octaves: int):
        """Set octave transposition for out-of-range notes."""
        self.transpose_octave = octaves
        self.update()

    def _midi_to_tine(self, midi_note: int) -> Optional[tuple]:
        """Convert MIDI note to kalimba tine info.

        Returns (tine_number, position, note_name) or None if not playable.
        """
        # Apply transposition to fit kalimba range
        adjusted = midi_note
        while adjusted < self.KALIMBA_RANGE_MIN:
            adjusted += 12
        while adjusted > self.KALIMBA_RANGE_MAX:
            adjusted -= 12

        # Additional manual transpose
        adjusted += self.transpose_octave * 12
        while adjusted < self.KALIMBA_RANGE_MIN:
            adjusted += 12
        while adjusted > self.KALIMBA_RANGE_MAX:
            adjusted -= 12

        return self.KALIMBA_17_C.get(adjusted)

    def paintEvent(self, event):
        """Paint the kalimba tablature."""
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)

        width = self.width()
        height = self.height()

        # Background
        painter.fillRect(0, 0, width, height, self.bg_color)

        # Layout areas
        tine_area_height = 60  # Bottom area showing tines
        tab_area_height = height - tine_area_height
        time_range = self.view_end - self.view_start

        if time_range <= 0:
            return

        # Draw time grid
        self._draw_time_grid(painter, width, tab_area_height, time_range)

        # Draw tine lanes (vertical lines in tab area)
        tine_positions = self._get_tine_x_positions(width)
        self._draw_tine_lanes(painter, tine_positions, tab_area_height)

        # Draw notes
        self._draw_notes(painter, tine_positions, tab_area_height, time_range)

        # Draw kalimba tines at bottom
        self._draw_kalimba_tines(painter, tine_positions, tab_area_height, tine_area_height)

        # Draw playhead
        self._draw_playhead(painter, width, height, time_range)

    def _get_tine_x_positions(self, width: int) -> Dict[int, float]:
        """Get x positions for each tine number."""
        positions = {}
        margin = 30
        usable_width = width - 2 * margin

        # Sort tines by position (center = 0, then alternating)
        sorted_tines = sorted(self.KALIMBA_17_C.items(), key=lambda x: x[1][1])

        num_tines = len(sorted_tines)
        tine_spacing = usable_width / (num_tines - 1) if num_tines > 1 else usable_width

        for i, (midi, (tine_num, pos, name)) in enumerate(sorted_tines):
            x = margin + i * tine_spacing
            positions[tine_num] = x

        return positions

    def _draw_time_grid(self, painter: QPainter, width: int, height: int, time_range: float):
        """Draw time grid lines."""
        painter.setPen(QPen(self.grid_color, 1))

        # Calculate appropriate grid interval
        if time_range <= 2:
            interval = 0.25
        elif time_range <= 5:
            interval = 0.5
        elif time_range <= 15:
            interval = 1.0
        elif time_range <= 60:
            interval = 5.0
        else:
            interval = 10.0

        # Draw horizontal lines (time markers)
        t = (int(self.view_start / interval) + 1) * interval
        while t < self.view_end:
            y = (t - self.view_start) / time_range * height
            painter.drawLine(0, int(y), width, int(y))

            # Time label
            painter.drawText(5, int(y) - 2, f"{t:.1f}s")
            t += interval

    def _draw_tine_lanes(self, painter: QPainter, positions: Dict[int, float], height: int):
        """Draw vertical lane lines for each tine."""
        painter.setPen(QPen(self.grid_color, 1, Qt.DotLine))

        for tine_num, x in positions.items():
            painter.drawLine(int(x), 0, int(x), height)

    def _draw_notes(self, painter: QPainter, positions: Dict[int, float],
                    tab_height: int, time_range: float):
        """Draw detected notes on the tablature."""
        note_width = 24
        note_height = 20

        font = QFont("Arial", 9, QFont.Bold)
        painter.setFont(font)

        for note in self.pitch_notes:
            # Check if note is in view
            if note.time + note.duration < self.view_start or note.time > self.view_end:
                continue

            # Get tine for this note
            tine_info = self._midi_to_tine(note.midi_note)
            if not tine_info:
                continue

            tine_num, pos, note_name = tine_info
            x = positions.get(tine_num)
            if x is None:
                continue

            # Calculate y position (time flows downward)
            y = (note.time - self.view_start) / time_range * tab_height

            # Color based on note
            color_idx = note.midi_note % len(self.note_colors)
            color = self.note_colors[color_idx]

            # Adjust alpha based on confidence
            alpha = int(150 + note.confidence * 105)
            color.setAlpha(alpha)

            # Draw note circle/rectangle
            rect = QRectF(x - note_width / 2, y - note_height / 2, note_width, note_height)
            painter.setBrush(QBrush(color))
            painter.setPen(QPen(color.darker(120), 2))
            painter.drawRoundedRect(rect, 5, 5)

            # Draw note name
            painter.setPen(QPen(Qt.white))
            # Remove octave number for cleaner display
            display_name = note_name[:-1] if note_name[-1].isdigit() else note_name
            painter.drawText(rect, Qt.AlignCenter, display_name)

            # Draw duration line for longer notes
            if note.duration > 0.1:
                end_y = (note.time + note.duration - self.view_start) / time_range * tab_height
                if end_y > y + note_height / 2:
                    painter.setPen(QPen(color, 3))
                    painter.drawLine(int(x), int(y + note_height / 2), int(x), int(end_y))

    def _draw_kalimba_tines(self, painter: QPainter, positions: Dict[int, float],
                            tab_height: int, tine_height: int):
        """Draw the kalimba tine visualization at bottom."""
        y_start = tab_height
        margin = 5

        # Background for tine area
        painter.fillRect(0, y_start, self.width(), tine_height, QColor(40, 35, 30))

        # Draw each tine
        tine_width = 20
        sorted_tines = sorted(self.KALIMBA_17_C.items(), key=lambda x: x[1][1])

        for midi, (tine_num, pos, name) in sorted_tines:
            x = positions.get(tine_num)
            if x is None:
                continue

            # Tine length varies (center is longest)
            base_length = tine_height - 2 * margin
            length_factor = 1.0 - abs(pos) * 0.03
            tine_length = base_length * length_factor

            # Draw tine
            rect = QRectF(x - tine_width / 2, y_start + margin,
                         tine_width, tine_length)
            painter.setBrush(QBrush(self.tine_color))
            painter.setPen(QPen(self.tine_highlight, 1))
            painter.drawRoundedRect(rect, 3, 3)

            # Draw note name on tine
            painter.setPen(QPen(QColor(60, 40, 20)))
            font = QFont("Arial", 7, QFont.Bold)
            painter.setFont(font)
            display_name = name[:-1] if name[-1].isdigit() else name
            painter.drawText(rect, Qt.AlignCenter, display_name)

    def _draw_playhead(self, painter: QPainter, width: int, height: int, time_range: float):
        """Draw the playback position indicator."""
        if self.view_start <= self.playback_position <= self.view_end:
            y = (self.playback_position - self.view_start) / time_range * (height - 60)

            painter.setPen(QPen(self.playhead_color, 2))
            painter.drawLine(0, int(y), width, int(y))

            # Draw time indicator
            painter.drawText(width - 50, int(y) - 2, f"{self.playback_position:.2f}s")

    def wheelEvent(self, event):
        """Handle mouse wheel for zooming."""
        delta = event.angleDelta().y()

        if delta > 0:
            # Zoom in
            factor = 0.8
        else:
            # Zoom out
            factor = 1.25

        # Calculate new range centered on mouse position
        center = self.view_start + (self.view_end - self.view_start) / 2
        new_range = (self.view_end - self.view_start) * factor

        # Clamp range
        new_range = max(1.0, min(new_range, self.duration if self.duration > 0 else 300))

        new_start = center - new_range / 2
        new_end = center + new_range / 2

        # Adjust if out of bounds
        if new_start < 0:
            new_end -= new_start
            new_start = 0
        if new_end > self.duration and self.duration > 0:
            new_start -= (new_end - self.duration)
            new_end = self.duration

        self.view_start = max(0, new_start)
        self.view_end = new_end
        self.update()
        self.view_range_changed.emit(self.view_start, self.view_end)
