import { useMemo } from "react";
import { useAppStore } from "../../store";
import { DatePicker, Button, Space } from "antd";
import dayjs from "dayjs";
import "./Gantt.css";

const { RangePicker } = DatePicker;

export default function GanttView() {
  const { getFilteredTasks, categories, members, ganttDateRange, setGanttDateRange } = useAppStore();
  const tasks = getFilteredTasks();

  const { startDate, days } = useMemo(() => {
    // 手動で日付範囲が設定されている場合はそれを使用
    if (ganttDateRange.start && ganttDateRange.end) {
      const minDate = dayjs(ganttDateRange.start);
      const maxDate = dayjs(ganttDateRange.end);
      const daysArr: dayjs.Dayjs[] = [];
      let current = minDate;
      while (current.isBefore(maxDate) || current.isSame(maxDate, "day")) {
        daysArr.push(current);
        current = current.add(1, "day");
      }
      return { startDate: minDate, endDate: maxDate, days: daysArr };
    }

    // 自動計算（タスクから算出）
    if (tasks.length === 0) {
      const today = dayjs();
      return {
        startDate: today.startOf("month"),
        endDate: today.endOf("month"),
        days: [] as dayjs.Dayjs[],
      };
    }
    let minDate = dayjs(tasks[0].start_date);
    let maxDate = dayjs(tasks[0].end_date);
    tasks.forEach((t) => {
      const s = dayjs(t.start_date);
      const e = dayjs(t.end_date);
      if (s.isBefore(minDate)) minDate = s;
      if (e.isAfter(maxDate)) maxDate = e;
    });
    minDate = minDate.subtract(7, "day");
    maxDate = maxDate.add(7, "day");
    const daysArr: dayjs.Dayjs[] = [];
    let current = minDate;
    while (current.isBefore(maxDate) || current.isSame(maxDate, "day")) {
      daysArr.push(current);
      current = current.add(1, "day");
    }
    return { startDate: minDate, endDate: maxDate, days: daysArr };
  }, [tasks, ganttDateRange]);

  const getTaskPosition = (start: string, end: string) => {
    const startIdx = dayjs(start).diff(startDate, "day");
    const endIdx = dayjs(end).diff(startDate, "day");
    return { left: startIdx * 30, width: (endIdx - startIdx + 1) * 30 };
  };

  // 日付範囲ピッカーのハンドラー
  const handleDateRangeChange = (dates: [dayjs.Dayjs | null, dayjs.Dayjs | null] | null) => {
    if (dates && dates[0] && dates[1]) {
      setGanttDateRange(dates[0].format("YYYY-MM-DD"), dates[1].format("YYYY-MM-DD"));
    }
  };

  const handleResetDateRange = () => {
    setGanttDateRange(null, null);
  };

  if (tasks.length === 0) {
    return <div style={{ padding: 40, textAlign: "center", color: "#999" }}>No tasks yet. Add tasks using the Table view.</div>;
  }

  return (
    <div className="gantt-wrapper">
      <div className="gantt-toolbar">
        <Space>
          <span>Period:</span>
          <RangePicker
            value={
              ganttDateRange.start && ganttDateRange.end
                ? [dayjs(ganttDateRange.start), dayjs(ganttDateRange.end)]
                : null
            }
            onChange={handleDateRangeChange}
            placeholder={["Start Date", "End Date"]}
          />
          <Button onClick={handleResetDateRange} disabled={!ganttDateRange.start}>
            Auto
          </Button>
        </Space>
      </div>
      <div className="gantt-container">
      <div className="gantt-header">
        <div className="gantt-label-col">Task</div>
        <div className="gantt-timeline" style={{ width: days.length * 30 }}>
          {days.map((d, i) => (
            <div key={i} className={`gantt-day ${d.day() === 0 || d.day() === 6 ? "weekend" : ""}`}>
              {d.format("MM/DD")}
            </div>
          ))}
        </div>
      </div>
      <div className="gantt-body">
        {tasks.map((task) => {
          const cat = categories.find((c) => c.id === task.category_id);
          const member = members.find((m) => m.id === task.member_id);
          const pos = getTaskPosition(task.start_date, task.end_date);
          return (
            <div key={task.id} className="gantt-row">
              <div className="gantt-label-col">
                <div className="gantt-task-title">{task.title}</div>
                <div className="gantt-task-meta">{member?.name || "Unassigned"}</div>
              </div>
              <div className="gantt-timeline" style={{ width: days.length * 30 }}>
                {days.map((d, i) => (
                  <div key={i} className={`gantt-cell ${d.day() === 0 || d.day() === 6 ? "weekend" : ""}`} />
                ))}
                <div
                  className="gantt-bar"
                  style={{ left: pos.left, width: pos.width, backgroundColor: cat?.color || "#4A90D9" }}
                  title={`${task.title} (${task.start_date} ~ ${task.end_date})`}
                />
              </div>
            </div>
          );
        })}
      </div>
      </div>
    </div>
  );
}
