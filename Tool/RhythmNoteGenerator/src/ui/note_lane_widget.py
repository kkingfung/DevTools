"""
Note lane visualization widget (piano roll style).
Shows notes on separate lanes for editing.
"""

import numpy as np
from PySide6.QtWidgets import QWidget, QToolTip
from PySide6.QtCore import Qt, Signal, QRectF, QPointF
from PySide6.QtGui import (
    QPainter, QPen, QColor, QBrush, QFont,
    QMouseEvent, QWheelEvent
)
from typing import Optional, List, Tuple

from ..core.note_generator import NoteChart, Note


class NoteLaneWidget(QWidget):
    """Widget displaying notes on lanes (piano roll view)."""

    # Signals
    note_selected = Signal(int)       # Note index selected
    note_moved = Signal(int, float, int)  # Note index, new time, new lane
    note_deleted = Signal(int)        # Note index to delete
    note_added = Signal(float, int)   # Time, lane for new note
    view_range_changed = Signal(float, float)  # Start, end time

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumHeight(200)
        self.setMouseTracking(True)
        self.setFocusPolicy(Qt.StrongFocus)

        # Data
        self.chart: Optional[NoteChart] = None
        self.num_keys = 4

        # View state
        self.view_start = 0.0
        self.view_end = 10.0
        self.duration = 10.0
        self.playback_position = 0.0

        # Interaction state
        self.selected_note_idx: Optional[int] = None
        self.hovered_note_idx: Optional[int] = None
        self.dragging = False
        self.drag_start_pos: Optional[QPointF] = None
        self.drag_note_original_time = 0.0
        self.drag_note_original_lane = 0

        # Colors
        self.bg_color = QColor(25, 25, 30)
        self.lane_colors = [
            QColor(40, 40, 50),
            QColor(35, 35, 45),
        ]
        self.lane_labels = ['1', '2', '3', '4', '5', '6']
        self.note_colors = [
            QColor(255, 80, 80),   # Lane 0 - Red
            QColor(80, 255, 80),   # Lane 1 - Green
            QColor(80, 80, 255),   # Lane 2 - Blue
            QColor(255, 255, 80),  # Lane 3 - Yellow
            QColor(255, 80, 255),  # Lane 4 - Magenta
            QColor(80, 255, 255),  # Lane 5 - Cyan
        ]
        self.selected_color = QColor(255, 255, 255)
        self.grid_color = QColor(60, 60, 70)
        self.playhead_color = QColor(255, 255, 255)

        # Layout
        self.lane_label_width = 30

    def set_chart(self, chart: NoteChart):
        """Set the note chart to display."""
        self.chart = chart
        self.num_keys = chart.num_keys
        self.duration = chart.duration
        self.selected_note_idx = None
        self.update()

    def set_num_keys(self, num_keys: int):
        """Set number of key lanes."""
        self.num_keys = max(1, min(6, num_keys))
        self.update()

    def set_view_range(self, start: float, end: float):
        """Set visible time range."""
        self.view_start = max(0, start)
        self.view_end = min(self.duration, end)
        self.update()

    def set_playback_position(self, position: float):
        """Set current playback position."""
        self.playback_position = position
        self.update()

    def get_lane_height(self) -> float:
        """Get height of each lane."""
        return self.height() / max(self.num_keys, 1)

    def time_to_x(self, time: float) -> float:
        """Convert time to x coordinate."""
        if self.view_end == self.view_start:
            return self.lane_label_width
        ratio = (time - self.view_start) / (self.view_end - self.view_start)
        return self.lane_label_width + ratio * (self.width() - self.lane_label_width)

    def x_to_time(self, x: float) -> float:
        """Convert x coordinate to time."""
        content_width = self.width() - self.lane_label_width
        if content_width <= 0:
            return self.view_start
        ratio = (x - self.lane_label_width) / content_width
        return self.view_start + ratio * (self.view_end - self.view_start)

    def y_to_lane(self, y: float) -> int:
        """Convert y coordinate to lane number."""
        lane = int(y / self.get_lane_height())
        return max(0, min(self.num_keys - 1, lane))

    def lane_to_y(self, lane: int) -> float:
        """Convert lane number to y coordinate (top of lane)."""
        return lane * self.get_lane_height()

    def get_note_at(self, x: float, y: float) -> Optional[int]:
        """Get index of note at position, or None."""
        if not self.chart or not self.chart.notes:
            return None

        time = self.x_to_time(x)
        lane = self.y_to_lane(y)
        lane_height = self.get_lane_height()

        # Search notes (reverse order to get top-most first)
        for i in range(len(self.chart.notes) - 1, -1, -1):
            note = self.chart.notes[i]
            if note.lane != lane:
                continue

            note_x = self.time_to_x(note.time)
            if note.duration > 0:
                note_end_x = self.time_to_x(note.time + note.duration)
                if note_x - 5 <= x <= note_end_x + 5:
                    return i
            else:
                if abs(x - note_x) <= 10:
                    return i

        return None

    def paintEvent(self, event):
        """Draw the lane view."""
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)

        # Background
        painter.fillRect(self.rect(), self.bg_color)

        # Draw lanes
        self._draw_lanes(painter)

        # Draw grid
        self._draw_grid(painter)

        # Draw notes
        self._draw_notes(painter)

        # Draw playhead
        self._draw_playhead(painter)

        # Draw lane labels
        self._draw_lane_labels(painter)

    def _draw_lanes(self, painter: QPainter):
        """Draw lane backgrounds."""
        lane_height = self.get_lane_height()

        for i in range(self.num_keys):
            color = self.lane_colors[i % 2]
            y = i * lane_height
            rect = QRectF(self.lane_label_width, y, self.width() - self.lane_label_width, lane_height)
            painter.fillRect(rect, color)

    def _draw_grid(self, painter: QPainter):
        """Draw time grid lines."""
        painter.setPen(QPen(self.grid_color, 1))

        # Calculate grid interval
        view_width = self.view_end - self.view_start
        if view_width < 1:
            interval = 0.1
        elif view_width < 5:
            interval = 0.5
        elif view_width < 20:
            interval = 1.0
        else:
            interval = 5.0

        t = (int(self.view_start / interval) + 1) * interval
        while t < self.view_end:
            x = self.time_to_x(t)
            painter.drawLine(int(x), 0, int(x), self.height())
            t += interval

    def _draw_notes(self, painter: QPainter):
        """Draw all notes."""
        if not self.chart or not self.chart.notes:
            return

        lane_height = self.get_lane_height()
        note_margin = 4

        for i, note in enumerate(self.chart.notes):
            if note.time > self.view_end or (note.time + note.duration) < self.view_start:
                continue

            x = self.time_to_x(note.time)
            y = self.lane_to_y(note.lane) + note_margin

            # Determine color
            if i == self.selected_note_idx:
                color = self.selected_color
                border_color = QColor(255, 200, 0)
            elif i == self.hovered_note_idx:
                color = self.note_colors[note.lane % len(self.note_colors)].lighter(120)
                border_color = QColor(200, 200, 200)
            else:
                color = self.note_colors[note.lane % len(self.note_colors)]
                border_color = color.darker(150)

            painter.setPen(QPen(border_color, 2))
            painter.setBrush(QBrush(color))

            height = lane_height - note_margin * 2

            if note.duration > 0:
                # Hold note
                end_x = self.time_to_x(note.time + note.duration)
                width = max(end_x - x, 8)
                rect = QRectF(x, y, width, height)
                painter.drawRoundedRect(rect, 4, 4)

                # Draw hold indicator lines
                painter.setPen(QPen(border_color.darker(120), 1))
                for lx in range(int(x) + 10, int(x + width) - 5, 8):
                    painter.drawLine(lx, int(y + 3), lx, int(y + height - 3))
            else:
                # Tap note
                width = 12
                rect = QRectF(x - width / 2, y, width, height)
                painter.drawRoundedRect(rect, 3, 3)

    def _draw_playhead(self, painter: QPainter):
        """Draw playback position."""
        if self.view_start <= self.playback_position <= self.view_end:
            x = self.time_to_x(self.playback_position)
            painter.setPen(QPen(self.playhead_color, 2))
            painter.drawLine(int(x), 0, int(x), self.height())

    def _draw_lane_labels(self, painter: QPainter):
        """Draw lane number labels on the left."""
        lane_height = self.get_lane_height()

        # Background
        painter.fillRect(QRectF(0, 0, self.lane_label_width, self.height()),
                         QColor(40, 40, 50))

        # Labels
        painter.setPen(QColor(180, 180, 180))
        font = painter.font()
        font.setPointSize(10)
        font.setBold(True)
        painter.setFont(font)

        for i in range(self.num_keys):
            y = i * lane_height
            rect = QRectF(0, y, self.lane_label_width, lane_height)
            painter.drawText(rect, Qt.AlignCenter, self.lane_labels[i])

    def mousePressEvent(self, event: QMouseEvent):
        """Handle mouse press."""
        if event.button() == Qt.LeftButton:
            note_idx = self.get_note_at(event.position().x(), event.position().y())

            if note_idx is not None:
                self.selected_note_idx = note_idx
                self.dragging = True
                self.drag_start_pos = event.position()
                note = self.chart.notes[note_idx]
                self.drag_note_original_time = note.time
                self.drag_note_original_lane = note.lane
                self.note_selected.emit(note_idx)
            else:
                self.selected_note_idx = None

            self.update()

        elif event.button() == Qt.RightButton:
            # Right click to add note
            if event.position().x() > self.lane_label_width:
                time = self.x_to_time(event.position().x())
                lane = self.y_to_lane(event.position().y())
                self.note_added.emit(time, lane)

    def mouseMoveEvent(self, event: QMouseEvent):
        """Handle mouse move."""
        # Update hover state
        note_idx = self.get_note_at(event.position().x(), event.position().y())
        if note_idx != self.hovered_note_idx:
            self.hovered_note_idx = note_idx
            self.update()

        # Show tooltip
        if note_idx is not None and self.chart:
            note = self.chart.notes[note_idx]
            tooltip = f"Time: {note.time:.3f}s\nLane: {note.lane + 1}"
            if note.duration > 0:
                tooltip += f"\nDuration: {note.duration:.3f}s"
            QToolTip.showText(event.globalPosition().toPoint(), tooltip)

        # Handle dragging
        if self.dragging and self.selected_note_idx is not None and self.drag_start_pos is not None:
            dx = event.position().x() - self.drag_start_pos.x()
            time_delta = (dx / (self.width() - self.lane_label_width)) * (self.view_end - self.view_start)
            new_time = max(0, self.drag_note_original_time + time_delta)

            new_lane = self.y_to_lane(event.position().y())

            # Update note position
            if self.chart:
                note = self.chart.notes[self.selected_note_idx]
                note.time = new_time
                note.lane = new_lane
                self.update()

    def mouseReleaseEvent(self, event: QMouseEvent):
        """Handle mouse release."""
        if event.button() == Qt.LeftButton and self.dragging:
            self.dragging = False
            if self.selected_note_idx is not None and self.chart:
                note = self.chart.notes[self.selected_note_idx]
                self.note_moved.emit(self.selected_note_idx, note.time, note.lane)
            self.drag_start_pos = None

    def keyPressEvent(self, event):
        """Handle key press for note deletion."""
        if event.key() in (Qt.Key_Delete, Qt.Key_Backspace):
            if self.selected_note_idx is not None:
                self.note_deleted.emit(self.selected_note_idx)
                self.selected_note_idx = None
                self.update()

    def wheelEvent(self, event: QWheelEvent):
        """Handle mouse wheel for zooming."""
        delta = event.angleDelta().y()
        factor = 1.2 if delta > 0 else 0.8

        center_time = self.x_to_time(event.position().x())
        current_width = self.view_end - self.view_start
        new_width = current_width / factor
        new_width = max(0.5, min(self.duration, new_width))

        ratio = (center_time - self.view_start) / current_width if current_width > 0 else 0.5
        new_start = center_time - new_width * ratio
        new_end = new_start + new_width

        # Clamp
        if new_start < 0:
            new_start = 0
            new_end = new_width
        if new_end > self.duration:
            new_end = self.duration
            new_start = new_end - new_width

        self.set_view_range(new_start, new_end)
        self.view_range_changed.emit(self.view_start, self.view_end)
