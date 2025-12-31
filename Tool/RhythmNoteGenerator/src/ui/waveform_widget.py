"""
Waveform visualization widget for audio display.
"""

import numpy as np
from PySide6.QtWidgets import QWidget
from PySide6.QtCore import Qt, Signal, QRectF
from PySide6.QtGui import QPainter, QPen, QColor, QBrush, QPainterPath
from typing import Optional

from ..core.audio_analyzer import AudioAnalysisResult
from ..core.note_generator import NoteChart


class WaveformWidget(QWidget):
    """Widget displaying audio waveform with notes overlay."""

    # Signal emitted when user clicks on timeline (time in seconds)
    time_clicked = Signal(float)
    # Signal for playback position updates
    position_changed = Signal(float)
    # Signal when view range changes (for syncing with other widgets)
    view_range_changed = Signal(float, float)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumHeight(150)
        self.setMouseTracking(True)

        # Data
        self.analysis: Optional[AudioAnalysisResult] = None
        self.chart: Optional[NoteChart] = None

        # View state
        self.view_start = 0.0   # Start time of visible area
        self.view_end = 10.0    # End time of visible area
        self.playback_position = 0.0

        # Cached waveform path
        self._waveform_cache = None
        self._cache_bounds = None

        # Colors
        self.bg_color = QColor(30, 30, 35)
        self.waveform_color = QColor(80, 150, 255)
        self.beat_color = QColor(255, 200, 100, 100)
        self.note_colors = [
            QColor(255, 100, 100),  # Lane 0 - Red
            QColor(100, 255, 100),  # Lane 1 - Green
            QColor(100, 100, 255),  # Lane 2 - Blue
            QColor(255, 255, 100),  # Lane 3 - Yellow
            QColor(255, 100, 255),  # Lane 4 - Magenta
            QColor(100, 255, 255),  # Lane 5 - Cyan
        ]
        self.playhead_color = QColor(255, 255, 255)
        self.grid_color = QColor(60, 60, 70)

    def set_analysis(self, analysis: AudioAnalysisResult):
        """Set audio analysis data."""
        self.analysis = analysis
        self.view_start = 0.0
        self.view_end = min(10.0, analysis.duration)
        self._waveform_cache = None
        self.update()

    def set_chart(self, chart: NoteChart):
        """Set note chart data."""
        self.chart = chart
        self.update()

    def set_view_range(self, start: float, end: float):
        """Set the visible time range."""
        if self.analysis:
            start = max(0, start)
            end = min(self.analysis.duration, end)
            if end > start:
                self.view_start = start
                self.view_end = end
                self._waveform_cache = None
                self.update()

    def set_playback_position(self, position: float):
        """Set current playback position."""
        self.playback_position = position
        self.update()

    def zoom(self, factor: float, center: Optional[float] = None):
        """Zoom in/out centered on a time position."""
        if not self.analysis:
            return

        if center is None:
            center = (self.view_start + self.view_end) / 2

        current_width = self.view_end - self.view_start
        new_width = current_width / factor
        new_width = max(0.5, min(self.analysis.duration, new_width))

        # Calculate new bounds centered on the zoom point
        ratio = (center - self.view_start) / current_width if current_width > 0 else 0.5
        new_start = center - new_width * ratio
        new_end = new_start + new_width

        # Clamp to valid range
        if new_start < 0:
            new_start = 0
            new_end = new_width
        if new_end > self.analysis.duration:
            new_end = self.analysis.duration
            new_start = new_end - new_width

        self.set_view_range(new_start, new_end)

    def time_to_x(self, time: float) -> float:
        """Convert time to x coordinate."""
        if self.view_end == self.view_start:
            return 0
        ratio = (time - self.view_start) / (self.view_end - self.view_start)
        return ratio * self.width()

    def x_to_time(self, x: float) -> float:
        """Convert x coordinate to time."""
        ratio = x / self.width() if self.width() > 0 else 0
        return self.view_start + ratio * (self.view_end - self.view_start)

    def paintEvent(self, event):
        """Draw the waveform and notes."""
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)

        # Background
        painter.fillRect(self.rect(), self.bg_color)

        if not self.analysis:
            painter.setPen(QColor(100, 100, 100))
            painter.drawText(self.rect(), Qt.AlignCenter, "Load an audio file to see waveform")
            return

        # Draw grid
        self._draw_grid(painter)

        # Draw waveform
        self._draw_waveform(painter)

        # Draw beat markers
        self._draw_beats(painter)

        # Draw notes
        if self.chart:
            self._draw_notes(painter)

        # Draw playhead
        self._draw_playhead(painter)

        # Draw time labels
        self._draw_time_labels(painter)

    def _draw_grid(self, painter: QPainter):
        """Draw time grid."""
        painter.setPen(QPen(self.grid_color, 1))

        # Calculate appropriate grid interval
        view_width = self.view_end - self.view_start
        if view_width < 1:
            interval = 0.1
        elif view_width < 5:
            interval = 0.5
        elif view_width < 20:
            interval = 1.0
        elif view_width < 60:
            interval = 5.0
        else:
            interval = 10.0

        t = (int(self.view_start / interval) + 1) * interval
        while t < self.view_end:
            x = self.time_to_x(t)
            painter.drawLine(int(x), 0, int(x), self.height())
            t += interval

    def _draw_waveform(self, painter: QPainter):
        """Draw audio waveform."""
        if self.analysis.waveform is None or len(self.analysis.waveform) == 0:
            return

        # Calculate sample range for visible area
        sr = self.analysis.waveform_sr
        start_sample = int(self.view_start * sr)
        end_sample = int(self.view_end * sr)

        start_sample = max(0, start_sample)
        end_sample = min(len(self.analysis.waveform), end_sample)

        if end_sample <= start_sample:
            return

        waveform = self.analysis.waveform[start_sample:end_sample]

        # Downsample for display
        width = self.width()
        samples_per_pixel = max(1, len(waveform) // width)

        # Calculate min/max for each pixel column
        center_y = self.height() / 2
        scale = self.height() * 0.4

        path = QPainterPath()
        path.moveTo(0, center_y)

        for i in range(width):
            sample_start = i * samples_per_pixel
            sample_end = min(sample_start + samples_per_pixel, len(waveform))

            if sample_start >= len(waveform):
                break

            chunk = waveform[sample_start:sample_end]
            if len(chunk) > 0:
                # Use RMS for smoother display
                rms = np.sqrt(np.mean(chunk ** 2))
                y = center_y - rms * scale
                path.lineTo(i, y)

        # Mirror for bottom half
        for i in range(width - 1, -1, -1):
            sample_start = i * samples_per_pixel
            sample_end = min(sample_start + samples_per_pixel, len(waveform))

            if sample_start >= len(waveform):
                continue

            chunk = waveform[sample_start:sample_end]
            if len(chunk) > 0:
                rms = np.sqrt(np.mean(chunk ** 2))
                y = center_y + rms * scale
                path.lineTo(i, y)

        path.closeSubpath()

        painter.setPen(Qt.NoPen)
        painter.setBrush(QBrush(self.waveform_color))
        painter.drawPath(path)

    def _draw_beats(self, painter: QPainter):
        """Draw beat markers."""
        if self.analysis.beat_times is None:
            return

        painter.setPen(QPen(self.beat_color, 2))

        for beat_time in self.analysis.beat_times:
            if self.view_start <= beat_time <= self.view_end:
                x = self.time_to_x(beat_time)
                painter.drawLine(int(x), 0, int(x), self.height())

    def _draw_notes(self, painter: QPainter):
        """Draw note markers."""
        if not self.chart or not self.chart.notes:
            return

        note_height = self.height() / max(self.chart.num_keys, 1)

        for note in self.chart.notes:
            if note.time < self.view_start - 1 or note.time > self.view_end + 1:
                continue

            x = self.time_to_x(note.time)
            y = note.lane * note_height

            color = self.note_colors[note.lane % len(self.note_colors)]
            painter.setPen(QPen(color.darker(120), 1))
            painter.setBrush(QBrush(color))

            if note.duration > 0:
                # Hold note - draw as rectangle
                end_x = self.time_to_x(note.time + note.duration)
                width = max(end_x - x, 4)
                painter.drawRect(QRectF(x, y + 2, width, note_height - 4))
            else:
                # Tap note - draw as small rectangle
                painter.drawRect(QRectF(x - 2, y + 2, 4, note_height - 4))

    def _draw_playhead(self, painter: QPainter):
        """Draw playback position indicator."""
        if self.view_start <= self.playback_position <= self.view_end:
            x = self.time_to_x(self.playback_position)
            painter.setPen(QPen(self.playhead_color, 2))
            painter.drawLine(int(x), 0, int(x), self.height())

    def _draw_time_labels(self, painter: QPainter):
        """Draw time labels."""
        painter.setPen(QColor(180, 180, 180))
        font = painter.font()
        font.setPointSize(8)
        painter.setFont(font)

        # Draw start and end times
        start_text = f"{self.view_start:.1f}s"
        end_text = f"{self.view_end:.1f}s"

        painter.drawText(5, self.height() - 5, start_text)
        painter.drawText(self.width() - 50, self.height() - 5, end_text)

    def mousePressEvent(self, event):
        """Handle mouse click to seek."""
        if event.button() == Qt.LeftButton:
            time = self.x_to_time(event.position().x())
            self.time_clicked.emit(time)

    def wheelEvent(self, event):
        """Handle mouse wheel for zooming (independent from note lane)."""
        if self.analysis:
            delta = event.angleDelta().y()
            factor = 1.2 if delta > 0 else 0.8
            center_time = self.x_to_time(event.position().x())
            self.zoom(factor, center_time)
