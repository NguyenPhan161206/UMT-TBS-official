# SENSOR_NODE_SCAFFOLD_LOG — B3: sensor-node scaffold (V2)

> Ngày: 2026-09-02 · Firmware: `firmware/sensor-node/` · Bước roadmap: B3

## Mục tiêu
Dựng sensor-node (ESP32-S3, Arduino framework) theo kiến trúc V2: đọc 6 cảm biến
JSN-SR04T → lọc cluster-EMA → ESP-NOW (đường chính) + MQTT CoreIoT (đường phụ qua
plugin sau `USE_COREIOT`). Mọi contract dùng chung lấy từ `firmware/shared/` (R2/R3/R4),
không định nghĩa lại; bỏ demo `appTask` khỏi production (R5).

## File đã tạo/sửa
| File | Vai trò |
|------|---------|
| `platformio.ini` | 3 env: `yolo_uno` (ESP-NOW only), `yolo_uno_coreiot` (USE_COREIOT=1, include plugin, PubSubClient), `native` (host unity test, `test_build_src=yes` + `+<distance_filter.cpp>`) |
| `boards/yolo_uno.json` | Board config copy từ cựu repo (Yolo_Uno_S3, 8MB flash) |
| `include/ultrasonic_sensor.h` + `src/ultrasonic_sensor.cpp` | Trigger/echo JSN-SR04T, `SOUND_SPEED_CM_PER_US` từ thresholds.h |
| `include/distance_filter.h` + `src/distance_filter.cpp` | Lọc cluster-EMA, mảng tĩnh không heap; 369 dòng |
| `include/shared_state.h` + `src/shared_state.cpp` | Nơi lưu khoảng cách + `sharedStateGetNearest()` |
| `include/buzzer.h` + `src/buzzer.cpp` | Buzzer task theo ngưỡng thresholds.h (50/20cm) |
| `include/espnow_client.h` + `src/espnow_client.cpp` | Gửi frame `espnow_sensor_msg_t` (shared) cho waveshare-screen |
| `src/main.cpp` | SensorTask(core1)/NetworkTask(core0)/BuzzerTask(core0)/CoreIoTTask(core0,#if USE_COREIOT); mảng `SENSOR_ESPNOW_SLOT[]`; cảnh báo GPIO reserved; KHÔNG appTask demo |
| `src/plugins/coreiot/coreiot_client.{h,cpp}` | MQTT PubSubClient đọc `credentials.h` (sinh từ keys.json), không hardcode secret |
| `test/test_thresholds.cpp` | Host test shared contract (R3/R4) — 5 test |
| `test/test_distance_filter.cpp` | Host test DistanceFilter — 5 test |
| `test/test_runner.cpp` | main() duy nhất cho unity native |
| `tools/guard/gen_credentials.py` | **Sửa lỗi**: `COREIOT_PORT` là int → `.startswith` crash; dùng `str(...)` |

## Quyết định kiến trúc
- **`SENSOR_ESPNOW_SLOT[]`**: thứ tự vật lý `SENSOR_PINS` KHÔNG khớp `espnow_slot_t`
  (bảng README cựu repo). Giữ bảng ánh xạ rõ ràng, chống sai nhãn dashboard.
- **Plugin CoreIoT** compile theo `USE_COREIOT` (R6): env `yolo_uno_coreiot` bắt buộc
  build trong CI. Env thường (`yolo_uno`) không kéo dependency PubSubClient.
- **`build_src_filter`** dùng pattern tương đối với `src_dir` (`+<*>` không phải `+<src/>`);
  native test bật `test_build_src=yes` (mặc định off trong PlatformIO ≥6.1).

## Kết quả kiểm thử (DoD)
```bash
# 1) Build thường
cd firmware/sensor-node && ~/.venv-platformio/bin/pio run -e yolo_uno          # SUCCESS (RAM 13.5% / Flash 20.6%)
# 2) Build CoreIoT (cần credentials.h tạm sinh từ keys.json local — gitignored)
~/.venv-platformio/bin/pio run -e yolo_uno_coreiot                            # SUCCESS (RAM 14.0% / Flash 21.2%)
# 3) Host unit test (10/10 PASSED)
~/.venv-platformio/bin/pio test -e native
# 4) Secret scan
python3 tools/guard/scan_secrets.py                                           # exit 0
# 5) Shared contract 1 nguồn
grep -rn "espnow_sensor_msg_t;" firmware/          # 1 dòng (firmware/shared/espnow_protocol.h)
grep -rn "define SENSOR_CAUTION_CM" firmware/      # 1 dòng (firmware/shared/thresholds.h)
grep -rn "define SENSOR_COUNT" firmware/           # 1 dòng (sizeof, R4)
# 6) Kích thước file: max project = distance_filter.cpp 368 dòng (<=400, R7)
```
> Ghi chú env coreiot: `config/keys.json` + `include/credentials.h` là gitignored; sinh bằng
> `python3 tools/guard/gen_credentials.py --out include/credentials.h` sau khi user điền token
> MỚI (B0b/B9). Build local dùng giá trị test rồi đã xoá sạch.

## Hướng dẫn vận hành / demo
1. Nạp firmware: `pio run -e yolo_uno -t upload --upload-port /dev/ttyACM0`
2. Monitor: `pio device monitor -p /dev/ttyACM0 -b 115200`
3. Quan sát log: mỗi sensor in giá trị sau lọc; khi khoảng cách < 50cm buzzer kêu,
   frame ESP-NOW gửi mỗi 500ms tới waveshare-screen (MAC trong shared header).
4. Bản CoreIoT: điền token mới vào `config/keys.json` rồi build env `yolo_uno_coreiot`;
   telemetry topic `v1/devices/me/telemetry`.

## Vi phạm ghi nhận
- Không có (guard gen_credentials.py có bug nhỏ về `str()` đã sửa trong chính bước này).