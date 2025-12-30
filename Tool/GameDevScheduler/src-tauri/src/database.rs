use crate::models::*;
use chrono::Utc;
use log::info;
use once_cell::sync::Lazy;
use rusqlite::{params, Connection, Result};
use std::path::PathBuf;
use std::sync::Mutex;
use uuid::Uuid;

static DB: Lazy<Mutex<Connection>> = Lazy::new(|| {
    info!("Initializing database...");
    let conn = init_db().expect("Failed to initialize database");
    info!("Database connection established");
    Mutex::new(conn)
});

/// Ensure database is initialized (triggers lazy initialization)
pub fn init_db_public() -> std::result::Result<(), String> {
    info!("Ensuring database is initialized...");
    // Access DB to trigger lazy initialization
    let _guard = DB.lock().map_err(|e| format!("Failed to lock DB: {}", e))?;
    info!("Database initialized successfully");
    Ok(())
}

fn get_db_path() -> PathBuf {
    let mut path = dirs_next::data_local_dir().unwrap_or_else(|| PathBuf::from("."));
    path.push("GameDevScheduler");
    std::fs::create_dir_all(&path).ok();
    path.push("scheduler.db");
    path
}
fn init_db() -> Result<Connection> {
    let path = get_db_path();
    let conn = Connection::open(path)?;

    conn.execute_batch(
        "
        CREATE TABLE IF NOT EXISTS teams (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            created_at TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS members (
            id TEXT PRIMARY KEY,
            team_id TEXT NOT NULL,
            name TEXT NOT NULL,
            role TEXT NOT NULL DEFAULT 'member',
            color TEXT NOT NULL DEFAULT '#4A90D9',
            created_at TEXT NOT NULL,
            FOREIGN KEY (team_id) REFERENCES teams(id) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS categories (
            id TEXT PRIMARY KEY,
            team_id TEXT NOT NULL,
            name TEXT NOT NULL,
            color TEXT NOT NULL DEFAULT '#4A90D9',
            order_index INTEGER NOT NULL DEFAULT 0,
            created_at TEXT NOT NULL,
            FOREIGN KEY (team_id) REFERENCES teams(id) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS tasks (
            id TEXT PRIMARY KEY,
            team_id TEXT NOT NULL,
            member_id TEXT,
            category_id TEXT,
            title TEXT NOT NULL,
            description TEXT DEFAULT '',
            start_date TEXT NOT NULL,
            end_date TEXT NOT NULL,
            status TEXT NOT NULL DEFAULT 'not_started',
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL,
            FOREIGN KEY (team_id) REFERENCES teams(id) ON DELETE CASCADE,
            FOREIGN KEY (member_id) REFERENCES members(id) ON DELETE SET NULL,
            FOREIGN KEY (category_id) REFERENCES categories(id) ON DELETE SET NULL
        );

        CREATE INDEX IF NOT EXISTS idx_tasks_team ON tasks(team_id);
        CREATE INDEX IF NOT EXISTS idx_tasks_member ON tasks(member_id);
        CREATE INDEX IF NOT EXISTS idx_tasks_category ON tasks(category_id);
        CREATE INDEX IF NOT EXISTS idx_members_team ON members(team_id);
        CREATE INDEX IF NOT EXISTS idx_categories_team ON categories(team_id);
        ",
    )?;

    Ok(conn)
}

pub fn with_db<F, T>(f: F) -> Result<T, String>
where
    F: FnOnce(&Connection) -> Result<T>,
{
    let conn = DB.lock().map_err(|e| e.to_string())?;
    f(&conn).map_err(|e| e.to_string())
}

// Team operations
pub fn create_team(data: CreateTeam) -> Result<Team, String> {
    with_db(|conn| {
        let id = Uuid::new_v4().to_string();
        let now = Utc::now().to_rfc3339();
        conn.execute(
            "INSERT INTO teams (id, name, created_at) VALUES (?1, ?2, ?3)",
            params![id, data.name, now],
        )?;
        Ok(Team {
            id,
            name: data.name,
            created_at: now,
        })
    })
}

pub fn get_teams() -> Result<Vec<Team>, String> {
    with_db(|conn| {
        let mut stmt = conn.prepare("SELECT id, name, created_at FROM teams ORDER BY created_at")?;
        let teams = stmt
            .query_map([], |row| {
                Ok(Team {
                    id: row.get(0)?,
                    name: row.get(1)?,
                    created_at: row.get(2)?,
                })
            })?
            .collect::<Result<Vec<_>>>()?;
        Ok(teams)
    })
}

pub fn delete_team(id: &str) -> Result<(), String> {
    with_db(|conn| {
        conn.execute("DELETE FROM teams WHERE id = ?1", params![id])?;
        Ok(())
    })
}

pub fn update_team(id: &str, name: &str) -> Result<(), String> {
    with_db(|conn| {
        conn.execute("UPDATE teams SET name = ?1 WHERE id = ?2", params![name, id])?;
        Ok(())
    })
}

// Member operations
pub fn create_member(data: CreateMember) -> Result<Member, String> {
    with_db(|conn| {
        let id = Uuid::new_v4().to_string();
        let now = Utc::now().to_rfc3339();
        conn.execute(
            "INSERT INTO members (id, team_id, name, role, color, created_at) VALUES (?1, ?2, ?3, ?4, ?5, ?6)",
            params![id, data.team_id, data.name, data.role, data.color, now],
        )?;
        Ok(Member {
            id,
            team_id: data.team_id,
            name: data.name,
            role: data.role,
            color: data.color,
            created_at: now,
        })
    })
}

pub fn get_members(team_id: &str) -> Result<Vec<Member>, String> {
    with_db(|conn| {
        let mut stmt = conn.prepare(
            "SELECT id, team_id, name, role, color, created_at FROM members WHERE team_id = ?1 ORDER BY created_at",
        )?;
        let members = stmt
            .query_map(params![team_id], |row| {
                Ok(Member {
                    id: row.get(0)?,
                    team_id: row.get(1)?,
                    name: row.get(2)?,
                    role: row.get(3)?,
                    color: row.get(4)?,
                    created_at: row.get(5)?,
                })
            })?
            .collect::<Result<Vec<_>>>()?;
        Ok(members)
    })
}

pub fn update_member(data: UpdateMember) -> Result<(), String> {
    with_db(|conn| {
        conn.execute(
            "UPDATE members SET name = ?1, role = ?2, color = ?3 WHERE id = ?4",
            params![data.name, data.role, data.color, data.id],
        )?;
        Ok(())
    })
}

pub fn delete_member(id: &str) -> Result<(), String> {
    with_db(|conn| {
        conn.execute("DELETE FROM members WHERE id = ?1", params![id])?;
        Ok(())
    })
}

// Category operations
pub fn create_category(data: CreateCategory) -> Result<Category, String> {
    with_db(|conn| {
        let id = Uuid::new_v4().to_string();
        let now = Utc::now().to_rfc3339();
        let order_index: i32 = conn.query_row(
            "SELECT COALESCE(MAX(order_index), -1) + 1 FROM categories WHERE team_id = ?1",
            params![data.team_id],
            |row| row.get(0),
        )?;
        conn.execute(
            "INSERT INTO categories (id, team_id, name, color, order_index, created_at) VALUES (?1, ?2, ?3, ?4, ?5, ?6)",
            params![id, data.team_id, data.name, data.color, order_index, now],
        )?;
        Ok(Category {
            id,
            team_id: data.team_id,
            name: data.name,
            color: data.color,
            order_index,
            created_at: now,
        })
    })
}

pub fn get_categories(team_id: &str) -> Result<Vec<Category>, String> {
    with_db(|conn| {
        let mut stmt = conn.prepare(
            "SELECT id, team_id, name, color, order_index, created_at FROM categories WHERE team_id = ?1 ORDER BY order_index",
        )?;
        let categories = stmt
            .query_map(params![team_id], |row| {
                Ok(Category {
                    id: row.get(0)?,
                    team_id: row.get(1)?,
                    name: row.get(2)?,
                    color: row.get(3)?,
                    order_index: row.get(4)?,
                    created_at: row.get(5)?,
                })
            })?
            .collect::<Result<Vec<_>>>()?;
        Ok(categories)
    })
}

pub fn update_category(data: UpdateCategory) -> Result<(), String> {
    with_db(|conn| {
        conn.execute(
            "UPDATE categories SET name = ?1, color = ?2, order_index = ?3 WHERE id = ?4",
            params![data.name, data.color, data.order_index, data.id],
        )?;
        Ok(())
    })
}

pub fn delete_category(id: &str) -> Result<(), String> {
    with_db(|conn| {
        conn.execute("DELETE FROM categories WHERE id = ?1", params![id])?;
        Ok(())
    })
}

// Task operations
pub fn create_task(data: CreateTask) -> Result<Task, String> {
    with_db(|conn| {
        let id = Uuid::new_v4().to_string();
        let now = Utc::now().to_rfc3339();
        conn.execute(
            "INSERT INTO tasks (id, team_id, member_id, category_id, title, description, start_date, end_date, status, created_at, updated_at)
             VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11)",
            params![id, data.team_id, data.member_id, data.category_id, data.title, data.description, data.start_date, data.end_date, data.status, now, now],
        )?;
        Ok(Task {
            id,
            team_id: data.team_id,
            member_id: data.member_id,
            category_id: data.category_id,
            title: data.title,
            description: data.description,
            start_date: data.start_date,
            end_date: data.end_date,
            status: data.status,
            created_at: now.clone(),
            updated_at: now,
        })
    })
}

pub fn get_tasks(team_id: &str) -> Result<Vec<Task>, String> {
    with_db(|conn| {
        let mut stmt = conn.prepare(
            "SELECT id, team_id, member_id, category_id, title, description, start_date, end_date, status, created_at, updated_at
             FROM tasks WHERE team_id = ?1 ORDER BY start_date",
        )?;
        let tasks = stmt
            .query_map(params![team_id], |row| {
                Ok(Task {
                    id: row.get(0)?,
                    team_id: row.get(1)?,
                    member_id: row.get(2)?,
                    category_id: row.get(3)?,
                    title: row.get(4)?,
                    description: row.get(5)?,
                    start_date: row.get(6)?,
                    end_date: row.get(7)?,
                    status: row.get(8)?,
                    created_at: row.get(9)?,
                    updated_at: row.get(10)?,
                })
            })?
            .collect::<Result<Vec<_>>>()?;
        Ok(tasks)
    })
}

pub fn update_task(data: UpdateTask) -> Result<(), String> {
    with_db(|conn| {
        let now = Utc::now().to_rfc3339();
        conn.execute(
            "UPDATE tasks SET member_id = ?1, category_id = ?2, title = ?3, description = ?4, start_date = ?5, end_date = ?6, status = ?7, updated_at = ?8 WHERE id = ?9",
            params![data.member_id, data.category_id, data.title, data.description, data.start_date, data.end_date, data.status, now, data.id],
        )?;
        Ok(())
    })
}

pub fn delete_task(id: &str) -> Result<(), String> {
    with_db(|conn| {
        conn.execute("DELETE FROM tasks WHERE id = ?1", params![id])?;
        Ok(())
    })
}

// Initialize default categories for a team
pub fn init_default_categories(team_id: &str) -> Result<Vec<Category>, String> {
    let defaults = vec![
        ("Proposal", "#FF6B6B"),
        ("Draft", "#4ECDC4"),
        ("Wireframe", "#45B7D1"),
        ("UI Instruction", "#96CEB4"),
        ("VFX Instruction", "#FFEAA7"),
        ("Sound Instruction", "#DDA0DD"),
        ("DM/VO", "#98D8C8"),
        ("Server", "#F7DC6F"),
        ("Client", "#BB8FCE"),
        ("QA", "#85C1E9"),
    ];

    let mut categories = Vec::new();
    for (name, color) in defaults {
        let cat = create_category(CreateCategory {
            team_id: team_id.to_string(),
            name: name.to_string(),
            color: color.to_string(),
        })?;
        categories.push(cat);
    }
    Ok(categories)
}

// Export data as JSON
pub fn export_data(team_id: &str) -> Result<String, String> {
    let team = with_db(|conn| {
        conn.query_row(
            "SELECT id, name, created_at FROM teams WHERE id = ?1",
            params![team_id],
            |row| {
                Ok(Team {
                    id: row.get(0)?,
                    name: row.get(1)?,
                    created_at: row.get(2)?,
                })
            },
        )
    })?;
    let members = get_members(team_id)?;
    let categories = get_categories(team_id)?;
    let tasks = get_tasks(team_id)?;

    let export = serde_json::json!({
        "team": team,
        "members": members,
        "categories": categories,
        "tasks": tasks,
        "exported_at": Utc::now().to_rfc3339()
    });

    serde_json::to_string_pretty(&export).map_err(|e| e.to_string())
}

pub fn import_data(team_id: &str, json_data: &str) -> Result<(), String> {
    let data: serde_json::Value = serde_json::from_str(json_data).map_err(|e| e.to_string())?;

    // Delete existing data for this team before importing
    info!("Deleting existing data for team: {}", team_id);
    with_db(|conn| {
        conn.execute("DELETE FROM tasks WHERE team_id = ?1", params![team_id])?;
        conn.execute("DELETE FROM categories WHERE team_id = ?1", params![team_id])?;
        conn.execute("DELETE FROM members WHERE team_id = ?1", params![team_id])?;
        Ok(())
    })?;
    info!("Existing data deleted, importing new data...");

    // Import members
    if let Some(members) = data.get("members").and_then(|v| v.as_array()) {
        for member in members {
            let name = member.get("name").and_then(|v| v.as_str()).unwrap_or("");
            let role = member.get("role").and_then(|v| v.as_str()).unwrap_or("");
            let color = member.get("color").and_then(|v| v.as_str()).unwrap_or("#4A90D9");
            create_member(CreateMember {
                team_id: team_id.to_string(),
                name: name.to_string(),
                role: role.to_string(),
                color: color.to_string(),
            })?;
        }
    }
    
    // Import categories
    if let Some(categories) = data.get("categories").and_then(|v| v.as_array()) {
        for cat in categories {
            let name = cat.get("name").and_then(|v| v.as_str()).unwrap_or("");
            let color = cat.get("color").and_then(|v| v.as_str()).unwrap_or("#4A90D9");
            create_category(CreateCategory {
                team_id: team_id.to_string(),
                name: name.to_string(),
                color: color.to_string(),
            })?;
        }
    }
    
    // Import tasks
    if let Some(tasks) = data.get("tasks").and_then(|v| v.as_array()) {
        for task in tasks {
            let title = task.get("title").and_then(|v| v.as_str()).unwrap_or("");
            let description = task.get("description").and_then(|v| v.as_str()).unwrap_or("");
            let start_date = task.get("start_date").and_then(|v| v.as_str()).unwrap_or("");
            let end_date = task.get("end_date").and_then(|v| v.as_str()).unwrap_or("");
            let status = task.get("status").and_then(|v| v.as_str()).unwrap_or("not_started");
            create_task(CreateTask {
                team_id: team_id.to_string(),
                member_id: None,
                category_id: None,
                title: title.to_string(),
                description: description.to_string(),
                start_date: start_date.to_string(),
                end_date: end_date.to_string(),
                status: status.to_string(),
            })?;
        }
    }
    
    Ok(())
}
