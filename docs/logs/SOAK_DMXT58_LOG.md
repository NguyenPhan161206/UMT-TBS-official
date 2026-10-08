# SOAK — Heartbeat soak test 24 h (DMXT-58)

- **Ngày**: 2026-10-06
- **Trạng thái**: đã chạy trên board 07–08/10 — **18,47 h liên tục, đạt cả 3 tiêu chí**; lượt 24 h bị cắt do PC mất nguồn (không phải lỗi firmware). Người dùng chọn lấy 18,5 h làm kết quả.

## Mục tiêu
Có số liệu 24 h liên tục cho bài: số lần reset + lý do, heap (đầu/cuối/min, độ dốc), tỷ lệ nhận ESP-NOW,
số lần mất link, số lần MQTT kết nối lại, stack HWM. Heartbeat bật trong firmware thường (không env riêng).

## File đã sửa
| File | Thay đổi |
|---|---|
| `firmware/shared/soak_diag.h` (mới) | `TBS_SOAK_HEARTBEAT_INTERVAL_MS` = 60000; `tbs_reset_reason_name()` |
| `firmware/sensor-node/include/soak_heartbeat.h`, `src/soak_heartbeat.cpp` (mới) | Bộ đếm boot (Preferences `diag/boot`), dòng `BOOT node`, `SOAK node` mỗi 60 s (heap/minheap/blk, tx, mqtt_rc, HWM 3 task) |
| `firmware/sensor-node/src/espnow_client.cpp`, `include/espnow_client.h` | Đếm `tx_ok`/`tx_fail` (onDataSent + esp_now_send lỗi) |
| `firmware/sensor-node/src/plugins/coreiot/coreiot_client.{h,cpp}` | `reconnectCount()` |
| `firmware/sensor-node/src/main.cpp` | Gọi `soakHeartbeatBoot()` trong setup, `soakHeartbeatPoll()` trong networkTask |
| `firmware/waveshare-screen/src/soak_heartbeat.{h,c}` (mới), `src/CMakeLists.txt` | Bộ đếm boot NVS `diag/boot`, `BOOT scr`, task `soak` in `SOAK scr` mỗi 60 s (heap nội/PSRAM, rx, linkdn, maxgap, mqtt_rc, HWM task lvgl) |
| `firmware/waveshare-screen/components/espnow_receiver/espnow_receiver.{h,c}` | `espnow_receiver_rx_count()`, `espnow_receiver_take_max_gap_ms()` |
| `firmware/waveshare-screen/components/coreiot_client/coreiot_client.{h,c}` | `coreiot_client_reconnect_count()` |
| `firmware/waveshare-screen/src/main.c` | `soak_heartbeat_start()` sau NVS init; đếm link UP→DOWN trong watchdog |
| `tools/soak/soak_logger.py`, `test_soak_logger.py` (mới) | record (2 cổng, không DTR/RTS, tự mở lại cổng, CSV flush từng dòng, chặn spam lỗi) + analyze |
| `docs/SOAK_TEST.md` (mới) | Quy trình, chuẩn bị PC, tiêu chí |

## Kiểm thử (không cần board)
- Build sensor-node: `yolo_uno`, `yolo_uno_coreiot`, `yolo_uno_latency`, `yolo_uno_coreiot_latency` — SUCCESS (06/10).
- Build màn hình: xem mục "Build màn hình" bên dưới.
- `pytest tools/soak`: 9 passed (gồm test mở lại cổng khi USB mất, đếm reset bị lỡ dòng BOOT, cộng dồn bộ đếm qua reset).

## Build màn hình
- `yolo_uno` — SUCCESS (06/10, 21 phút 45 giây do CMake cấu hình lại khi thêm `soak_heartbeat.c`); RAM 32,8 %, Flash 32,0 %.
  Đã kiểm: có `.pio/build/yolo_uno/src/soak_heartbeat.c.o`; `firmware.elf` chứa `SOAK scr up=` và `BOOT scr boot=`,
  không chứa chuỗi của bản đo độ trễ.
- `yolo_uno_latency` — xem `docs/logs/LATENCY_DMXT57_LOG.md`.

## Chưa kiểm chứng trên phần cứng
- Dòng BOOT/SOAK thật trên COM4/COM9, giá trị `hwm_lvgl` (tên task `lvgl`), hành vi tự mở lại cổng khi board reset thật.

## Kết quả soak

### Firmware & điều kiện
| Board | Nhánh / commit | Env | ELF lưu tại |
|---|---|---|---|
| sensor-node (COM7) | `nguyen` `752ab27` (theo yêu cầu: dashboard có raw/filter) | `yolo_uno_coreiot_algotest` (Hybrid + 18 trường `std/raw/flt` trên MQTT) | `data/soak/sensor_nguyen_752ab27_algotest.elf` |
| màn hình (COM9) | `khoa` `f05b62f` | `yolo_uno` | `data/soak/screen_khoa_f05b62f_yolo_uno.elf` |

- Wi-Fi hotspot Windows, CoreIoT bật. 2 cảm biến: D6/D7 (GPIO 9/10, R-Front) và D8/D9 (GPIO 17/18, L-Rear); **không đặt vật cản** —
  cảm biến đọc tường ổn định ~341 / ~326 cm (luồng đo/lọc/gửi luôn chạy). Trong phòng.
- **Lưu ý:** số soak đo trên bản thử nghiệm algotest, không phải bản phát hành. sensor-node `nguyen` thiếu bản sửa thứ tự `d1..d6`
  (`e77360c`) — chỉ ảnh hưởng nhãn dữ liệu, không ảnh hưởng độ ổn định.
- `linkdn=1` ở màn hình có từ **trước** khi bắt đầu (lúc nạp lại sensor-node sang bản `nguyen`, trước 01:01); trong soak không tăng.

### Bảng chính thức — 08/10 01:01 → 19:29 (18,47 h liên tục)
File: `data/soak/soak18h_algotest_seg1_0101-1929.csv` (cắt từ `soak24h_algotest_20261008_010116.csv`), tổng kết
`data/soak/soak18h_algotest_seg1.md` / `.json`.

| Đại lượng | sensor-node | màn hình |
|---|---|---|
| Dòng SOAK | 1108 (thiếu 1 nhịp > 90 s) | 1109 (thiếu 0) |
| Reset | **0** | **0** |
| Heap trống đầu → cuối | 256,9 → 256,9 KB | 62,2 → 62,2 KB (RAM nội) |
| Độ dốc heap trống | **+0,001 KB/h** | **−0,002 KB/h** |
| Heap min-ever (sau 10 phút đầu → cuối) | 249,1 → 248,3 KB | 54,8 → 50,9 KB |
| PSRAM trống | — | 7281,4 KB, không đổi |
| Stack HWM thấp nhất | buzz 1172 B, net 1732 B, sensor 1932 B | lvgl 6156 B |
| MQTT reconnect | 0 | 0 |

- **ESP-NOW:** gửi 664 388 gói (lỗi 0), màn hình nhận 663 056 → **99,80 %**; mất link **0** lần; khoảng hở lớn nhất **317 ms**
  (không cửa sổ nào > 1500 ms).
- **Tiêu chí (`docs/SOAK_TEST.md` §5):** 0 reset ngoài ý muốn — ĐẠT cả 2 board; không mất link > 1500 ms — ĐẠT; heap đi ngang — ĐẠT
  (độ dốc ≈ 0). Min-ever của màn hình là đáy thấp nhất từng chạm (giảm 3,9 KB trong 18 h, còn 50,9 KB), không phải xu hướng heap trống.
- **Lỗi đã biết:** màn hình in `E esp_lvgl:adapter: Failed to acquire LVGL lock` **11 lần / 18,5 h** — callback (`src/main.c`) chờ
  khóa LVGL quá 500 ms thì bỏ 1 lần cập nhật UI; không gây reset, ESP-NOW không bị ảnh hưởng. Cần điều tra riêng.

### Sự cố cắt ngang (không phải lỗi firmware)
| Thời điểm 08/10 | Sự kiện (Windows System log) |
|---|---|
| 19:29:42 | Power source change — dây sạc bị tuột, laptop chạy pin |
| 19:29–19:53 | Modern Standby; logger mất cổng USB (`PORT_LOST` 19:33, 19:53) |
| 19:53:24 | "Hibernate from Sleep — Standby Battery Budget Exceeded" → USB mất điện, 2 board tắt |
| 20:56:09 | Cắm lại sạc, máy thức; 2 board boot lại `rr=POWERON` (sensor `boot` 36→38, màn hình 20→21) |

Dữ liệu sau 19:29 không dùng. Bài học: 2 board lấy nguồn từ USB laptop → nguồn PC là điểm hỏng chung; lần sau cố định dây sạc và
đặt cả chế độ pin (DC) không sleep/hibernate, hoặc cấp nguồn board bằng adapter riêng.

### Lượt trước (bị dừng có chủ đích)
- `try30m_20261007_214039.csv` (bản `khoa` `yolo_uno_coreiot`, 30 phút): 0 reset, nhận 99,65 %, đạt tiêu chí.
- `soak24h_20261007_223224.csv` (bản `khoa`, 07/10 22:32 → 23:41, 1,15 h): 0 reset, 0 mất link — dừng để nạp bản `nguyen` theo yêu cầu dashboard.
