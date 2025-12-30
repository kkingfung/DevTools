use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Team {
    pub id: String,
    pub name: String,
    pub created_at: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Member {
    pub id: String,
    pub team_id: String,
    pub name: String,
    pub role: String, // "leader" or "member"
    pub color: String, // hex color for display
    pub created_at: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Category {
    pub id: String,
    pub team_id: String,
    pub name: String,
    pub color: String, // hex color for task bars
    pub order_index: i32,
    pub created_at: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Task {
    pub id: String,
    pub team_id: String,
    pub member_id: Option<String>,
    pub category_id: Option<String>,
    pub title: String,
    pub description: String,
    pub start_date: String,
    pub end_date: String,
    pub status: String, // "not_started", "in_progress", "completed"
    pub created_at: String,
    pub updated_at: String,
}

// DTOs for creation
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CreateTeam {
    pub name: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CreateMember {
    pub team_id: String,
    pub name: String,
    pub role: String,
    pub color: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CreateCategory {
    pub team_id: String,
    pub name: String,
    pub color: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CreateTask {
    pub team_id: String,
    pub member_id: Option<String>,
    pub category_id: Option<String>,
    pub title: String,
    pub description: String,
    pub start_date: String,
    pub end_date: String,
    pub status: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct UpdateTask {
    pub id: String,
    pub member_id: Option<String>,
    pub category_id: Option<String>,
    pub title: String,
    pub description: String,
    pub start_date: String,
    pub end_date: String,
    pub status: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct UpdateMember {
    pub id: String,
    pub name: String,
    pub role: String,
    pub color: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct UpdateCategory {
    pub id: String,
    pub name: String,
    pub color: String,
    pub order_index: i32,
}
