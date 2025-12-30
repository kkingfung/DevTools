export interface Team {
  id: string;
  name: string;
  created_at: string;
}

export interface Member {
  id: string;
  team_id: string;
  name: string;
  role: string;
  color: string;
  created_at: string;
}

export interface Category {
  id: string;
  team_id: string;
  name: string;
  color: string;
  order_index: number;
  created_at: string;
}

export type TaskStatus = 'not_started' | 'in_progress' | 'completed';

export interface Task {
  id: string;
  team_id: string;
  member_id: string | null;
  category_id: string | null;
  title: string;
  description: string;
  start_date: string;
  end_date: string;
  status: TaskStatus;
  created_at: string;
  updated_at: string;
}

export interface CreateTeam {
  name: string;
}

export interface CreateMember {
  team_id: string;
  name: string;
  role: string;
  color: string;
}

export interface CreateCategory {
  team_id: string;
  name: string;
  color: string;
}

export interface CreateTask {
  team_id: string;
  member_id: string | null;
  category_id: string | null;
  title: string;
  description: string;
  start_date: string;
  end_date: string;
  status: TaskStatus;
}

export interface UpdateTask {
  id: string;
  member_id: string | null;
  category_id: string | null;
  title: string;
  description: string;
  start_date: string;
  end_date: string;
  status: TaskStatus;
}

export interface UpdateMember {
  id: string;
  name: string;
  role: string;
  color: string;
}

export interface UpdateCategory {
  id: string;
  name: string;
  color: string;
  order_index: number;
}

export type ViewMode = 'gantt' | 'calendar' | 'table';
