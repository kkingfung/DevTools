"""
Main window for the Rhythm Note Generator application.
"""

import os
import traceback
from pathlib import Path
from typing import Optional

from PySide6.QtWidgets import (
    QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QPushButton, QComboBox, QSpinBox, QSlider,
    QFileDialog, QMessageBox, QGroupBox, QFormLayout,
    QLineEdit, QScrollArea, QSplitter, QProgressDialog,
    QStatusBar, QTabWidget, QTextEdit, QScrollBar, QCheckBox
)
from PySide6.QtCore import Qt, QTimer, Signal, Slot, QThread

from PySide6.QtGui import QAction, QKeySequence

from .waveform_widget import WaveformWidget
from .note_lane_widget import NoteLaneWidget
from .kalimba_widget import KalimbaWidget
from ..core.audio_analyzer import AudioAnalyzer, AudioAnalysisResult, PitchNote
from ..core.note_generator import NoteGenerator, NoteChart, Note, Difficulty
from ..core.template_manager import TemplateManager

# Use pygame for audio playback (more reliable on Windows)
import pygame
import numpy as np


class AnalysisWorker(QThread):
    """Background worker for audio analysis."""
    finished = Signal(object)  # AudioAnalysisResult
    progress = Signal(int, str)  # Progress percent and message
    error = Signal(str)

    def __init__(self, analyzer: AudioAnalyzer, file_path: str):
        super().__init__()
        self.analyzer = analyzer
        self.file_path = file_path

    def _progress_callback(self, percent: int, message: str):
        """Callback for progress updates from analyzer."""
        self.progress.emit(percent, message)

    def run(self):
        try:
            result = self.analyzer.analyze(
                self.file_path,
                progress_callback=self._progress_callback
            )
            self.finished.emit(result)
        except Exception as e:
            self.error.emit(f"{e}\n{traceback.format_exc()}")


class PitchDetectionWorker(QThread):
    """Background worker for pitch detection."""
    finished = Signal(list)  # List of PitchNote
    progress = Signal(int, str)   # Progress percent and message
    error = Signal(str)

    def __init__(self, analyzer: AudioAnalyzer, file_path: str):
        super().__init__()
        self.analyzer = analyzer
        self.file_path = file_path

    def _progress_callback(self, percent: int, message: str):
        """Callback for progress updates from analyzer."""
        self.progress.emit(percent, message)

    def run(self):
        try:
            pitch_notes = self.analyzer.detect_pitches(
                self.file_path,
                progress_callback=self._progress_callback
            )
            self.finished.emit(pitch_notes)
        except Exception as e:
            self.error.emit(f"{e}\n{traceback.format_exc()}")


class MainWindow(QMainWindow):
    """Main application window."""

    def __init__(self):
        super().__init__()
        self.setWindowTitle("Rhythm Note Generator")
        self.setMinimumSize(1200, 800)

        # Core components
        self.analyzer = AudioAnalyzer()
        self.generator: Optional[NoteGenerator] = None
        self.template_manager = TemplateManager()

        # Data
        self.current_file: Optional[str] = None
        self.analysis: Optional[AudioAnalysisResult] = None
        self.chart: Optional[NoteChart] = None

        # Audio playback with pygame (init both mixer and time)
        pygame.init()
        pygame.mixer.init(frequency=44100, size=-16, channels=2, buffer=512)
        self.is_playing = False
        self.playback_start_time = 0
        self.playback_offset = 0.0
        self.last_played_note_idx = -1  # Track which notes have been played

        # Generate note sounds (different pitch per lane)
        self.note_sounds_enabled = True
        self.note_sounds = self._generate_note_sounds()

        # Playback timer
        self.playback_timer = QTimer()
        self.playback_timer.setInterval(16)  # ~60 fps for better note timing
        self.playback_timer.timeout.connect(self._update_playback_position)

        # Setup UI
        self._setup_ui()
        self._setup_menu()
        self._setup_connections()

        # Status bar
        self.statusBar().showMessage("Ready. Load an audio file to begin.")

    def _setup_ui(self):
        """Setup the main UI layout."""
        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        main_layout = QHBoxLayout(central_widget)

        # Left panel - Settings
        left_panel = self._create_settings_panel()
        left_panel.setMaximumWidth(350)
        left_panel.setMinimumWidth(280)

        # Right panel - Visualization
        right_panel = self._create_visualization_panel()

        # Splitter
        splitter = QSplitter(Qt.Horizontal)
        splitter.addWidget(left_panel)
        splitter.addWidget(right_panel)
        splitter.setSizes([300, 900])

        main_layout.addWidget(splitter)

    def _create_settings_panel(self) -> QWidget:
        """Create the left settings panel."""
        panel = QWidget()
        layout = QVBoxLayout(panel)

        # File section
        file_group = QGroupBox("Audio File")
        file_layout = QVBoxLayout(file_group)

        self.file_label = QLabel("No file loaded")
        self.file_label.setWordWrap(True)
        file_layout.addWidget(self.file_label)

        load_btn = QPushButton("Load Audio File...")
        load_btn.clicked.connect(self._load_file)
        file_layout.addWidget(load_btn)

        layout.addWidget(file_group)

        # Metadata section
        meta_group = QGroupBox("Metadata")
        meta_layout = QFormLayout(meta_group)

        self.title_edit = QLineEdit()
        self.title_edit.setPlaceholderText("Song title")
        meta_layout.addRow("Title:", self.title_edit)

        self.artist_edit = QLineEdit()
        self.artist_edit.setPlaceholderText("Artist name")
        meta_layout.addRow("Artist:", self.artist_edit)

        layout.addWidget(meta_group)

        # Generation settings
        gen_group = QGroupBox("Generation Settings")
        gen_layout = QFormLayout(gen_group)

        self.keys_spin = QSpinBox()
        self.keys_spin.setRange(1, 6)
        self.keys_spin.setValue(4)
        self.keys_spin.valueChanged.connect(self._on_keys_changed)
        gen_layout.addRow("Keys:", self.keys_spin)

        self.difficulty_combo = QComboBox()
        for diff in Difficulty:
            self.difficulty_combo.addItem(diff.name, diff)
        self.difficulty_combo.setCurrentIndex(1)  # NORMAL
        gen_layout.addRow("Difficulty:", self.difficulty_combo)

        layout.addWidget(gen_group)

        # Generate button
        self.generate_btn = QPushButton("Generate Notes")
        self.generate_btn.setEnabled(False)
        self.generate_btn.clicked.connect(self._generate_notes)
        self.generate_btn.setStyleSheet("QPushButton { font-size: 14px; padding: 10px; }")
        layout.addWidget(self.generate_btn)

        # Playback controls
        playback_group = QGroupBox("Playback")
        playback_layout = QVBoxLayout(playback_group)

        btn_layout = QHBoxLayout()
        self.play_btn = QPushButton("Play")
        self.play_btn.clicked.connect(self._toggle_playback)
        self.play_btn.setEnabled(False)
        btn_layout.addWidget(self.play_btn)

        self.stop_btn = QPushButton("Stop")
        self.stop_btn.clicked.connect(self._stop_playback)
        self.stop_btn.setEnabled(False)
        btn_layout.addWidget(self.stop_btn)
        playback_layout.addLayout(btn_layout)

        self.position_slider = QSlider(Qt.Horizontal)
        self.position_slider.setEnabled(False)
        self.position_slider.sliderPressed.connect(self._on_slider_pressed)
        self.position_slider.sliderReleased.connect(self._on_slider_released)
        playback_layout.addWidget(self.position_slider)

        self.position_label = QLabel("0:00 / 0:00")
        playback_layout.addWidget(self.position_label)

        # Note sounds checkbox
        from PySide6.QtWidgets import QCheckBox
        self.note_sounds_checkbox = QCheckBox("Play note sounds")
        self.note_sounds_checkbox.setChecked(True)
        self.note_sounds_checkbox.stateChanged.connect(self._on_note_sounds_changed)
        playback_layout.addWidget(self.note_sounds_checkbox)

        layout.addWidget(playback_group)

        # Import/Export section
        export_group = QGroupBox("Import / Export")
        export_layout = QFormLayout(export_group)

        self.import_btn = QPushButton("Import Chart...")
        self.import_btn.clicked.connect(self._import_chart)
        export_layout.addRow(self.import_btn)

        self.template_combo = QComboBox()
        templates = self.template_manager.list_templates()
        for template in templates:
            self.template_combo.addItem(
                f"{template['name']} ({template['extension']})",
                template['name'].lower()
            )
        export_layout.addRow("Template:", self.template_combo)

        self.export_btn = QPushButton("Export Chart...")
        self.export_btn.setEnabled(False)
        self.export_btn.clicked.connect(self._export_chart)
        export_layout.addRow(self.export_btn)

        layout.addWidget(export_group)

        # Stats section
        stats_group = QGroupBox("Chart Statistics")
        stats_layout = QFormLayout(stats_group)

        self.tempo_label = QLabel("-")
        stats_layout.addRow("BPM:", self.tempo_label)

        self.notes_label = QLabel("-")
        stats_layout.addRow("Notes:", self.notes_label)

        self.density_label = QLabel("-")
        stats_layout.addRow("Notes/sec:", self.density_label)

        self.diff_value_label = QLabel("-")
        stats_layout.addRow("Difficulty:", self.diff_value_label)

        layout.addWidget(stats_group)

        # Stretch to fill
        layout.addStretch()

        return panel

    def _create_visualization_panel(self) -> QWidget:
        """Create the right visualization panel."""
        panel = QWidget()
        layout = QVBoxLayout(panel)

        # Tab widget for different views
        self.viz_tabs = QTabWidget()

        # Waveform + Lane view tab
        viz_widget = QWidget()
        viz_layout = QVBoxLayout(viz_widget)

        # Waveform
        waveform_label = QLabel("Waveform (scroll to zoom)")
        viz_layout.addWidget(waveform_label)

        self.waveform_widget = WaveformWidget()
        self.waveform_widget.setMinimumHeight(150)
        viz_layout.addWidget(self.waveform_widget)

        # Note lanes
        lanes_label = QLabel("Note Lanes (right-click to add, Delete key to remove)")
        viz_layout.addWidget(lanes_label)

        self.lane_widget = NoteLaneWidget()
        self.lane_widget.setMinimumHeight(300)
        viz_layout.addWidget(self.lane_widget)

        # Horizontal scrollbar for timeline navigation
        self.timeline_scrollbar = QScrollBar(Qt.Horizontal)
        self.timeline_scrollbar.setMinimum(0)
        self.timeline_scrollbar.setMaximum(1000)  # Will be updated when audio loads
        self.timeline_scrollbar.setPageStep(100)
        self.timeline_scrollbar.valueChanged.connect(self._on_scrollbar_changed)
        viz_layout.addWidget(self.timeline_scrollbar)

        # Zoom controls
        zoom_layout = QHBoxLayout()
        zoom_layout.addWidget(QLabel("View range:"))

        self.zoom_out_btn = QPushButton("-")
        self.zoom_out_btn.setMaximumWidth(40)
        self.zoom_out_btn.clicked.connect(lambda: self._zoom(0.8))
        zoom_layout.addWidget(self.zoom_out_btn)

        self.zoom_in_btn = QPushButton("+")
        self.zoom_in_btn.setMaximumWidth(40)
        self.zoom_in_btn.clicked.connect(lambda: self._zoom(1.25))
        zoom_layout.addWidget(self.zoom_in_btn)

        self.reset_view_btn = QPushButton("Reset View")
        self.reset_view_btn.clicked.connect(self._reset_view)
        zoom_layout.addWidget(self.reset_view_btn)

        zoom_layout.addStretch()
        viz_layout.addLayout(zoom_layout)

        self.viz_tabs.addTab(viz_widget, "Visualization")

        # Kalimba Tab view
        kalimba_widget = QWidget()
        kalimba_layout = QVBoxLayout(kalimba_widget)

        kalimba_header = QHBoxLayout()
        kalimba_header.addWidget(QLabel("Kalimba Tab (17-key C major)"))

        # Pitch detection button
        self.detect_pitch_btn = QPushButton("Detect Pitches")
        self.detect_pitch_btn.setEnabled(False)
        self.detect_pitch_btn.clicked.connect(self._detect_pitches)
        kalimba_header.addWidget(self.detect_pitch_btn)

        kalimba_header.addStretch()
        kalimba_layout.addLayout(kalimba_header)

        self.kalimba_widget = KalimbaWidget()
        self.kalimba_widget.setMinimumHeight(400)
        kalimba_layout.addWidget(self.kalimba_widget)

        # Kalimba scrollbar
        self.kalimba_scrollbar = QScrollBar(Qt.Horizontal)
        self.kalimba_scrollbar.setMinimum(0)
        self.kalimba_scrollbar.setMaximum(1000)
        self.kalimba_scrollbar.setPageStep(100)
        self.kalimba_scrollbar.valueChanged.connect(self._on_kalimba_scrollbar_changed)
        kalimba_layout.addWidget(self.kalimba_scrollbar)

        # Kalimba controls
        kalimba_controls = QHBoxLayout()
        kalimba_controls.addWidget(QLabel("Zoom:"))

        kalimba_zoom_out = QPushButton("-")
        kalimba_zoom_out.setMaximumWidth(40)
        kalimba_zoom_out.clicked.connect(lambda: self._zoom_kalimba(0.8))
        kalimba_controls.addWidget(kalimba_zoom_out)

        kalimba_zoom_in = QPushButton("+")
        kalimba_zoom_in.setMaximumWidth(40)
        kalimba_zoom_in.clicked.connect(lambda: self._zoom_kalimba(1.25))
        kalimba_controls.addWidget(kalimba_zoom_in)

        kalimba_controls.addStretch()
        kalimba_layout.addLayout(kalimba_controls)

        self.viz_tabs.addTab(kalimba_widget, "Kalimba Tab")

        # Preview tab (text preview of export)
        preview_widget = QWidget()
        preview_layout = QVBoxLayout(preview_widget)

        preview_controls = QHBoxLayout()
        preview_controls.addWidget(QLabel("Template:"))

        self.preview_template_combo = QComboBox()
        templates = self.template_manager.list_templates()
        for template in templates:
            self.preview_template_combo.addItem(f"{template['name']}", template['name'].lower())
        self.preview_template_combo.currentIndexChanged.connect(self._update_preview)
        preview_controls.addWidget(self.preview_template_combo)

        refresh_preview_btn = QPushButton("Refresh")
        refresh_preview_btn.clicked.connect(self._update_preview)
        preview_controls.addWidget(refresh_preview_btn)

        preview_controls.addStretch()
        preview_layout.addLayout(preview_controls)

        self.preview_text = QTextEdit()
        self.preview_text.setReadOnly(True)
        self.preview_text.setFontFamily("Consolas")
        preview_layout.addWidget(self.preview_text)

        self.viz_tabs.addTab(preview_widget, "Export Preview")

        layout.addWidget(self.viz_tabs)

        return panel

    def _setup_menu(self):
        """Setup menu bar."""
        menubar = self.menuBar()

        # File menu
        file_menu = menubar.addMenu("File")

        open_action = QAction("Open Audio...", self)
        open_action.setShortcut(QKeySequence.Open)
        open_action.triggered.connect(self._load_file)
        file_menu.addAction(open_action)

        import_action = QAction("Import Chart...", self)
        import_action.setShortcut(QKeySequence("Ctrl+I"))
        import_action.triggered.connect(self._import_chart)
        file_menu.addAction(import_action)

        export_action = QAction("Export Chart...", self)
        export_action.setShortcut(QKeySequence.Save)
        export_action.triggered.connect(self._export_chart)
        file_menu.addAction(export_action)

        file_menu.addSeparator()

        quit_action = QAction("Quit", self)
        quit_action.setShortcut(QKeySequence.Quit)
        quit_action.triggered.connect(self.close)
        file_menu.addAction(quit_action)

        # Edit menu
        edit_menu = menubar.addMenu("Edit")

        regenerate_action = QAction("Regenerate Notes", self)
        regenerate_action.setShortcut(QKeySequence("Ctrl+R"))
        regenerate_action.triggered.connect(self._generate_notes)
        edit_menu.addAction(regenerate_action)

        # Help menu
        help_menu = menubar.addMenu("Help")

        about_action = QAction("About", self)
        about_action.triggered.connect(self._show_about)
        help_menu.addAction(about_action)

    def _setup_connections(self):
        """Setup signal connections."""
        # Waveform click to seek
        self.waveform_widget.time_clicked.connect(self._on_time_clicked)

        # Lane view range changes - only update scrollbar (independent zoom)
        self.lane_widget.view_range_changed.connect(self._on_lane_view_range_changed)

        # Lane note editing signals
        self.lane_widget.note_added.connect(self._add_note)
        self.lane_widget.note_deleted.connect(self._delete_note)

    @Slot()
    def _load_file(self):
        """Load an audio file."""
        file_path, _ = QFileDialog.getOpenFileName(
            self,
            "Open Audio File",
            "",
            "Audio Files (*.wav *.mp3 *.ogg *.flac);;All Files (*)"
        )

        if not file_path:
            return

        # Stop any current playback
        self._stop_playback()

        self.current_file = file_path
        self.file_label.setText(Path(file_path).name)

        # Set title from filename
        stem = Path(file_path).stem
        self.title_edit.setText(stem)

        # Show progress dialog with percentage
        self.analysis_progress = QProgressDialog("Analyzing audio...", None, 0, 100, self)
        self.analysis_progress.setWindowTitle("Audio Analysis")
        self.analysis_progress.setWindowModality(Qt.WindowModal)
        self.analysis_progress.setMinimumDuration(0)
        self.analysis_progress.setMinimumWidth(350)
        self.analysis_progress.setValue(0)
        self.analysis_progress.show()

        # Run analysis in background
        self.worker = AnalysisWorker(self.analyzer, file_path)
        self.worker.progress.connect(self._on_analysis_progress)
        self.worker.finished.connect(self._on_analysis_complete_new)
        self.worker.error.connect(self._on_analysis_error_new)
        self.worker.start()

    def _on_analysis_progress(self, percent: int, message: str):
        """Update audio analysis progress."""
        if hasattr(self, 'analysis_progress') and self.analysis_progress:
            self.analysis_progress.setValue(percent)
            self.analysis_progress.setLabelText(message)
        self.statusBar().showMessage(f"Audio analysis: {message}")

    def _on_analysis_complete_new(self, result: AudioAnalysisResult):
        """Handle completed audio analysis."""
        if hasattr(self, 'analysis_progress') and self.analysis_progress:
            self.analysis_progress.close()
        self._on_analysis_complete(result, None)

    def _on_analysis_error_new(self, error: str):
        """Handle analysis error."""
        if hasattr(self, 'analysis_progress') and self.analysis_progress:
            self.analysis_progress.close()
        self._on_analysis_error(error, None)

    def _on_analysis_complete(self, result: AudioAnalysisResult, progress: QProgressDialog):
        """Handle completed audio analysis."""
        if progress:
            progress.close()
        self.analysis = result

        # Update UI
        self.waveform_widget.set_analysis(result)
        self.lane_widget.duration = result.duration
        self.lane_widget.set_view_range(0, min(10.0, result.duration))

        # Initialize kalimba widget
        self.kalimba_widget.duration = result.duration
        self.kalimba_widget.set_view_range(0, min(10.0, result.duration))
        self.detect_pitch_btn.setEnabled(True)

        self.tempo_label.setText(f"{result.tempo:.1f}")
        self.generate_btn.setEnabled(True)

        # Load audio for playback with pygame
        try:
            pygame.mixer.music.load(self.current_file)
            self.play_btn.setEnabled(True)
            self.stop_btn.setEnabled(True)
            self.position_slider.setEnabled(True)
            self.position_slider.setRange(0, int(result.duration * 1000))
        except Exception as e:
            self.statusBar().showMessage(f"Playback not available: {e}")
            self.play_btn.setEnabled(False)

        # Update scrollbars
        self._update_scrollbar_from_view()
        self._update_kalimba_scrollbar_from_view()

        self.statusBar().showMessage(
            f"Loaded: {Path(self.current_file).name} | Duration: {result.duration:.1f}s | BPM: {result.tempo:.1f}"
        )

    def _on_analysis_error(self, error: str, progress: QProgressDialog):
        """Handle analysis error."""
        if progress:
            progress.close()
        QMessageBox.critical(self, "Error", f"Failed to analyze audio:\n{error}")

    @Slot()
    def _generate_notes(self):
        """Generate note chart from analysis."""
        if not self.analysis:
            return

        num_keys = self.keys_spin.value()
        difficulty = self.difficulty_combo.currentData()

        self.generator = NoteGenerator(num_keys=num_keys, difficulty=difficulty)
        self.chart = self.generator.generate(
            self.analysis,
            title=self.title_edit.text() or "Untitled",
            artist=self.artist_edit.text() or "Unknown",
            audio_file=Path(self.current_file).name if self.current_file else ""
        )

        # Update visualization
        self.waveform_widget.set_chart(self.chart)
        self.lane_widget.set_chart(self.chart)

        # Update stats
        self.notes_label.setText(str(len(self.chart.notes)))
        if self.chart.duration > 0:
            density = len(self.chart.notes) / self.chart.duration
            self.density_label.setText(f"{density:.2f}")
        self.diff_value_label.setText(f"{self.chart.difficulty_value}/10")

        self.export_btn.setEnabled(True)
        self._update_preview()

        self.statusBar().showMessage(f"Generated {len(self.chart.notes)} notes")

    @Slot()
    def _toggle_playback(self):
        """Toggle audio playback."""
        if self.is_playing:
            # Pause
            pygame.mixer.music.pause()
            self.is_playing = False
            self.play_btn.setText("Play")
            self.playback_timer.stop()
            # Save current position
            self.playback_offset += (pygame.time.get_ticks() - self.playback_start_time) / 1000.0
        else:
            # Play
            if self.playback_offset > 0:
                pygame.mixer.music.unpause()
            else:
                pygame.mixer.music.play()
            self.is_playing = True
            self.play_btn.setText("Pause")
            self.playback_start_time = pygame.time.get_ticks()
            self.playback_timer.start()

    @Slot()
    def _stop_playback(self):
        """Stop audio playback."""
        pygame.mixer.music.stop()
        self.is_playing = False
        self.play_btn.setText("Play")
        self.playback_timer.stop()
        self.playback_offset = 0.0
        self.last_played_note_idx = -1  # Reset note tracking
        self.position_slider.setValue(0)
        self._update_position_label(0)
        self.waveform_widget.set_playback_position(0)
        self.lane_widget.set_playback_position(0)

    def _on_slider_pressed(self):
        """Handle slider press - pause updates."""
        self.playback_timer.stop()

    def _on_slider_released(self):
        """Handle slider release - seek to position."""
        position_ms = self.position_slider.value()
        position_sec = position_ms / 1000.0

        # Reset note tracking to position before seek point
        self._reset_note_tracking(position_sec)

        # Seek in pygame
        if self.is_playing:
            pygame.mixer.music.stop()
            pygame.mixer.music.play(start=position_sec)
            self.playback_start_time = pygame.time.get_ticks()
            self.playback_offset = position_sec
            self.playback_timer.start()
        else:
            self.playback_offset = position_sec

        self._update_position_label(position_ms)
        self.waveform_widget.set_playback_position(position_sec)
        self.lane_widget.set_playback_position(position_sec)
        self.kalimba_widget.set_playback_position(position_sec)

    def _update_playback_position(self):
        """Update visualization playback position."""
        if not self.is_playing:
            return

        # Calculate current position
        elapsed = (pygame.time.get_ticks() - self.playback_start_time) / 1000.0
        position_sec = self.playback_offset + elapsed

        # Check if playback finished
        if self.analysis and position_sec >= self.analysis.duration:
            self._stop_playback()
            return

        position_ms = int(position_sec * 1000)

        self.position_slider.setValue(position_ms)
        self._update_position_label(position_ms)
        self.waveform_widget.set_playback_position(position_sec)
        self.lane_widget.set_playback_position(position_sec)
        self.kalimba_widget.set_playback_position(position_sec)

        # Play note sounds
        if self.note_sounds_enabled and self.chart and self.chart.notes:
            self._play_notes_at_position(position_sec)

    def _play_notes_at_position(self, position_sec: float):
        """Play note sounds for notes at current position."""
        if not self.chart or not self.note_sounds:
            return

        # Find notes that should play (within a small window)
        tolerance = 0.05  # 50ms tolerance

        for i, note in enumerate(self.chart.notes):
            # Skip already played notes
            if i <= self.last_played_note_idx:
                continue

            # Check if note is within play window
            if note.time <= position_sec + tolerance:
                if note.time >= position_sec - tolerance:
                    # Play the note sound
                    lane = note.lane % len(self.note_sounds)
                    self.note_sounds[lane].play()
                self.last_played_note_idx = i
            else:
                # Notes are sorted by time, so we can stop early
                break

    def _reset_note_tracking(self, position_sec: float):
        """Reset note tracking index based on seek position."""
        if not self.chart or not self.chart.notes:
            self.last_played_note_idx = -1
            return

        # Find the last note before the seek position
        self.last_played_note_idx = -1
        for i, note in enumerate(self.chart.notes):
            if note.time < position_sec:
                self.last_played_note_idx = i
            else:
                break

    def _generate_note_sounds(self) -> list:
        """Generate beep sounds for each lane (different frequencies)."""
        sounds = []
        sample_rate = 44100
        duration = 0.08  # 80ms beep

        # Frequencies for each lane (pentatonic scale - pleasant sounding)
        frequencies = [523, 587, 659, 784, 880, 988]  # C5, D5, E5, G5, A5, B5

        for freq in frequencies:
            # Generate sine wave
            t = np.linspace(0, duration, int(sample_rate * duration), False)
            wave = np.sin(2 * np.pi * freq * t)

            # Apply envelope (attack/release) to avoid clicks
            envelope = np.ones_like(wave)
            attack_samples = int(0.005 * sample_rate)  # 5ms attack
            release_samples = int(0.02 * sample_rate)  # 20ms release
            envelope[:attack_samples] = np.linspace(0, 1, attack_samples)
            envelope[-release_samples:] = np.linspace(1, 0, release_samples)
            wave *= envelope

            # Convert to 16-bit integer
            wave = (wave * 32767 * 0.5).astype(np.int16)

            # Create stereo
            stereo = np.column_stack((wave, wave))

            # Create pygame sound
            sound = pygame.sndarray.make_sound(stereo)
            sounds.append(sound)

        return sounds

    def _on_note_sounds_changed(self, state: int):
        """Handle note sounds checkbox change."""
        self.note_sounds_enabled = state == Qt.Checked.value

    def _update_position_label(self, position_ms: int):
        """Update the position label."""
        duration_ms = int(self.analysis.duration * 1000) if self.analysis else 0
        pos_str = f"{int(position_ms // 60000)}:{int((position_ms // 1000) % 60):02d}"
        dur_str = f"{int(duration_ms // 60000)}:{int((duration_ms // 1000) % 60):02d}"
        self.position_label.setText(f"{pos_str} / {dur_str}")

    @Slot(float)
    def _on_time_clicked(self, time: float):
        """Seek to clicked time."""
        position_ms = int(time * 1000)
        self.position_slider.setValue(position_ms)

        # Reset note tracking to position before seek point
        self._reset_note_tracking(time)

        if self.is_playing:
            pygame.mixer.music.stop()
            pygame.mixer.music.play(start=time)
            self.playback_start_time = pygame.time.get_ticks()
            self.playback_offset = time
        else:
            self.playback_offset = time

        self._update_position_label(position_ms)
        self.waveform_widget.set_playback_position(time)
        self.lane_widget.set_playback_position(time)
        self.kalimba_widget.set_playback_position(time)

    @Slot(float, float)
    def _on_lane_view_range_changed(self, start: float, end: float):
        """Update scrollbar when lane widget view range changes (independent zoom)."""
        self._update_scrollbar_from_view()

    def _on_keys_changed(self, value: int):
        """Update lane widget when keys change."""
        self.lane_widget.set_num_keys(value)

    @Slot(float, int)
    def _add_note(self, time: float, lane: int):
        """Add a new note."""
        if self.chart:
            note = Note(time=time, lane=lane, duration=0.0)
            self.chart.notes.append(note)
            self.chart.notes.sort(key=lambda n: (n.time, n.lane))
            self._refresh_chart_display()

    @Slot(int)
    def _delete_note(self, index: int):
        """Delete a note."""
        if self.chart and 0 <= index < len(self.chart.notes):
            del self.chart.notes[index]
            self._refresh_chart_display()

    def _refresh_chart_display(self):
        """Refresh all chart displays."""
        if self.chart:
            self.waveform_widget.set_chart(self.chart)
            self.lane_widget.set_chart(self.chart)
            self.notes_label.setText(str(len(self.chart.notes)))
            self._update_preview()

    def _zoom(self, factor: float):
        """Zoom lane widget view by factor (independent from waveform)."""
        current_range = self.lane_widget.view_end - self.lane_widget.view_start
        center = self.lane_widget.view_start + current_range / 2

        new_range = current_range * factor
        # Clamp range to valid bounds
        if self.analysis:
            new_range = max(1.0, min(new_range, self.analysis.duration))

        new_start = max(0, center - new_range / 2)
        new_end = new_start + new_range

        # Clamp to duration
        if self.analysis and new_end > self.analysis.duration:
            new_end = self.analysis.duration
            new_start = max(0, new_end - new_range)

        # Only update lane widget (waveform has independent zoom)
        self.lane_widget.set_view_range(new_start, new_end)
        self._update_scrollbar_from_view()

    @Slot()
    def _reset_view(self):
        """Reset lane widget view to full duration (waveform independent)."""
        if self.analysis:
            self.lane_widget.set_view_range(0, self.analysis.duration)
            self._update_scrollbar_from_view()

    def _on_scrollbar_changed(self, value: int):
        """Handle timeline scrollbar - only affects lane widget (waveform independent)."""
        if not self.analysis or self.analysis.duration <= 0:
            return

        # Calculate view position from scrollbar value
        max_val = self.timeline_scrollbar.maximum()
        if max_val <= 0:
            return

        # Use lane_widget view range
        view_width = self.lane_widget.view_end - self.lane_widget.view_start

        # Calculate new start position
        scrollable_range = self.analysis.duration - view_width
        if scrollable_range <= 0:
            return

        new_start = (value / max_val) * scrollable_range
        new_end = new_start + view_width

        # Clamp to valid range
        if new_end > self.analysis.duration:
            new_end = self.analysis.duration
            new_start = max(0, new_end - view_width)

        # Only update lane widget (waveform has independent zoom)
        self.lane_widget.set_view_range(new_start, new_end)

    def _update_scrollbar_from_view(self):
        """Update scrollbar position based on lane widget view range."""
        if not self.analysis or self.analysis.duration <= 0:
            return

        # Use lane_widget as the primary reference
        view_width = self.lane_widget.view_end - self.lane_widget.view_start
        scrollable_range = self.analysis.duration - view_width

        if scrollable_range <= 0:
            self.timeline_scrollbar.setValue(0)
            self.timeline_scrollbar.setEnabled(False)
            return

        self.timeline_scrollbar.setEnabled(True)

        # Calculate scrollbar value based on lane widget position
        max_val = self.timeline_scrollbar.maximum()
        value = int((self.lane_widget.view_start / scrollable_range) * max_val)

        # Block signals to prevent feedback loop
        self.timeline_scrollbar.blockSignals(True)
        self.timeline_scrollbar.setValue(min(value, max_val))
        self.timeline_scrollbar.blockSignals(False)

    def _on_kalimba_scrollbar_changed(self, value: int):
        """Handle kalimba scrollbar value change."""
        if not self.analysis or self.analysis.duration <= 0:
            return

        max_val = self.kalimba_scrollbar.maximum()
        if max_val <= 0:
            return

        view_width = self.kalimba_widget.view_end - self.kalimba_widget.view_start
        scrollable_range = self.analysis.duration - view_width
        if scrollable_range <= 0:
            return

        new_start = (value / max_val) * scrollable_range
        new_end = new_start + view_width

        self.kalimba_widget.set_view_range(new_start, new_end)

    def _update_kalimba_scrollbar_from_view(self):
        """Update kalimba scrollbar position based on current view range."""
        if not self.analysis or self.analysis.duration <= 0:
            return

        view_width = self.kalimba_widget.view_end - self.kalimba_widget.view_start
        scrollable_range = self.analysis.duration - view_width

        if scrollable_range <= 0:
            self.kalimba_scrollbar.setValue(0)
            return

        max_val = self.kalimba_scrollbar.maximum()
        value = int((self.kalimba_widget.view_start / scrollable_range) * max_val)

        self.kalimba_scrollbar.blockSignals(True)
        self.kalimba_scrollbar.setValue(value)
        self.kalimba_scrollbar.blockSignals(False)

    def _zoom_kalimba(self, factor: float):
        """Zoom kalimba view by factor."""
        current_range = self.kalimba_widget.view_end - self.kalimba_widget.view_start
        center = self.kalimba_widget.view_start + current_range / 2

        new_range = current_range * factor
        new_range = max(1.0, min(new_range, self.analysis.duration if self.analysis else 300))

        new_start = max(0, center - new_range / 2)
        new_end = new_start + new_range

        if self.analysis and new_end > self.analysis.duration:
            new_end = self.analysis.duration
            new_start = max(0, new_end - new_range)

        self.kalimba_widget.set_view_range(new_start, new_end)
        self._update_kalimba_scrollbar_from_view()

    @Slot()
    def _detect_pitches(self):
        """Detect pitches from audio and display in kalimba tab."""
        if not self.current_file or not self.analysis:
            return

        # Disable button during detection
        self.detect_pitch_btn.setEnabled(False)
        self.detect_pitch_btn.setText("Detecting...")

        # Show progress dialog with percentage
        self.pitch_progress = QProgressDialog("Initializing pitch detection...", "Cancel", 0, 100, self)
        self.pitch_progress.setWindowTitle("Pitch Detection")
        self.pitch_progress.setWindowModality(Qt.WindowModal)
        self.pitch_progress.setMinimumDuration(0)
        self.pitch_progress.setMinimumWidth(350)
        self.pitch_progress.setValue(0)
        self.pitch_progress.show()

        # Run in background thread
        self.pitch_worker = PitchDetectionWorker(self.analyzer, self.current_file)
        self.pitch_worker.progress.connect(self._on_pitch_progress)
        self.pitch_worker.finished.connect(self._on_pitch_detection_complete)
        self.pitch_worker.error.connect(self._on_pitch_detection_error)
        self.pitch_worker.start()

    def _on_pitch_progress(self, percent: int, message: str):
        """Update pitch detection progress."""
        if hasattr(self, 'pitch_progress') and self.pitch_progress:
            self.pitch_progress.setValue(percent)
            self.pitch_progress.setLabelText(message)
        self.statusBar().showMessage(f"Pitch detection: {message}")

    def _on_pitch_detection_complete(self, pitch_notes: list):
        """Handle completed pitch detection."""
        if hasattr(self, 'pitch_progress') and self.pitch_progress:
            self.pitch_progress.close()

        # Update kalimba widget
        self.kalimba_widget.set_pitch_notes(pitch_notes, self.analysis.duration)
        self.kalimba_widget.set_view_range(0, min(10.0, self.analysis.duration))

        # Store in analysis
        self.analysis.pitch_notes = pitch_notes

        # Re-enable button
        self.detect_pitch_btn.setEnabled(True)
        self.detect_pitch_btn.setText("Detect Pitches")

        # Show results
        self.statusBar().showMessage(f"Detected {len(pitch_notes)} notes for kalimba tab")

        # Switch to kalimba tab
        self.viz_tabs.setCurrentIndex(1)

        # Show summary
        if len(pitch_notes) > 0:
            # Count notes in kalimba range
            kalimba_notes = sum(1 for n in pitch_notes if 60 <= n.midi_note <= 88)
            QMessageBox.information(
                self,
                "Pitch Detection Complete",
                f"Detected {len(pitch_notes)} notes total.\n"
                f"{kalimba_notes} notes are within kalimba range (C4-E6).\n\n"
                f"Notes outside this range will be transposed automatically."
            )
        else:
            QMessageBox.warning(
                self,
                "No Pitches Detected",
                "Could not detect any clear pitches in the audio.\n\n"
                "This works best with:\n"
                "- Monophonic audio (single notes)\n"
                "- Clear melodies (kalimba, vocals, flute)\n"
                "- Less background noise"
            )

    def _on_pitch_detection_error(self, error: str):
        """Handle pitch detection error."""
        if hasattr(self, 'pitch_progress') and self.pitch_progress:
            self.pitch_progress.close()

        # Re-enable button
        self.detect_pitch_btn.setEnabled(True)
        self.detect_pitch_btn.setText("Detect Pitches")

        QMessageBox.critical(self, "Error", f"Failed to detect pitches:\n{error}")

    @Slot()
    def _update_preview(self):
        """Update export preview."""
        if not self.chart:
            self.preview_text.setText("Generate notes first to see preview")
            return

        template_name = self.preview_template_combo.currentData()
        if template_name:
            try:
                # Update chart metadata from current UI values
                self.chart.title = self.title_edit.text() or "Untitled"
                self.chart.artist = self.artist_edit.text() or "Unknown"

                preview = self.template_manager.export(self.chart, template_name)
                self.preview_text.setText(preview)
            except Exception as e:
                self.preview_text.setText(f"Error: {e}\n\n{traceback.format_exc()}")

    @Slot()
    def _export_chart(self):
        """Export chart to file."""
        if not self.chart:
            QMessageBox.warning(self, "Warning", "Generate notes first before exporting.")
            return

        template_name = self.template_combo.currentData()
        if not template_name:
            QMessageBox.warning(self, "Warning", "Please select a template.")
            return

        template = self.template_manager.get_template(template_name)
        if not template:
            QMessageBox.warning(self, "Warning", f"Template '{template_name}' not found.")
            return

        # Update chart metadata
        self.chart.title = self.title_edit.text() or "Untitled"
        self.chart.artist = self.artist_edit.text() or "Unknown"

        # Get save path
        default_name = f"{self.chart.title}{template.file_extension}"
        # Sanitize filename
        default_name = "".join(c for c in default_name if c not in r'<>:"/\|?*')

        file_path, _ = QFileDialog.getSaveFileName(
            self,
            "Export Chart",
            default_name,
            f"Chart Files (*{template.file_extension});;All Files (*)"
        )

        if not file_path:
            return

        try:
            content = self.template_manager.export(self.chart, template_name)
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(content)

            self.statusBar().showMessage(f"Exported to {file_path}")
            QMessageBox.information(self, "Success", f"Chart exported to:\n{file_path}")
        except Exception as e:
            error_msg = f"Failed to export:\n{e}\n\n{traceback.format_exc()}"
            QMessageBox.critical(self, "Error", error_msg)

    @Slot()
    def _import_chart(self):
        """Import a chart from file."""
        file_path, _ = QFileDialog.getOpenFileName(
            self,
            "Import Chart",
            "",
            "Chart Files (*.json *.yaml *.yml *.csv *.txt *.notes *.chart);;All Files (*)"
        )

        if not file_path:
            return

        try:
            chart = self.template_manager.import_chart(file_path)
            self.chart = chart

            # Update UI with imported data
            self.title_edit.setText(chart.title)
            self.artist_edit.setText(chart.artist)
            self.keys_spin.setValue(chart.num_keys)

            # Find and set difficulty in combo
            for i in range(self.difficulty_combo.count()):
                if self.difficulty_combo.itemData(i) == chart.difficulty:
                    self.difficulty_combo.setCurrentIndex(i)
                    break

            # Update visualization
            self.waveform_widget.set_chart(chart)
            self.lane_widget.set_num_keys(chart.num_keys)
            self.lane_widget.set_chart(chart)

            # Update stats
            self.tempo_label.setText(f"{chart.bpm:.1f}")
            self.notes_label.setText(str(len(chart.notes)))
            if chart.duration > 0:
                density = len(chart.notes) / chart.duration
                self.density_label.setText(f"{density:.2f}")
            else:
                self.density_label.setText("-")
            self.diff_value_label.setText(f"{chart.difficulty_value}/10")

            # Enable export
            self.export_btn.setEnabled(True)
            self._update_preview()

            # Check if we need to load audio file
            if chart.audio_file:
                self.statusBar().showMessage(
                    f"Imported chart with {len(chart.notes)} notes. Audio file: {chart.audio_file}"
                )
            else:
                self.statusBar().showMessage(f"Imported chart with {len(chart.notes)} notes")

            QMessageBox.information(
                self,
                "Import Successful",
                f"Imported {len(chart.notes)} notes.\n\n"
                f"Title: {chart.title}\n"
                f"Artist: {chart.artist}\n"
                f"BPM: {chart.bpm:.1f}\n"
                f"Keys: {chart.num_keys}\n"
                f"Difficulty: {chart.difficulty.name}"
            )

        except Exception as e:
            error_msg = f"Failed to import chart:\n{e}\n\n{traceback.format_exc()}"
            QMessageBox.critical(self, "Import Error", error_msg)

    @Slot()
    def _show_about(self):
        """Show about dialog."""
        QMessageBox.about(
            self,
            "About Rhythm Note Generator",
            "Rhythm Note Generator\n\n"
            "A tool for generating rhythm game note charts from audio files.\n\n"
            "Features:\n"
            "- Automatic beat and onset detection\n"
            "- Configurable keys (1-6) and difficulty\n"
            "- Visual waveform and note editing\n"
            "- Multiple export templates\n"
            "- Import/Export chart files\n"
            "- Note sound preview during playback\n"
        )

    def closeEvent(self, event):
        """Clean up on close."""
        pygame.mixer.quit()
        pygame.quit()
        event.accept()
