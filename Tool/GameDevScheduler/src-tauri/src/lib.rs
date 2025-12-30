mod database;
mod models;

use log::{info, error};
use models::*;

// Team commands
#[tauri::command]
fn cmd_create_team(data: CreateTeam) -> Result<Team, String> {
    database::create_team(data)
}

#[tauri::command]
fn cmd_get_teams() -> Result<Vec<Team>, String> {
    database::get_teams()
}

#[tauri::command]
fn cmd_update_team(id: String, name: String) -> Result<(), String> {
    database::update_team(&id, &name)
}

#[tauri::command]
fn cmd_delete_team(id: String) -> Result<(), String> {
    database::delete_team(&id)
}

// Member commands
#[tauri::command]
fn cmd_create_member(data: CreateMember) -> Result<Member, String> {
    database::create_member(data)
}

#[tauri::command]
fn cmd_get_members(team_id: String) -> Result<Vec<Member>, String> {
    database::get_members(&team_id)
}

#[tauri::command]
fn cmd_update_member(data: UpdateMember) -> Result<(), String> {
    database::update_member(data)
}

#[tauri::command]
fn cmd_delete_member(id: String) -> Result<(), String> {
    database::delete_member(&id)
}

// Category commands
#[tauri::command]
fn cmd_create_category(data: CreateCategory) -> Result<Category, String> {
    database::create_category(data)
}

#[tauri::command]
fn cmd_get_categories(team_id: String) -> Result<Vec<Category>, String> {
    database::get_categories(&team_id)
}

#[tauri::command]
fn cmd_update_category(data: UpdateCategory) -> Result<(), String> {
    database::update_category(data)
}

#[tauri::command]
fn cmd_delete_category(id: String) -> Result<(), String> {
    database::delete_category(&id)
}

#[tauri::command]
fn cmd_init_default_categories(team_id: String) -> Result<Vec<Category>, String> {
    database::init_default_categories(&team_id)
}

// Task commands
#[tauri::command]
fn cmd_create_task(data: CreateTask) -> Result<Task, String> {
    database::create_task(data)
}

#[tauri::command]
fn cmd_get_tasks(team_id: String) -> Result<Vec<Task>, String> {
    database::get_tasks(&team_id)
}

#[tauri::command]
fn cmd_update_task(data: UpdateTask) -> Result<(), String> {
    database::update_task(data)
}

#[tauri::command]
fn cmd_delete_task(id: String) -> Result<(), String> {
    database::delete_task(&id)
}

// Export command
#[tauri::command]
fn cmd_export_data(team_id: String) -> Result<String, String> {
    database::export_data(&team_id)
}

// Import command
#[tauri::command]
fn cmd_import_data(team_id: String, json_data: String) -> Result<(), String> {
    database::import_data(&team_id, &json_data)
}

// File I/O commands
#[tauri::command]
fn cmd_write_file(path: String, content: String) -> Result<(), String> {
    info!("Writing file to: {}", path);
    std::fs::write(&path, &content).map_err(|e| {
        error!("Failed to write file: {}", e);
        format!("Failed to write file: {}", e)
    })
}

#[tauri::command]
fn cmd_read_file(path: String) -> Result<String, String> {
    info!("Reading file from: {}", path);
    std::fs::read_to_string(&path).map_err(|e| {
        error!("Failed to read file: {}", e);
        format!("Failed to read file: {}", e)
    })
}

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_log::Builder::new()
            .target(tauri_plugin_log::Target::new(
                tauri_plugin_log::TargetKind::LogDir { file_name: Some("app.log".into()) }
            ))
            .level(log::LevelFilter::Info)
            .build())
        .plugin(tauri_plugin_opener::init())
        .plugin(tauri_plugin_dialog::init())
        .setup(|_app| {
            info!("Application starting...");

            // Initialize database
            match database::init_db_public() {
                Ok(_) => info!("Database ready"),
                Err(e) => error!("Failed to initialize database: {}", e),
            }

            info!("Setup complete");
            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            cmd_create_team,
            cmd_get_teams,
            cmd_update_team,
            cmd_delete_team,
            cmd_create_member,
            cmd_get_members,
            cmd_update_member,
            cmd_delete_member,
            cmd_create_category,
            cmd_get_categories,
            cmd_update_category,
            cmd_delete_category,
            cmd_init_default_categories,
            cmd_create_task,
            cmd_get_tasks,
            cmd_update_task,
            cmd_delete_task,
            cmd_export_data,
            cmd_import_data,
            cmd_write_file,
            cmd_read_file,
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
