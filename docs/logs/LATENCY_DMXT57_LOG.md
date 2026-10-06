# LATENCY — Đo độ trễ ESP-NOW & MQTT (DMXT-57)

- **Ngày**: 2026-10-06
- **Trạng thái**: công cụ đo xong, **chưa có số đo trên board**.

## Mục tiêu
Có số đo độ trễ V2 cho bài hội nghị (go/no-go 09/10): ESP-NOW một chiều, MQTT đầu–cuối qua CoreIoT,
tỷ lệ nhận gói, tuổi mẫu lúc gửi. Không có logic analyzer → đo thuần phần mềm (RTT/2 + mốc PC).

## File đã sửa
| File | Thay đổi |
|---|---|
| `firmware/shared/espnow_protocol.h` | Thêm `espnow_echo_msg_t` (3 byte, `ESPNOW_ECHO_LATENCY`) + static_assert kích thước khác 2 gói cũ |
| `firmware/sensor-node/src/espnow_client.cpp`, `include/espnow_client.h` | (`TBS_LATENCY_PROBE`) mốc `esp_timer` lúc gửi theo seq, nhận echo → RTT vào queue; `pollRtt()` |
| `firmware/sensor-node/src/shared_state.cpp`, `include/shared_state.h` | Lưu `millis()` lúc cập nhật mỗi cảm biến; `sharedStateGetUpdatedMs()` |
| `firmware/sensor-node/src/main.cpp` | (`TBS_LATENCY_PROBE`) log `LAT TX`/`LAT RTT`/`LAT MQTT_TX`; telemetry MQTT luôn có `"seq"` |
| `firmware/sensor-node/platformio.ini` | Env `yolo_uno_latency`, `yolo_uno_coreiot_latency` |
| `firmware/waveshare-screen/components/espnow_receiver/espnow_receiver.c`, `CMakeLists.txt` | (`TBS_LATENCY_PROBE`) echo seq ngay trong callback nhận + log `LAT: RX`; add peer broadcast |
| `firmware/waveshare-screen/src/main.c` | Log `LAT: MQTT_RX seq=` khi attribute có `seq` |
| `firmware/waveshare-screen/platformio.ini` | Env `yolo_uno_latency` (`board_build.cmake_extra_args = -DTBS_LATENCY_PROBE=1`) |
| `cloud/coreiot/rule_chain/supersonic_rule_chain.json` | Transform chuyển tiếp `seq` sang shared attribute màn hình |
| `tools/latency/measure_latency.py`, `test_measure_latency.py` | Ghi song song 2 cổng serial (tự dò theo VID:PID), CSV, thống kê n/min/p50/p95/p99/max/mean/std, tỷ lệ nhận |
| `docs/LATENCY_TEST.md` | Quy trình đo |

Firmware thường (`yolo_uno`, `yolo_uno_coreiot`) không đổi hành vi ngoài trường `seq` trong telemetry MQTT.

## Kết quả kiểm thử (không cần board)
- `pio run` sensor-node: `yolo_uno`, `yolo_uno_coreiot`, `yolo_uno_latency`, `yolo_uno_coreiot_latency` — SUCCESS.
- `pio run` waveshare-screen: `yolo_uno_latency` SUCCESS (06/10, 15 phút; RAM 32,8 %, Flash 32,0 %) — `firmware.elf` có `RX seq=%u` và cảnh báo "LATENCY PROBE build"; `yolo_uno` SUCCESS và **không** chứa chuỗi LAT RX (bản thường không echo).
- Build lại 06/10 sau khi thêm soak heartbeat: sensor-node 5 env SUCCESS (`yolo_uno`, `yolo_uno_coreiot`, `yolo_uno_latency`, `yolo_uno_coreiot_latency`, `yolo_uno_accuracy`); chuỗi `LAT RTT` chỉ có trong 2 env `*_latency`.
- Script tự dò cổng theo VID:PID (máy dev: sensor-node COM7, màn hình COM9; COM4 là Bluetooth).
- `pytest tools/latency`: 8 passed. `pytest tools/guard/test_guard.py`: 30 passed.
- `scan_secrets.py` OK, `arch_guard.py` OK, `check_rulechain_thresholds.py` OK.

## Việc còn lại (cần người + board)
1. Import lại rule-chain lên CoreIoT (cho kịch bản MQTT).
2. Flash env `*_latency` cho 2 board, chạy 3 lượt A1/A2/B1 theo `docs/LATENCY_TEST.md`.
3. Dán bảng số vào mục kết quả dưới đây, `docs/PROGRESS.md` (T5.2) và Jira DMXT-57.

## Kết quả đo
_(chưa có)_
