import FullCalendar from "@fullcalendar/react";
import dayGridPlugin from "@fullcalendar/daygrid";
import interactionPlugin from "@fullcalendar/interaction";
import { useAppStore } from "../../store";
import { Modal } from "antd";
import { useState } from "react";
import type { Task } from "../../types";

export default function CalendarView() {
  const { getFilteredTasks, categories, members } = useAppStore();
  const [selectedTask, setSelectedTask] = useState<Task | null>(null);
  const tasks = getFilteredTasks();

  const events = tasks.map((task) => {
    const category = categories.find((c) => c.id === task.category_id);
    return {
      id: task.id,
      title: task.title,
      start: task.start_date,
      end: task.end_date,
      backgroundColor: category?.color || "#4A90D9",
      borderColor: category?.color || "#4A90D9",
      extendedProps: { task },
    };
  });

  const handleEventClick = (info: any) => {
    setSelectedTask(info.event.extendedProps.task);
  };

  return (
    <div style={{ height: "calc(100vh - 150px)" }}>
      <FullCalendar
        plugins={[dayGridPlugin, interactionPlugin]}
        initialView="dayGridMonth"
        events={events}
        eventClick={handleEventClick}
        height="100%"
        headerToolbar={{ left: "prev,next today", center: "title", right: "dayGridMonth,dayGridWeek" }}
      />
      <Modal
        title={selectedTask?.title}
        open={!!selectedTask}
        onCancel={() => setSelectedTask(null)}
        footer={null}
      >
        {selectedTask && (
          <div>
            <p><strong>Member:</strong> {members.find((m) => m.id === selectedTask.member_id)?.name || "Unassigned"}</p>
            <p><strong>Category:</strong> {categories.find((c) => c.id === selectedTask.category_id)?.name || "None"}</p>
            <p><strong>Duration:</strong> {selectedTask.start_date} ~ {selectedTask.end_date}</p>
            <p><strong>Status:</strong> {selectedTask.status.replace("_", " ")}</p>
            {selectedTask.description && <p><strong>Description:</strong> {selectedTask.description}</p>}
          </div>
        )}
      </Modal>
    </div>
  );
}
