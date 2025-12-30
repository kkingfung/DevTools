import { useEffect, useState } from "react";
import {
  Layout,
  Select,
  Button,
  Radio,
  Modal,
  Form,
  Input,
  message,
  Spin,
  Dropdown,
  ColorPicker,
  Table,
  Space,
  Popconfirm,
  Tag,
} from "antd";
import {
  PlusOutlined,
  TeamOutlined,
  UserOutlined,
  TagsOutlined,
  ExportOutlined,
  SettingOutlined,
  EditOutlined,
  DeleteOutlined,
  ImportOutlined,
} from "@ant-design/icons";
import type { MenuProps } from "antd";
import { useAppStore } from "./store";
import GanttView from "./components/GanttView";
import CalendarView from "./components/CalendarView";
import TableView from "./components/TableView";
import FilterSidebar from "./components/FilterSidebar";
import type { ViewMode, Member, Category } from "./types";
import { save, open } from "@tauri-apps/plugin-dialog";
import { invoke } from "@tauri-apps/api/core";
import "./App.css";

const { Header, Content, Sider } = Layout;

function App() {
  const {
    teams,
    currentTeam,
    viewMode,
    loading,
    members,
    categories,
    loadTeams,
    createTeam,
    setCurrentTeam,
    loadTeamData,
    setViewMode,
    createMember,
    updateMember,
    deleteMember,
    createCategory,
    updateCategory,
    deleteCategory,
    initDefaultCategories,
    exportData,
    importData,
    updateTeam,
    deleteTeam,
  } = useAppStore();

  const [teamModalOpen, setTeamModalOpen] = useState(false);
  const [memberModalOpen, setMemberModalOpen] = useState(false);
  const [categoryModalOpen, setCategoryModalOpen] = useState(false);
  const [manageMembersOpen, setManageMembersOpen] = useState(false);
  const [manageCategoriesOpen, setManageCategoriesOpen] = useState(false);
  const [editingMember, setEditingMember] = useState<Member | null>(null);
  const [editingCategory, setEditingCategory] = useState<Category | null>(null);
  const [editTeamModalOpen, setEditTeamModalOpen] = useState(false);
  const [editTeamForm] = Form.useForm();
  const [teamForm] = Form.useForm();
  const [memberForm] = Form.useForm();
  const [categoryForm] = Form.useForm();

  useEffect(() => {
    loadTeams();
  }, [loadTeams]);

  useEffect(() => {
    if (currentTeam) {
      loadTeamData(currentTeam.id);
    }
  }, [currentTeam, loadTeamData]);

  const handleCreateTeam = async () => {
    try {
      const values = await teamForm.validateFields();
      const team = await createTeam({ name: values.name });
      setCurrentTeam(team);
      await initDefaultCategories(team.id);
      setTeamModalOpen(false);
      teamForm.resetFields();
      message.success("Team created successfully");
    } catch (err) {
      message.error("Failed to create team");
    }
  };

  const handleMemberSubmit = async () => {
    if (!currentTeam) return;
    try {
      const values = await memberForm.validateFields();
      const color = typeof values.color === "string" ? values.color : values.color?.toHexString() || "#4A90D9";
      if (editingMember) {
        await updateMember({ id: editingMember.id, name: values.name, role: values.role || "", color });
        message.success("Member updated");
      } else {
        await createMember({ team_id: currentTeam.id, name: values.name, role: values.role || "", color });
        message.success("Member added");
      }
      setMemberModalOpen(false);
      setEditingMember(null);
      memberForm.resetFields();
    } catch (err) {
      message.error("Failed to save member");
    }
  };

  const handleCategorySubmit = async () => {
    if (!currentTeam) return;
    try {
      const values = await categoryForm.validateFields();
      const color = typeof values.color === "string" ? values.color : values.color?.toHexString() || "#4A90D9";
      if (editingCategory) {
        await updateCategory({ id: editingCategory.id, name: values.name, color, order_index: editingCategory.order_index });
        message.success("Category updated");
      } else {
        await createCategory({ team_id: currentTeam.id, name: values.name, color });
        message.success("Category added");
      }
      setCategoryModalOpen(false);
      setEditingCategory(null);
      categoryForm.resetFields();
    } catch (err) {
      message.error("Failed to save category");
    }
  };

  const handleDeleteMember = async (id: string) => {
    try {
      await deleteMember(id);
      message.success("Member deleted");
    } catch (err) {
      message.error("Failed to delete member");
    }
  };

  const handleDeleteCategory = async (id: string) => {
    try {
      await deleteCategory(id);
      message.success("Category deleted");
    } catch (err) {
      message.error("Failed to delete category");
    }
  };

  const handleExport = async () => {
    if (!currentTeam) return;
    try {
      const filePath = await save({
        defaultPath: currentTeam.name + "_export.json",
        filters: [{ name: "JSON", extensions: ["json"] }],
      });
      if (!filePath) return;
      const json = await exportData(currentTeam.id);
      await invoke("cmd_write_file", { path: filePath, content: json });
      message.success("Data exported");
    } catch (err) {
      console.error("Export error:", err);
      message.error("Failed to export data: " + String(err));
    }
  };
  const handleImport = async () => {
    if (!currentTeam) return;
    try {
      const filePath = await open({
        filters: [{ name: "JSON", extensions: ["json"] }],
        multiple: false,
      });
      if (!filePath) return;
      const text = await invoke<string>("cmd_read_file", { path: filePath as string });
      await importData(currentTeam.id, text);
      message.success("Data imported successfully");
    } catch (err) {
      console.error("Import error:", err);
      message.error("Failed to import data: " + String(err));
    }
  };

  const openRenameTeam = () => {
    if (currentTeam) {
      editTeamForm.setFieldsValue({ name: currentTeam.name });
      setEditTeamModalOpen(true);
    }
  };

  const handleRenameTeam = async () => {
    if (!currentTeam) return;
    try {
      const values = await editTeamForm.validateFields();
      await updateTeam(currentTeam.id, values.name);
      setEditTeamModalOpen(false);
      message.success("Team renamed successfully");
    } catch (err) {
      message.error("Failed to rename team");
    }
  };

  const handleDeleteTeam = async () => {
    if (!currentTeam) return;
    try {
      await deleteTeam(currentTeam.id);
      message.success("Team deleted successfully");
    } catch (err) {
      message.error("Failed to delete team");
    }
  };

  const openEditMember = (member: Member) => {
    setEditingMember(member);
    memberForm.setFieldsValue({ name: member.name, role: member.role, color: member.color });
    setMemberModalOpen(true);
  };

  const openEditCategory = (category: Category) => {
    setEditingCategory(category);
    categoryForm.setFieldsValue({ name: category.name, color: category.color });
    setCategoryModalOpen(true);
  };

  const openAddMember = () => {
    setEditingMember(null);
    memberForm.resetFields();
    setMemberModalOpen(true);
  };

  const openAddCategory = () => {
    setEditingCategory(null);
    categoryForm.resetFields();
    setCategoryModalOpen(true);
  };

  const settingsMenuItems: MenuProps["items"] = [
    { key: "rename-team", icon: <EditOutlined />, label: "Rename Team", onClick: openRenameTeam },
    { key: "delete-team", icon: <DeleteOutlined />, label: "Delete Team", danger: true, onClick: () => {
      Modal.confirm({
        title: "Delete Team",
        content: `Are you sure you want to delete "${currentTeam?.name}"? This will delete all tasks, members, and categories.`,
        okText: "Delete",
        okType: "danger",
        onOk: handleDeleteTeam,
      });
    }},
    { type: "divider" },
    { key: "add-member", icon: <PlusOutlined />, label: "Add Member", onClick: openAddMember },
    { key: "manage-members", icon: <UserOutlined />, label: "Manage Members", onClick: () => setManageMembersOpen(true) },
    { type: "divider" },
    { key: "add-category", icon: <PlusOutlined />, label: "Add Category", onClick: openAddCategory },
    { key: "manage-categories", icon: <TagsOutlined />, label: "Manage Categories", onClick: () => setManageCategoriesOpen(true) },
    { type: "divider" },
    { key: "export", icon: <ExportOutlined />, label: "Export Data", onClick: handleExport },
    { key: "import", icon: <ImportOutlined />, label: "Import Data", onClick: handleImport },
  ];

  const memberColumns = [
    { title: "Name", dataIndex: "name", key: "name" },
    { title: "Role", dataIndex: "role", key: "role", render: (role: string) => role || "-" },
    {
      title: "Color",
      dataIndex: "color",
      key: "color",
      render: (color: string) => <div style={{ width: 24, height: 24, backgroundColor: color, borderRadius: 4 }} />,
    },
    {
      title: "Actions",
      key: "actions",
      render: (_: unknown, record: Member) => (
        <Space>
          <Button icon={<EditOutlined />} size="small" onClick={() => openEditMember(record)} />
          <Popconfirm title="Delete this member?" onConfirm={() => handleDeleteMember(record.id)}>
            <Button icon={<DeleteOutlined />} size="small" danger />
          </Popconfirm>
        </Space>
      ),
    },
  ];

  const categoryColumns = [
    { title: "Name", dataIndex: "name", key: "name" },
    {
      title: "Color",
      dataIndex: "color",
      key: "color",
      render: (color: string, record: Category) => <Tag color={color}>{record.name}</Tag>,
    },
    {
      title: "Actions",
      key: "actions",
      render: (_: unknown, record: Category) => (
        <Space>
          <Button icon={<EditOutlined />} size="small" onClick={() => openEditCategory(record)} />
          <Popconfirm title="Delete this category?" onConfirm={() => handleDeleteCategory(record.id)}>
            <Button icon={<DeleteOutlined />} size="small" danger />
          </Popconfirm>
        </Space>
      ),
    },
  ];

  return (
    <Layout style={{ minHeight: "100vh" }}>
      <Header style={{ display: "flex", alignItems: "center", gap: 16, padding: "0 24px" }}>
        <TeamOutlined style={{ fontSize: 24, color: "#fff" }} />
        <h1 style={{ color: "#fff", margin: 0, fontSize: 18 }}>GameDev Scheduler</h1>
        <Select
          style={{ width: 200, marginLeft: 24 }}
          placeholder="Select Team"
          value={currentTeam?.id}
          onChange={(id) => setCurrentTeam(teams.find((t) => t.id === id) || null)}
          options={teams.map((t) => ({ label: t.name, value: t.id }))}
          dropdownRender={(menu) => (
            <>
              {menu}
              <div style={{ padding: 8, borderTop: "1px solid #f0f0f0" }}>
                <Button type="link" icon={<PlusOutlined />} onClick={() => setTeamModalOpen(true)}>
                  New Team
                </Button>
              </div>
            </>
          )}
        />
        <div style={{ flex: 1 }} />
        <Radio.Group
          value={viewMode}
          onChange={(e) => setViewMode(e.target.value as ViewMode)}
          optionType="button"
          buttonStyle="solid"
        >
          <Radio.Button value="gantt">Gantt</Radio.Button>
          <Radio.Button value="calendar">Calendar</Radio.Button>
          <Radio.Button value="table">Table</Radio.Button>
        </Radio.Group>
        <Dropdown menu={{ items: settingsMenuItems }} placement="bottomRight" disabled={!currentTeam}>
          <Button icon={<SettingOutlined />}>Settings</Button>
        </Dropdown>
      </Header>
      <Layout>
        <Sider width={250} theme="light" style={{ borderRight: "1px solid #f0f0f0" }}>
          <FilterSidebar />
        </Sider>
        <Content style={{ padding: 16, overflow: "auto" }}>
          {loading ? (
            <div style={{ display: "flex", justifyContent: "center", alignItems: "center", height: "100%" }}>
              <Spin size="large" />
            </div>
          ) : currentTeam ? (
            viewMode === "gantt" ? <GanttView /> : viewMode === "calendar" ? <CalendarView /> : <TableView />
          ) : (
            <div style={{ display: "flex", flexDirection: "column", justifyContent: "center", alignItems: "center", height: "100%", gap: 16 }}>
              <TeamOutlined style={{ fontSize: 64, color: "#ccc" }} />
              <h2>Select or Create a Team</h2>
              <Button type="primary" icon={<PlusOutlined />} onClick={() => setTeamModalOpen(true)}>
                Create New Team
              </Button>
            </div>
          )}
        </Content>
      </Layout>

      <Modal title="Create New Team" open={teamModalOpen} onOk={handleCreateTeam} onCancel={() => setTeamModalOpen(false)}>
        <Form form={teamForm} layout="vertical">
          <Form.Item name="name" label="Team Name" rules={[{ required: true, message: "Please enter team name" }]}>
            <Input placeholder="Enter team name" />
          </Form.Item>
        </Form>
      </Modal>

      <Modal
        title={editingMember ? "Edit Member" : "Add Member"}
        open={memberModalOpen}
        onOk={handleMemberSubmit}
        onCancel={() => { setMemberModalOpen(false); setEditingMember(null); memberForm.resetFields(); }}
      >
        <Form form={memberForm} layout="vertical" initialValues={{ color: "#4A90D9" }}>
          <Form.Item name="name" label="Name" rules={[{ required: true, message: "Please enter member name" }]}>
            <Input placeholder="Member name" />
          </Form.Item>
          <Form.Item name="role" label="Role">
            <Input placeholder="Role (optional)" />
          </Form.Item>
          <Form.Item name="color" label="Color">
            <ColorPicker />
          </Form.Item>
        </Form>
      </Modal>

      <Modal
        title={editingCategory ? "Edit Category" : "Add Category"}
        open={categoryModalOpen}
        onOk={handleCategorySubmit}
        onCancel={() => { setCategoryModalOpen(false); setEditingCategory(null); categoryForm.resetFields(); }}
      >
        <Form form={categoryForm} layout="vertical" initialValues={{ color: "#4A90D9" }}>
          <Form.Item name="name" label="Name" rules={[{ required: true, message: "Please enter category name" }]}>
            <Input placeholder="Category name" />
          </Form.Item>
          <Form.Item name="color" label="Color">
            <ColorPicker />
          </Form.Item>
        </Form>
      </Modal>

      <Modal
        title="Manage Members"
        open={manageMembersOpen}
        onCancel={() => setManageMembersOpen(false)}
        footer={[
          <Button key="add" type="primary" icon={<PlusOutlined />} onClick={() => { setManageMembersOpen(false); openAddMember(); }}>
            Add Member
          </Button>,
          <Button key="close" onClick={() => setManageMembersOpen(false)}>
            Close
          </Button>,
        ]}
        width={600}
      >
        <Table columns={memberColumns} dataSource={members} rowKey="id" pagination={false} size="small" />
      </Modal>

      <Modal
        title="Manage Categories"
        open={manageCategoriesOpen}
        onCancel={() => setManageCategoriesOpen(false)}
        footer={[
          <Button key="add" type="primary" icon={<PlusOutlined />} onClick={() => { setManageCategoriesOpen(false); openAddCategory(); }}>
            Add Category
          </Button>,
          <Button key="close" onClick={() => setManageCategoriesOpen(false)}>
            Close
          </Button>,
        ]}
        width={600}
      >
        <Table columns={categoryColumns} dataSource={categories} rowKey="id" pagination={false} size="small" />
      </Modal>

      <Modal
        title="Rename Team"
        open={editTeamModalOpen}
        onOk={handleRenameTeam}
        onCancel={() => { setEditTeamModalOpen(false); editTeamForm.resetFields(); }}
      >
        <Form form={editTeamForm} layout="vertical">
          <Form.Item name="name" label="Team Name" rules={[{ required: true, message: "Please enter team name" }]}>
            <Input placeholder="Enter team name" />
          </Form.Item>
        </Form>
      </Modal>
    </Layout>
  );
}

export default App;
