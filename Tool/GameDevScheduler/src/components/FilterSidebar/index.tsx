import { Select, DatePicker, Divider, Button } from "antd";
import { ClearOutlined } from "@ant-design/icons";
import { useAppStore } from "../../store";
import dayjs from "dayjs";

const { RangePicker } = DatePicker;

const statusOptions = [
  { label: "Not Started", value: "not_started" },
  { label: "In Progress", value: "in_progress" },
  { label: "Completed", value: "completed" },
];

export default function FilterSidebar() {
  const {
    members,
    categories,
    selectedMemberIds,
    selectedCategoryIds,
    selectedStatuses,
    dateRange,
    setSelectedMemberIds,
    setSelectedCategoryIds,
    setSelectedStatuses,
    setDateRange,
  } = useAppStore();

  const hasActiveFilters = selectedMemberIds.length > 0 || selectedCategoryIds.length > 0 || selectedStatuses.length > 0 || dateRange.start || dateRange.end;

  const clearAllFilters = () => {
    setSelectedMemberIds([]);
    setSelectedCategoryIds([]);
    setSelectedStatuses([]);
    setDateRange(null, null);
  };

  return (
    <div style={{ padding: 16 }}>
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: 16 }}>
        <h3 style={{ margin: 0 }}>Filters</h3>
        {hasActiveFilters && (
          <Button type="link" icon={<ClearOutlined />} onClick={clearAllFilters} size="small">
            Clear All
          </Button>
        )}
      </div>
      <div style={{ marginBottom: 16 }}>
        <label style={{ display: "block", marginBottom: 8 }}>Member</label>
        <Select
          mode="multiple"
          style={{ width: "100%" }}
          placeholder="All Members"
          allowClear
          value={selectedMemberIds}
          onChange={(ids) => setSelectedMemberIds(ids)}
          options={members.map((m) => ({ label: m.name, value: m.id }))}
          maxTagCount="responsive"
        />
      </div>
      <div style={{ marginBottom: 16 }}>
        <label style={{ display: "block", marginBottom: 8 }}>Category</label>
        <Select
          mode="multiple"
          style={{ width: "100%" }}
          placeholder="All Categories"
          allowClear
          value={selectedCategoryIds}
          onChange={(ids) => setSelectedCategoryIds(ids)}
          options={categories.map((c) => ({ label: c.name, value: c.id }))}
          maxTagCount="responsive"
        />
      </div>
      <div style={{ marginBottom: 16 }}>
        <label style={{ display: "block", marginBottom: 8 }}>Status</label>
        <Select
          mode="multiple"
          style={{ width: "100%" }}
          placeholder="All Statuses"
          allowClear
          value={selectedStatuses}
          onChange={(statuses) => setSelectedStatuses(statuses)}
          options={statusOptions}
          maxTagCount="responsive"
        />
      </div>
      <Divider />
      <div>
        <label style={{ display: "block", marginBottom: 8 }}>Date Range</label>
        <RangePicker
          style={{ width: "100%" }}
          value={
            dateRange.start && dateRange.end
              ? [dayjs(dateRange.start), dayjs(dateRange.end)]
              : null
          }
          onChange={(dates) => {
            if (dates && dates[0] && dates[1]) {
              setDateRange(dates[0].format("YYYY-MM-DD"), dates[1].format("YYYY-MM-DD"));
            } else {
              setDateRange(null, null);
            }
          }}
        />
      </div>
    </div>
  );
}
