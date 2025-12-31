# リズムノートジェネレーター

音声ファイルからリズムゲーム用のノート譜面を自動生成するツールです。

## 機能

- **音声解析**: librosaを使用した自動ビート・オンセット検出
- **ピッチ検出**: 音声からド・レ・ミを自動検出（カリンバタブ生成用）
- **ノート生成**: 音声解析に基づいたプレイ可能なノートパターンの生成
- **設定可能なパラメータ**:
  - 1〜6キーモード
  - 5段階の難易度（Easy, Normal, Hard, Expert, Master）
- **ビジュアルインターフェース**:
  - ズーム可能な波形表示
  - ピアノロール式ノートエディタ
  - スクロールバーによるタイムライン操作
  - ビジュアライゼーション付きリアルタイム再生
  - ノート音プレビュー（レーンごとに異なる音程）
- **カリンバタブ機能**:
  - 17キーCメジャーカリンバ対応
  - 音声からピッチを自動検出
  - カリンバタブ形式で表示
  - 範囲外の音は自動的にオクターブ変換
- **テンプレートシステム**:
  - 内蔵エクスポートテンプレート（JSON, YAML, CSV, Text, Minimal）
  - カスタムテンプレート対応
  - 制約機能: テーブル構造は変更可能、必須フィールドは削除不可
- **インポート/エクスポート**:
  - 複数フォーマットでエクスポート
  - エクスポートしたファイルを再インポート可能

## インストール

1. Python 3.10以上をインストール

2. 依存パッケージをインストール:
```bash
pip install -r requirements.txt
```

3. （任意）MP3対応のためFFmpegをインストール:
   - Windows: https://ffmpeg.org/ からダウンロードしてPATHに追加
   - または conda: `conda install ffmpeg`

## 使い方

アプリケーションを起動:
```bash
python run.py
```

### 基本的なワークフロー

1. **音声読み込み**: 「Load Audio File...」をクリックしてWAV、MP3、OGG、FLACファイルを選択
2. **設定**:
   - キー数を設定（1〜6）
   - 難易度を選択
   - 曲名とアーティスト名を入力
3. **ノート生成**: 「Generate Notes」をクリックしてノート譜面を作成
4. **編集（任意）**:
   - 右クリックでノートを追加
   - ノートを選択してDeleteキーで削除
   - ドラッグでノートを移動
5. **エクスポート**: テンプレートを選択して「Export Chart...」をクリック

### カリンバタブの作成

1. 音声ファイルを読み込み
2. 「Kalimba Tab」タブを選択
3. 「Detect Pitches」ボタンをクリック
4. ピッチ検出が完了すると、カリンバタブが表示されます

**ピッチ検出に最適な音声**:
- カリンバ録音（最高精度）
- ボーカル（アカペラ）
- フルート、ホイッスル
- ピアノソロ

**検出が難しい音声**:
- フルバンドミックス
- ドラムが強い曲
- 複数楽器の同時演奏

### キーボードショートカット

- `Ctrl+O`: 音声ファイルを開く
- `Ctrl+I`: 譜面をインポート
- `Ctrl+S`: 譜面をエクスポート
- `Ctrl+R`: ノートを再生成
- `Delete/Backspace`: 選択中のノートを削除
- マウスホイール: タイムラインをズーム

### 難易度システム

| 難易度 | ビート優先度 | ノート密度 | 説明 |
|--------|-------------|-----------|------|
| Easy | 100% | 低 | ビートのみ使用、シンプル |
| Normal | 80% | 中低 | ビート中心、一部オンセット |
| Hard | 60% | 中 | ビートとオンセットのバランス |
| Expert | 40% | 高 | オンセット多め |
| Master | 20% | 最高 | オンセット中心、複雑 |

## カスタムテンプレート

`templates/`フォルダに`.template.yaml`ファイルを配置することでカスタムテンプレートを作成できます。

テンプレート構造の例:
```yaml
name: "マイフォーマット"
description: "カスタムフォーマットの説明"
file_extension: ".myformat"
format_type: "text"  # json, yaml, csv, または text

note_table_format: "custom"
note_table_template: "${time_ms}:${lane}"

header_template: |
  [HEADER]
  Title: ${title}
  BPM: ${bpm}

  [NOTES]

footer_template: |
  [END]
```

### 利用可能な変数

**譜面変数**: title, artist, audio_file, bpm, offset, duration, num_keys, difficulty, difficulty_value

**ノート変数**: time, time_ms, lane, duration, duration_ms

## プロジェクト構成

```
RhythmNoteGenerator/
├── run.py                    # エントリーポイント
├── requirements.txt          # 依存パッケージ
├── templates/                # エクスポートテンプレート
│   └── example_custom.template.yaml
└── src/
    ├── core/
    │   ├── audio_analyzer.py # ビート・オンセット・ピッチ検出
    │   ├── note_generator.py # ノート譜面生成
    │   └── template_manager.py # エクスポートテンプレート管理
    └── ui/
        ├── main_window.py    # メインウィンドウ
        ├── waveform_widget.py # 波形表示
        ├── note_lane_widget.py # ピアノロールエディタ
        └── kalimba_widget.py # カリンバタブ表示
```

## 依存パッケージ

- **librosa**: 音声解析（ビート・オンセット・ピッチ検出）
- **PySide6**: Qt ベースUI
- **numpy/scipy**: 数値処理
- **pygame**: 音声再生・ノート音生成
- **pyyaml**: テンプレート解析
- **pydub**: 追加音声フォーマット対応
