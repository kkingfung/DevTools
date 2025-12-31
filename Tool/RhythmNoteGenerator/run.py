#!/usr/bin/env python3
"""
Rhythm Note Generator - Main entry point.
A tool for generating rhythm game note charts from audio files.
"""

import sys
from pathlib import Path

# Add src to path
sys.path.insert(0, str(Path(__file__).parent / 'src'))

from PySide6.QtWidgets import QApplication
from PySide6.QtCore import Qt
from PySide6.QtGui import QPalette, QColor

from src.ui.main_window import MainWindow


def setup_dark_theme(app: QApplication):
    """Apply dark theme to application."""
    app.setStyle("Fusion")

    palette = QPalette()

    # Base colors
    palette.setColor(QPalette.Window, QColor(45, 45, 50))
    palette.setColor(QPalette.WindowText, QColor(220, 220, 220))
    palette.setColor(QPalette.Base, QColor(35, 35, 40))
    palette.setColor(QPalette.AlternateBase, QColor(45, 45, 50))
    palette.setColor(QPalette.ToolTipBase, QColor(25, 25, 30))
    palette.setColor(QPalette.ToolTipText, QColor(220, 220, 220))
    palette.setColor(QPalette.Text, QColor(220, 220, 220))
    palette.setColor(QPalette.Button, QColor(55, 55, 60))
    palette.setColor(QPalette.ButtonText, QColor(220, 220, 220))
    palette.setColor(QPalette.BrightText, QColor(255, 255, 255))
    palette.setColor(QPalette.Link, QColor(100, 150, 255))
    palette.setColor(QPalette.Highlight, QColor(80, 120, 200))
    palette.setColor(QPalette.HighlightedText, QColor(255, 255, 255))

    # Disabled colors
    palette.setColor(QPalette.Disabled, QPalette.WindowText, QColor(120, 120, 120))
    palette.setColor(QPalette.Disabled, QPalette.Text, QColor(120, 120, 120))
    palette.setColor(QPalette.Disabled, QPalette.ButtonText, QColor(120, 120, 120))

    app.setPalette(palette)


def main():
    """Main entry point."""
    # High DPI support
    QApplication.setHighDpiScaleFactorRoundingPolicy(
        Qt.HighDpiScaleFactorRoundingPolicy.PassThrough
    )

    app = QApplication(sys.argv)
    app.setApplicationName("Rhythm Note Generator")
    app.setOrganizationName("DevTools")

    # Apply dark theme
    setup_dark_theme(app)

    # Create and show main window
    window = MainWindow()
    window.show()

    sys.exit(app.exec())


if __name__ == "__main__":
    main()
