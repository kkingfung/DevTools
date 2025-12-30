# GameDev Scheduler

ゲーム開発チーム向けのスケジュール管理ツールです。タスクの作成・管理、ガントチャート表示、カレンダー表示をサポートしています。

## 技術スタック

- **Frontend**: React + TypeScript + Vite
- **Backend**: Tauri 2.0 (Rust)
- **Database**: SQLite (rusqlite)
- **UI**: Ant Design
- **State Management**: Zustand
- **Calendar**: FullCalendar

## 機能

### チーム管理
- チームの作成・選択
- チームごとにメンバー、カテゴリ、タスクを管理

### メンバー管理
- メンバーの追加（名前、役職）
- タスクへのメンバー割り当て

### カテゴリ管理
- カテゴリの追加（名前、色）
- デフォルトカテゴリ（proposal, draft, wireframe, ui instruction, vfx instruction, sound instruction, dm/vo, server, client, qa）

### タスク管理
- タスクの作成・編集・削除
- タスクステータス（Not Started, In Progress, Completed）
- 開始日・終了日の設定
- メンバー・カテゴリの割り当て

### ビュー切り替え
- **ガントチャート**: タスクのタイムラインを視覚的に表示
- **カレンダー**: 月/週単位でタスクを表示
- **テーブル**: タスク一覧を表形式で表示・編集

### フィルタリング
- メンバーでフィルタ
- カテゴリでフィルタ
- 日付範囲でフィルタ

### データエクスポート
- チームデータをJSONファイルとしてエクスポート

## 開発環境のセットアップ

### 必要条件
- Node.js 18+
- Rust 1.70+
- npm

### インストール

```bash
cd Tool/GameDevScheduler
npm install
```

### 開発サーバーの起動

```bash
npm run tauri dev
```

### ビルド

```bash
npm run tauri build
```

## ディレクトリ構造

```
GameDevScheduler/
├── src/                    # React フロントエンド
│   ├── components/         # UIコンポーネント
│   │   ├── CalendarView/   # カレンダービュー
│   │   ├── FilterSidebar/  # フィルターサイドバー
│   │   ├── GanttView/      # ガントチャート
│   │   └── TableView/      # テーブルビュー
│   ├── store/              # Zustand ストア
│   ├── types/              # TypeScript 型定義
│   ├── App.tsx             # メインアプリケーション
│   └── App.css             # スタイル
├── src-tauri/              # Rust バックエンド
│   ├── src/
│   │   ├── database.rs     # SQLite データベース操作
│   │   ├── models.rs       # データモデル
│   │   └── lib.rs          # Tauri コマンド
│   └── Cargo.toml          # Rust 依存関係
└── package.json            # npm 依存関係
```

## ライセンス

MIT License

## データのエクスポート/インポート

### エクスポート
Settings > Export Data からチームデータをJSONファイルとしてエクスポートできます。

エクスポートファイルの形式:
```json
{
  "team": {
    "id": "uuid",
    "name": "Team Name",
    "created_at": "2024-01-01T00:00:00Z"
  },
  "members": [
    {
      "id": "uuid",
      "team_id": "uuid",
      "name": "Member Name",
      "role": "developer",
      "color": "#4A90D9",
      "created_at": "2024-01-01T00:00:00Z"
    }
  ],
  "categories": [
    {
      "id": "uuid",
      "team_id": "uuid",
      "name": "Category Name",
      "color": "#FF5733",
      "order_index": 0,
      "created_at": "2024-01-01T00:00:00Z"
    }
  ],
  "tasks": [
    {
      "id": "uuid",
      "team_id": "uuid",
      "member_id": "uuid or null",
      "category_id": "uuid or null",
      "title": "Task Title",
      "description": "Task description",
      "start_date": "2024-01-01",
      "end_date": "2024-01-07",
      "status": "not_started|in_progress|completed",
      "created_at": "2024-01-01T00:00:00Z",
      "updated_at": "2024-01-01T00:00:00Z"
    }
  ],
  "exported_at": "2024-01-01T00:00:00Z"
}
```

### インポート
Settings > Import Data からJSONファイルをインポートできます。

**注意:**
- インポートは現在のチームに対してメンバー、カテゴリ、タスクを追加します（既存データは削除されません）
- インポート時にメンバー/カテゴリとタスクの関連付けはリセットされます
- チーム間でデータを移行する場合は、まず新しいチームを作成してからインポートしてください
