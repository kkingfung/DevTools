import { create } from 'zustand';
import { invoke } from '@tauri-apps/api/core';
import type {
  Team,
  Member,
  Category,
  Task,
  CreateTeam,
  CreateMember,
  CreateCategory,
  CreateTask,
  UpdateTask,
  UpdateMember,
  UpdateCategory,
  ViewMode,
} from '../types';

interface AppState {
  teams: Team[];
  currentTeam: Team | null;
  members: Member[];
  categories: Category[];
  tasks: Task[];
  viewMode: ViewMode;
  selectedMemberIds: string[];
  selectedCategoryIds: string[];
  selectedStatuses: string[];
  dateRange: { start: string | null; end: string | null };
  ganttDateRange: { start: string | null; end: string | null };
  loading: boolean;
  error: string | null;
  setViewMode: (mode: ViewMode) => void;
  setSelectedMemberIds: (ids: string[]) => void;
  setSelectedCategoryIds: (ids: string[]) => void;
  setSelectedStatuses: (statuses: string[]) => void;
  setDateRange: (start: string | null, end: string | null) => void;
  setGanttDateRange: (start: string | null, end: string | null) => void;
  loadTeams: () => Promise<void>;
  createTeam: (data: CreateTeam) => Promise<Team>;
  updateTeam: (id: string, name: string) => Promise<void>;
  setCurrentTeam: (team: Team | null) => void;
  deleteTeam: (id: string) => Promise<void>;
  loadMembers: (teamId: string) => Promise<void>;
  createMember: (data: CreateMember) => Promise<Member>;
  updateMember: (data: UpdateMember) => Promise<void>;
  deleteMember: (id: string) => Promise<void>;
  loadCategories: (teamId: string) => Promise<void>;
  createCategory: (data: CreateCategory) => Promise<Category>;
  updateCategory: (data: UpdateCategory) => Promise<void>;
  deleteCategory: (id: string) => Promise<void>;
  initDefaultCategories: (teamId: string) => Promise<void>;
  loadTasks: (teamId: string) => Promise<void>;
  createTask: (data: CreateTask) => Promise<Task>;
  updateTask: (data: UpdateTask) => Promise<void>;
  deleteTask: (id: string) => Promise<void>;
  exportData: (teamId: string) => Promise<string>;
  importData: (teamId: string, jsonData: string) => Promise<void>;
  getFilteredTasks: () => Task[];
  loadTeamData: (teamId: string) => Promise<void>;
}

export const useAppStore = create<AppState>((set, get) => ({
  teams: [],
  currentTeam: null,
  members: [],
  categories: [],
  tasks: [],
  viewMode: 'gantt',
  selectedMemberIds: [],
  selectedCategoryIds: [],
  selectedStatuses: [],
  dateRange: { start: null, end: null },
  ganttDateRange: { start: null, end: null },
  loading: false,
  error: null,

  setViewMode: (mode) => set({ viewMode: mode }),
  setSelectedMemberIds: (ids) => set({ selectedMemberIds: ids }),
  setSelectedCategoryIds: (ids) => set({ selectedCategoryIds: ids }),
  setSelectedStatuses: (statuses) => set({ selectedStatuses: statuses }),
  setDateRange: (start, end) => set({ dateRange: { start, end } }),
  setGanttDateRange: (start, end) => set({ ganttDateRange: { start, end } }),

  loadTeams: async () => {
    set({ loading: true, error: null });
    try {
      const teams = await invoke<Team[]>('cmd_get_teams');
      set({ teams, loading: false });
    } catch (err) {
      set({ error: String(err), loading: false });
    }
  },

  createTeam: async (data) => {
    const team = await invoke<Team>('cmd_create_team', { data });
    set((state) => ({ teams: [...state.teams, team] }));
    return team;
  },

  updateTeam: async (id, name) => {
    await invoke('cmd_update_team', { id, name });
    set((state) => ({
      teams: state.teams.map((t) => t.id === id ? { ...t, name } : t),
      currentTeam: state.currentTeam?.id === id ? { ...state.currentTeam, name } : state.currentTeam,
    }));
  },

  setCurrentTeam: (team) => set({ currentTeam: team }),

  deleteTeam: async (id) => {
    await invoke('cmd_delete_team', { id });
    set((state) => ({
      teams: state.teams.filter((t) => t.id !== id),
      currentTeam: state.currentTeam?.id === id ? null : state.currentTeam,
    }));
  },

  loadMembers: async (teamId) => {
    const members = await invoke<Member[]>('cmd_get_members', { teamId });
    set({ members });
  },

  createMember: async (data) => {
    const member = await invoke<Member>('cmd_create_member', { data });
    set((state) => ({ members: [...state.members, member] }));
    return member;
  },

  updateMember: async (data) => {
    await invoke('cmd_update_member', { data });
    set((state) => ({
      members: state.members.map((m) => m.id === data.id ? { ...m, ...data } : m),
    }));
  },

  deleteMember: async (id) => {
    await invoke('cmd_delete_member', { id });
    set((state) => ({ members: state.members.filter((m) => m.id !== id) }));
  },

  loadCategories: async (teamId) => {
    const categories = await invoke<Category[]>('cmd_get_categories', { teamId });
    set({ categories });
  },

  createCategory: async (data) => {
    const category = await invoke<Category>('cmd_create_category', { data });
    set((state) => ({ categories: [...state.categories, category] }));
    return category;
  },

  updateCategory: async (data) => {
    await invoke('cmd_update_category', { data });
    set((state) => ({
      categories: state.categories.map((c) => c.id === data.id ? { ...c, ...data } : c),
    }));
  },

  deleteCategory: async (id) => {
    await invoke('cmd_delete_category', { id });
    set((state) => ({ categories: state.categories.filter((c) => c.id !== id) }));
  },

  initDefaultCategories: async (teamId) => {
    const categories = await invoke<Category[]>('cmd_init_default_categories', { teamId });
    set({ categories });
  },

  loadTasks: async (teamId) => {
    const tasks = await invoke<Task[]>('cmd_get_tasks', { teamId });
    set({ tasks });
  },

  createTask: async (data) => {
    const task = await invoke<Task>('cmd_create_task', { data });
    set((state) => ({ tasks: [...state.tasks, task] }));
    return task;
  },

  updateTask: async (data) => {
    await invoke('cmd_update_task', { data });
    set((state) => ({
      tasks: state.tasks.map((t) => t.id === data.id ? { ...t, ...data } : t),
    }));
  },

  deleteTask: async (id) => {
    await invoke('cmd_delete_task', { id });
    set((state) => ({ tasks: state.tasks.filter((t) => t.id !== id) }));
  },

  exportData: async (teamId) => {
    return await invoke<string>('cmd_export_data', { teamId });
  },
  importData: async (teamId, jsonData) => {
    await invoke("cmd_import_data", { teamId, jsonData });
    await get().loadTeamData(teamId);
  },

  getFilteredTasks: () => {
    const { tasks, selectedMemberIds, selectedCategoryIds, selectedStatuses, dateRange } = get();
    return tasks.filter((task) => {
      if (selectedMemberIds.length > 0 && (!task.member_id || !selectedMemberIds.includes(task.member_id))) return false;
      if (selectedCategoryIds.length > 0 && (!task.category_id || !selectedCategoryIds.includes(task.category_id))) return false;
      if (selectedStatuses.length > 0 && !selectedStatuses.includes(task.status)) return false;
      if (dateRange.start && task.end_date < dateRange.start) return false;
      if (dateRange.end && task.start_date > dateRange.end) return false;
      return true;
    });
  },

  loadTeamData: async (teamId) => {
    set({ loading: true });
    try {
      await Promise.all([
        get().loadMembers(teamId),
        get().loadCategories(teamId),
        get().loadTasks(teamId),
      ]);
      set({ loading: false });
    } catch (err) {
      set({ error: String(err), loading: false });
    }
  },
}));
