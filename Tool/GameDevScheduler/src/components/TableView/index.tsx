import { Table, Button, Tag, Space, Popconfirm, Modal, Form, Input, Select, DatePicker, message } from "antd";
import { PlusOutlined, EditOutlined, DeleteOutlined } from "@ant-design/icons";
import { useAppStore } from "../../store";
import type { Task, CreateTask, UpdateTask } from "../../types";
import { useState } from "react";
import dayjs from "dayjs";

const statusColors = {
  not_started: "gray",
  in_progress: "blue",
  completed: "green",
};

export default function TableView() {
  const { currentTeam, members, categories, getFilteredTasks, createTask, updateTask, deleteTask } = useAppStore();
  const [modalOpen, setModalOpen] = useState(false);
  const [editingTask, setEditingTask] = useState<Task | null>(null);
  const [form] = Form.useForm();
  const tasks = getFilteredTasks();

  const handleSubmit = async () => {
    if (!currentTeam) return;
    try {
      const values = await form.validateFields();
      if (editingTask) {
        const data: UpdateTask = {
          id: editingTask.id,
          title: values.title,
          description: values.description || "",
          member_id: values.member_id || null,
          category_id: values.category_id || null,
          start_date: values.dates[0].format("YYYY-MM-DD"),
          end_date: values.dates[1].format("YYYY-MM-DD"),
          status: values.status,
        };
        await updateTask(data);
        message.success("Task updated");
      } else {
        const data: CreateTask = {
          team_id: currentTeam.id,
          title: values.title,
          description: values.description || "",
          member_id: values.member_id || null,
          category_id: values.category_id || null,
          start_date: values.dates[0].format("YYYY-MM-DD"),
          end_date: values.dates[1].format("YYYY-MM-DD"),
          status: values.status || "not_started",
        };
        await createTask(data);
        message.success("Task created");
      }
      setModalOpen(false);
      setEditingTask(null);
      form.resetFields();
    } catch (err) {
      message.error("Failed to save task");
    }
  };

  const columns = [
    { title: "Title", dataIndex: "title", key: "title" },
    {
      title: "Member",
      dataIndex: "member_id",
      key: "member",
      render: (id: string) => members.find((m) => m.id === id)?.name || "-",
    },
    {
      title: "Category",
      dataIndex: "category_id",
      key: "category",
      render: (id: string) => {
        const cat = categories.find((c) => c.id === id);
        return cat ? <Tag color={cat.color}>{cat.name}</Tag> : "-";
      },
    },
    { title: "Start", dataIndex: "start_date", key: "start" },
    { title: "End", dataIndex: "end_date", key: "end" },
    {
      title: "Status",
      dataIndex: "status",
      key: "status",
      render: (s: string) => <Tag color={statusColors[s as keyof typeof statusColors]}>{s.replace("_", " ")}</Tag>,
    },
    {
      title: "Actions",
      key: "actions",
      render: (_: unknown, record: Task) => (
        <Space>
          <Button icon={<EditOutlined />} size="small" onClick={() => { setEditingTask(record); form.setFieldsValue({ ...record, dates: [dayjs(record.start_date), dayjs(record.end_date)] }); setModalOpen(true); }} />
          <Popconfirm title="Delete this task?" onConfirm={() => deleteTask(record.id)}>
            <Button icon={<DeleteOutlined />} size="small" danger />
          </Popconfirm>
        </Space>
      ),
    },
  ];

  return (
    <div>
      <div style={{ marginBottom: 16 }}>
        <Button type="primary" icon={<PlusOutlined />} onClick={() => { setEditingTask(null); form.resetFields(); setModalOpen(true); }}>
          Add Task
        </Button>
      </div>
      <Table columns={columns} dataSource={tasks} rowKey="id" />
      <Modal
        title={editingTask ? "Edit Task" : "New Task"}
        open={modalOpen}
        onOk={handleSubmit}
        onCancel={() => { setModalOpen(false); setEditingTask(null); }}
      >
        <Form form={form} layout="vertical" initialValues={{ status: "not_started" }}>
          <Form.Item name="title" label="Title" rules={[{ required: true }]}>
            <Input />
          </Form.Item>
          <Form.Item name="description" label="Description">
            <Input.TextArea rows={3} />
          </Form.Item>
          <Form.Item name="member_id" label="Member">
            <Select allowClear options={members.map((m) => ({ label: m.name, value: m.id }))} />
          </Form.Item>
          <Form.Item name="category_id" label="Category">
            <Select allowClear options={categories.map((c) => ({ label: c.name, value: c.id }))} />
          </Form.Item>
          <Form.Item name="dates" label="Date Range" rules={[{ required: true }]}>
            <DatePicker.RangePicker style={{ width: "100%" }} />
          </Form.Item>
          <Form.Item name="status" label="Status">
            <Select options={[{ label: "Not Started", value: "not_started" }, { label: "In Progress", value: "in_progress" }, { label: "Completed", value: "completed" }]} />
          </Form.Item>
        </Form>
      </Modal>
    </div>
  );
}
