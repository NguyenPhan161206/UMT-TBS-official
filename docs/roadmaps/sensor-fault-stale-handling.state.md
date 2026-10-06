# sensor-fault-stale-handling — Orchestration State
Roadmap: docs/roadmaps/sensor-fault-stale-handling.roadmap.json
State: COMPLETED (2026-09-16).

| Step | Title | Status | Verified by | Notes |
|:---:|---|:---:|---|---|
| 1 | Shared Contract: Thêm sensor_health_t và freshness header | DONE | arch_guard + scan_secrets | R2/R3 thresholds.h & espnow_protocol.h |
| 2 | Sensor-node: Fast Disconnect Detection & Buzzer Immediate Silence | DONE | pio run 2 envs (yolo_uno & yolo_uno_coreiot) | Cắt còi ngay <300ms khi rút nguồn sensor |
| 3 | Hazard Core: Bỏ qua dữ liệu sensor lỗi/stale trong phân loại | DONE | host_sim tests (test_hazard_core.c) | C C99 thuần, test trên dev PC |
| 4 | Waveshare Receiver & Watchdog: Xử lý Stale Per-Sensor & Link Loss | DONE | pio run waveshare | Xóa dữ liệu cũ, không freeze cảnh báo |
| 5 | UI Dashboard: Hiển thị trực quan trạng thái mất kết nối / hỏng cảm biến | DONE | pio run waveshare (yolo_uno) | Phân biệt rõ 'SAFE' vs 'SENSOR FAULT' |

## Contracts to Establish
- `sensor_health_t`: `SENSOR_HEALTH_OK`, `SENSOR_HEALTH_OUT_OF_RANGE`, `SENSOR_HEALTH_DISCONNECTED`, `SENSOR_HEALTH_STALE`.
- `SENSOR_FAULT_CONSECUTIVE_MISS`: 3 chu kỳ đo liên tiếp timeout (~300ms) -> chuyển sang `DISCONNECTED`.
- `SENSOR_STALE_TIMEOUT_MS`: 1000ms quá hạn riêng từng cảm biến trên màn hình.
- `espnow_sensor_msg_t`: có trường `seq` (freshness) và `health[6]`.
