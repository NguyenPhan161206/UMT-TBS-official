# paper-measurements — Orchestration State
Roadmap: docs/roadmaps/paper-measurements.roadmap.json
Updated: 2026-10-06

| Step | Title | Status | Verified by | Notes |
|------|-------|--------|-------------|-------|
| 1 | Shared soak contract | DONE | build 4 env sensor-node | `firmware/shared/soak_diag.h` |
| 2 | Sensor-node tx ok/fail | DONE | `pio run -e yolo_uno` | |
| 3 | Sensor-node MQTT reconnect | DONE | `pio run -e yolo_uno_coreiot` | |
| 4 | Sensor-node BOOT + SOAK | DONE (build) | 5 env SUCCESS, chuỗi SOAK có trong mọi firmware.elf | chưa thấy trên board |
| 5 | Màn hình rx/maxgap/mqtt_rc | DONE (build) | `pio run -e yolo_uno` SUCCESS | |
| 6 | Màn hình BOOT + SOAK | DONE (build) | yolo_uno + yolo_uno_latency SUCCESS, có `soak_heartbeat.c.o`, elf chứa `SOAK scr` | chưa thấy trên board |
| 7 | soak_logger.py + pytest | DONE | `pytest tools/soak` 11 passed | gồm test dò cổng |
| 8 | docs/SOAK_TEST.md | DONE | — | |
| 9 | env yolo_uno_accuracy + ACC log | DONE | `pio run -e yolo_uno_accuracy` SUCCESS | |
| 10 | measure_accuracy.py + docs | DONE | `pytest tools/accuracy` 8 passed | TEST_PROTOCOL.md 2.1/2.2 sửa |
| 11 | Đóng phần độ trễ | DONE (build) | màn hình `yolo_uno_latency` SUCCESS 06/10 | chờ import rule-chain + đo |
| 12 | Kiểm thử chức năng | DONE | ctest 13/13, native 19/19, pytest | `docs/logs/FUNCTIONAL_TEST_LOG.md` |
| 13 | PROGRESS + nháp Jira | DONE | — | nháp ở `docs/logs/JIRA_COMMENT_DRAFTS.md`, chưa gửi |

## Contracts established
- `#define TBS_SOAK_HEARTBEAT_INTERVAL_MS 60000`, `static inline const char *tbs_reset_reason_name(int rr)` — firmware/shared/soak_diag.h
- `uint32_t EspNowClient::txOk() const; uint32_t EspNowClient::txFail() const;` — sensor-node/include/espnow_client.h
- `uint32_t CoreiotClient::reconnectCount() const;` — sensor-node/src/plugins/coreiot/coreiot_client.h
- `void soakHeartbeatBoot(); void soakHeartbeatSetTasks(TaskHandle_t, TaskHandle_t, TaskHandle_t); void soakHeartbeatPoll(uint32_t nowMs, const SoakNodeCounters &c);` — sensor-node/include/soak_heartbeat.h
- `uint32_t espnow_receiver_rx_count(void); uint32_t espnow_receiver_take_max_gap_ms(void);` — espnow_receiver.h
- `uint32_t coreiot_client_reconnect_count(void);` — coreiot_client.h
- `void soak_heartbeat_start(void); void soak_heartbeat_note_link_down(void);` — waveshare-screen/src/soak_heartbeat.h
- `resolve_port(port, role)` — tools/serial_ports.py (auto = dò VID:PID 303A:1001 sensor / 1A86:55D3 màn hình)

## Deviations from plan
- Người dùng yêu cầu làm ngay trong phiên (soak phải flash hôm nay) → orchestrator tự thực thi và tự verify DoD
  thay vì phát Worker prompt.
- Module heartbeat đặt tên `soak_heartbeat.*` (không phải `soak_diag.*`) để không trùng tên header shared trên include path.
- Dòng ACC reject: `rej=` để cuối dòng + thêm `calc=`; env accuracy đọc cả cảm biến DISCONNECTED mỗi chu kỳ.
- Thêm tự dò cổng (máy dev: COM4 là Bluetooth, sensor-node là COM7).
- `tbs_reset_is_unexpected()` không làm trong firmware; phân loại reset nằm ở `soak_logger.py` (`UNEXPECTED_RESETS`).
