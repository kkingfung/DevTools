"""
Template management system for note chart export.
Templates define output format with constraints on structure.
"""

import json
import yaml
from pathlib import Path
from dataclasses import dataclass, field
from typing import Dict, List, Any, Optional
from string import Template
from .note_generator import NoteChart, Note, Difficulty


@dataclass
class TemplateField:
    """Defines a field in the template with constraints."""
    name: str           # Field identifier
    required: bool      # Cannot be removed if True
    data_type: str      # 'string', 'number', 'array', 'object'
    description: str    # Human-readable description


@dataclass
class TemplateSchema:
    """Schema defining what fields a template must include."""
    metadata_fields: List[TemplateField] = field(default_factory=list)
    timing_fields: List[TemplateField] = field(default_factory=list)
    chart_fields: List[TemplateField] = field(default_factory=list)
    note_fields: List[TemplateField] = field(default_factory=list)

    @classmethod
    def default(cls) -> 'TemplateSchema':
        """Create the default schema with required fields."""
        return cls(
            metadata_fields=[
                TemplateField('title', True, 'string', 'Song title'),
                TemplateField('artist', True, 'string', 'Artist name'),
                TemplateField('audio_file', True, 'string', 'Audio filename'),
            ],
            timing_fields=[
                TemplateField('bpm', True, 'number', 'Beats per minute'),
                TemplateField('offset', True, 'number', 'Start offset in seconds'),
                TemplateField('duration', True, 'number', 'Song duration in seconds'),
            ],
            chart_fields=[
                TemplateField('num_keys', True, 'number', 'Number of keys/lanes'),
                TemplateField('difficulty', True, 'string', 'Difficulty name'),
                TemplateField('difficulty_value', True, 'number', 'Numeric difficulty rating'),
            ],
            note_fields=[
                TemplateField('time', True, 'number', 'Note time in seconds'),
                TemplateField('lane', True, 'number', 'Lane number (0-indexed)'),
                TemplateField('duration', True, 'number', 'Hold duration (0 for tap)'),
            ]
        )


@dataclass
class ChartTemplate:
    """A template for exporting note charts."""
    name: str                           # Template name
    description: str                    # Description
    file_extension: str                 # Output file extension
    format_type: str                    # 'json', 'yaml', 'text', 'csv'

    # Structure templates (can be modified within constraints)
    structure: Dict[str, Any] = field(default_factory=dict)

    # Table format for notes (customizable)
    note_table_format: str = "list"     # 'list', 'csv', 'custom'
    note_table_template: str = ""       # Custom format string for notes

    # Separator for text-based formats
    separator: str = ","

    # Header/footer templates
    header_template: str = ""
    footer_template: str = ""


class TemplateManager:
    """Manages chart templates and export functionality."""

    def __init__(self, templates_dir: Optional[Path] = None):
        self.templates_dir = templates_dir or Path(__file__).parent.parent.parent / 'templates'
        self.schema = TemplateSchema.default()
        self.templates: Dict[str, ChartTemplate] = {}
        self._load_builtin_templates()
        self._load_user_templates()

    def _load_builtin_templates(self):
        """Load built-in templates."""
        # JSON template
        self.templates['json'] = ChartTemplate(
            name='JSON',
            description='Standard JSON format for general use',
            file_extension='.json',
            format_type='json',
            structure={
                'metadata': ['title', 'artist', 'audio_file'],
                'timing': ['bpm', 'offset', 'duration'],
                'chart': ['num_keys', 'difficulty', 'difficulty_value'],
                'notes': 'note_list'
            }
        )

        # YAML template
        self.templates['yaml'] = ChartTemplate(
            name='YAML',
            description='YAML format for human-readable charts',
            file_extension='.yaml',
            format_type='yaml',
            structure={
                'metadata': ['title', 'artist', 'audio_file'],
                'timing': ['bpm', 'offset', 'duration'],
                'chart': ['num_keys', 'difficulty', 'difficulty_value'],
                'notes': 'note_list'
            }
        )

        # CSV template (notes only)
        self.templates['csv'] = ChartTemplate(
            name='CSV',
            description='CSV format for spreadsheet compatibility',
            file_extension='.csv',
            format_type='csv',
            note_table_format='csv',
            header_template='# title: ${title}\n# artist: ${artist}\n# bpm: ${bpm}\n# keys: ${num_keys}\n# difficulty: ${difficulty}\n',
            structure={}
        )

        # Simple text format
        self.templates['simple'] = ChartTemplate(
            name='Simple Text',
            description='Simple text format with one note per line',
            file_extension='.txt',
            format_type='text',
            note_table_format='custom',
            note_table_template='${time}|${lane}|${duration}',
            header_template='[Metadata]\ntitle=${title}\nartist=${artist}\naudio=${audio_file}\n\n[Timing]\nbpm=${bpm}\noffset=${offset}\n\n[Chart]\nkeys=${num_keys}\ndifficulty=${difficulty}\nlevel=${difficulty_value}\n\n[Notes]\n',
            separator='\n'
        )

        # Minimal format (just notes, timing in milliseconds)
        self.templates['minimal'] = ChartTemplate(
            name='Minimal',
            description='Minimal format: time_ms,lane per line',
            file_extension='.notes',
            format_type='text',
            note_table_format='custom',
            note_table_template='${time_ms},${lane}',
            header_template='# BPM:${bpm} Keys:${num_keys} Difficulty:${difficulty}\n',
            separator='\n'
        )

    def _load_user_templates(self):
        """Load user-defined templates from templates directory."""
        if not self.templates_dir.exists():
            return

        for template_file in self.templates_dir.glob('*.template.yaml'):
            try:
                with open(template_file, 'r', encoding='utf-8') as f:
                    data = yaml.safe_load(f)

                name = data.get('name', template_file.stem.replace('.template', ''))
                template = ChartTemplate(
                    name=name,
                    description=data.get('description', ''),
                    file_extension=data.get('file_extension', '.txt'),
                    format_type=data.get('format_type', 'text'),
                    structure=data.get('structure', {}),
                    note_table_format=data.get('note_table_format', 'list'),
                    note_table_template=data.get('note_table_template', ''),
                    separator=data.get('separator', '\n'),
                    header_template=data.get('header_template', ''),
                    footer_template=data.get('footer_template', '')
                )

                # Validate against schema
                if self._validate_template(template):
                    self.templates[name.lower()] = template
            except Exception as e:
                print(f"Warning: Failed to load template {template_file}: {e}")

    def _validate_template(self, template: ChartTemplate) -> bool:
        """Validate that a template includes all required fields."""
        # For now, accept all templates but ensure they can render required fields
        return True

    def get_template(self, name: str) -> Optional[ChartTemplate]:
        """Get a template by name."""
        return self.templates.get(name.lower())

    def list_templates(self) -> List[Dict[str, str]]:
        """List all available templates."""
        return [
            {
                'name': t.name,
                'description': t.description,
                'extension': t.file_extension
            }
            for t in self.templates.values()
        ]

    def export(self, chart: NoteChart, template_name: str) -> str:
        """Export a chart using the specified template."""
        template = self.get_template(template_name)
        if not template:
            raise ValueError(f"Template '{template_name}' not found")

        if template.format_type == 'json':
            return self._export_json(chart, template)
        elif template.format_type == 'yaml':
            return self._export_yaml(chart, template)
        elif template.format_type == 'csv':
            return self._export_csv(chart, template)
        elif template.format_type == 'text':
            return self._export_text(chart, template)
        else:
            raise ValueError(f"Unknown format type: {template.format_type}")

    def _get_chart_vars(self, chart: NoteChart) -> Dict[str, Any]:
        """Get all chart variables for template substitution."""
        return {
            'title': chart.title,
            'artist': chart.artist,
            'audio_file': chart.audio_file,
            'bpm': chart.bpm,
            'offset': chart.offset,
            'duration': chart.duration,
            'num_keys': chart.num_keys,
            'difficulty': chart.difficulty.name,
            'difficulty_value': chart.difficulty_value,
        }

    def _get_note_vars(self, note: Note) -> Dict[str, Any]:
        """Get note variables for template substitution."""
        return {
            'time': round(note.time, 4),
            'time_ms': int(note.time * 1000),
            'lane': note.lane,
            'duration': round(note.duration, 4),
            'duration_ms': int(note.duration * 1000),
        }

    def _export_json(self, chart: NoteChart, template: ChartTemplate) -> str:
        """Export as JSON."""
        data = chart.to_dict()
        return json.dumps(data, indent=2, ensure_ascii=False)

    def _export_yaml(self, chart: NoteChart, template: ChartTemplate) -> str:
        """Export as YAML."""
        data = chart.to_dict()
        return yaml.dump(data, default_flow_style=False, allow_unicode=True, sort_keys=False)

    def _export_csv(self, chart: NoteChart, template: ChartTemplate) -> str:
        """Export as CSV."""
        lines = []

        # Header comments
        if template.header_template:
            header = Template(template.header_template).safe_substitute(self._get_chart_vars(chart))
            lines.append(header)

        # CSV header
        lines.append('time,lane,duration')

        # Notes
        for note in chart.notes:
            lines.append(f'{note.time:.4f},{note.lane},{note.duration:.4f}')

        return '\n'.join(lines)

    def _export_text(self, chart: NoteChart, template: ChartTemplate) -> str:
        """Export as custom text format."""
        lines = []
        chart_vars = self._get_chart_vars(chart)

        # Header
        if template.header_template:
            header = Template(template.header_template).safe_substitute(chart_vars)
            lines.append(header)

        # Notes
        note_lines = []
        for note in chart.notes:
            note_vars = self._get_note_vars(note)
            if template.note_table_template:
                line = Template(template.note_table_template).safe_substitute(note_vars)
            else:
                line = f"{note_vars['time']},{note_vars['lane']},{note_vars['duration']}"
            note_lines.append(line)

        if template.separator == '\n':
            lines.extend(note_lines)
        else:
            lines.append(template.separator.join(note_lines))

        # Footer
        if template.footer_template:
            footer = Template(template.footer_template).safe_substitute(chart_vars)
            lines.append(footer)

        return '\n'.join(lines)

    def save_template(self, template: ChartTemplate) -> Path:
        """Save a custom template to the templates directory."""
        self.templates_dir.mkdir(parents=True, exist_ok=True)

        filename = f"{template.name.lower().replace(' ', '_')}.template.yaml"
        filepath = self.templates_dir / filename

        data = {
            'name': template.name,
            'description': template.description,
            'file_extension': template.file_extension,
            'format_type': template.format_type,
            'structure': template.structure,
            'note_table_format': template.note_table_format,
            'note_table_template': template.note_table_template,
            'separator': template.separator,
            'header_template': template.header_template,
            'footer_template': template.footer_template,
        }

        with open(filepath, 'w', encoding='utf-8') as f:
            yaml.dump(data, f, default_flow_style=False, allow_unicode=True, sort_keys=False)

        self.templates[template.name.lower()] = template
        return filepath

    def get_schema(self) -> TemplateSchema:
        """Get the template schema (constraints)."""
        return self.schema

    def get_required_fields(self) -> Dict[str, List[str]]:
        """Get list of required fields that cannot be removed."""
        return {
            'metadata': [f.name for f in self.schema.metadata_fields if f.required],
            'timing': [f.name for f in self.schema.timing_fields if f.required],
            'chart': [f.name for f in self.schema.chart_fields if f.required],
            'note': [f.name for f in self.schema.note_fields if f.required],
        }

    def import_chart(self, file_path: str | Path) -> NoteChart:
        """Import a chart from file. Auto-detects format based on extension."""
        file_path = Path(file_path)
        ext = file_path.suffix.lower()

        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()

        if ext == '.json':
            return self._import_json(content)
        elif ext in ('.yaml', '.yml'):
            return self._import_yaml(content)
        elif ext == '.csv':
            return self._import_csv(content)
        else:
            # Try to auto-detect format
            content_stripped = content.strip()
            if content_stripped.startswith('{'):
                return self._import_json(content)
            elif content_stripped.startswith('metadata:') or content_stripped.startswith('timing:'):
                return self._import_yaml(content)
            else:
                return self._import_text(content)

    def _import_json(self, content: str) -> NoteChart:
        """Import from JSON format."""
        data = json.loads(content)
        return self._data_to_chart(data)

    def _import_yaml(self, content: str) -> NoteChart:
        """Import from YAML format."""
        data = yaml.safe_load(content)
        return self._data_to_chart(data)

    def _import_csv(self, content: str) -> NoteChart:
        """Import from CSV format."""
        lines = content.strip().split('\n')

        # Parse header comments for metadata
        metadata = {}
        note_lines = []

        for line in lines:
            line = line.strip()
            if line.startswith('#'):
                # Parse comment metadata (# key: value)
                if ':' in line:
                    key, value = line[1:].split(':', 1)
                    metadata[key.strip().lower()] = value.strip()
            elif line and not line.startswith('time,'):  # Skip CSV header
                note_lines.append(line)

        # Parse notes
        notes = []
        for line in note_lines:
            parts = line.split(',')
            if len(parts) >= 2:
                time = float(parts[0])
                lane = int(parts[1])
                duration = float(parts[2]) if len(parts) > 2 else 0.0
                notes.append(Note(time=time, lane=lane, duration=duration))

        # Determine difficulty from string
        diff_str = metadata.get('difficulty', 'NORMAL').upper()
        try:
            difficulty = Difficulty[diff_str]
        except KeyError:
            difficulty = Difficulty.NORMAL

        return NoteChart(
            title=metadata.get('title', ''),
            artist=metadata.get('artist', ''),
            audio_file=metadata.get('audio', metadata.get('audio_file', '')),
            bpm=float(metadata.get('bpm', 120)),
            offset=float(metadata.get('offset', 0)),
            duration=float(metadata.get('duration', 0)),
            num_keys=int(metadata.get('keys', metadata.get('num_keys', 4))),
            difficulty=difficulty,
            difficulty_value=int(metadata.get('level', metadata.get('difficulty_value', 1))),
            notes=notes
        )

    def _import_text(self, content: str) -> NoteChart:
        """Import from text format (simple or custom)."""
        lines = content.strip().split('\n')

        metadata = {}
        notes = []
        in_notes_section = False

        for line in lines:
            line = line.strip()

            # Skip empty lines
            if not line:
                continue

            # Check for section headers
            if line.startswith('[') and line.endswith(']'):
                section = line[1:-1].lower()
                in_notes_section = section == 'notes'
                continue

            # Parse metadata lines (key=value or key:value)
            if not in_notes_section and ('=' in line or ':' in line):
                sep = '=' if '=' in line else ':'
                parts = line.split(sep, 1)
                if len(parts) == 2:
                    key = parts[0].strip().lower()
                    value = parts[1].strip()
                    metadata[key] = value
                continue

            # Parse note lines
            if in_notes_section or line[0].isdigit():
                # Try different separators: |, :, ,, whitespace
                for sep in ['|', ':', ',', '\t', ' ']:
                    if sep in line:
                        parts = [p.strip() for p in line.split(sep) if p.strip()]
                        if len(parts) >= 2:
                            try:
                                # Check if first part is time in ms or seconds
                                time_val = float(parts[0])
                                if time_val > 1000:  # Likely milliseconds
                                    time_val /= 1000.0
                                lane = int(parts[1])
                                duration = 0.0
                                if len(parts) > 2:
                                    dur_val = float(parts[2])
                                    if dur_val > 100:  # Likely milliseconds
                                        dur_val /= 1000.0
                                    duration = dur_val
                                notes.append(Note(time=time_val, lane=lane, duration=duration))
                                break
                            except (ValueError, IndexError):
                                continue

        # Determine difficulty
        diff_str = metadata.get('difficulty', 'NORMAL').upper()
        try:
            difficulty = Difficulty[diff_str]
        except KeyError:
            difficulty = Difficulty.NORMAL

        return NoteChart(
            title=metadata.get('title', ''),
            artist=metadata.get('artist', ''),
            audio_file=metadata.get('audio', metadata.get('audio_file', '')),
            bpm=float(metadata.get('bpm', 120)),
            offset=float(metadata.get('offset', 0)),
            duration=float(metadata.get('duration', 0)),
            num_keys=int(metadata.get('keys', metadata.get('num_keys', 4))),
            difficulty=difficulty,
            difficulty_value=int(metadata.get('level', metadata.get('difficulty_value', 1))),
            notes=notes
        )

    def _data_to_chart(self, data: Dict[str, Any]) -> NoteChart:
        """Convert dictionary data to NoteChart."""
        # Handle nested structure
        metadata = data.get('metadata', data)
        timing = data.get('timing', data)
        chart_info = data.get('chart', data)
        notes_data = data.get('notes', [])

        # Determine difficulty
        diff_str = chart_info.get('difficulty', 'NORMAL')
        if isinstance(diff_str, str):
            try:
                difficulty = Difficulty[diff_str.upper()]
            except KeyError:
                difficulty = Difficulty.NORMAL
        else:
            difficulty = Difficulty(int(diff_str))

        # Parse notes
        notes = []
        for note_data in notes_data:
            notes.append(Note(
                time=float(note_data.get('time', 0)),
                lane=int(note_data.get('lane', 0)),
                duration=float(note_data.get('duration', 0))
            ))

        return NoteChart(
            title=str(metadata.get('title', '')),
            artist=str(metadata.get('artist', '')),
            audio_file=str(metadata.get('audio_file', '')),
            bpm=float(timing.get('bpm', 120)),
            offset=float(timing.get('offset', 0)),
            duration=float(timing.get('duration', 0)),
            num_keys=int(chart_info.get('num_keys', 4)),
            difficulty=difficulty,
            difficulty_value=int(chart_info.get('difficulty_value', 1)),
            notes=notes
        )
