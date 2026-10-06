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

## Việc còn lại
1. ~~Import rule-chain~~ (xong 06/10 22:36, đặt Root; đổi tên device `waveshare-sreen` → `waveshare-screen`).
2. ~~A1~~, ~~B1~~ xong 06/10. A2 (khoảng cách xa hơn) — tuỳ chọn, cần cáp dài hoặc sửa script cho phép chỉ cắm sensor-node.
3. Gửi comment Jira DMXT-57 (nháp ở `docs/logs/JIRA_COMMENT_DRAFTS.md`, chờ duyệt).

## Kết quả đo

### Sửa lỗi trước khi đo (06/10 ~22:10)
- Lượt kiểm tra đầu: màn hình nhận đủ và gửi echo, nhưng sensor-node **không tính RTT** — `onDataRecv()` thiếu nhánh
  chuyển gói echo 3 byte sang `handleEcho()`. Đã thêm nhánh (chỉ trong `TBS_LATENCY_PROBE`), build lại `yolo_uno_latency`,
  `yolo_uno_coreiot_latency`, `yolo_uno` SUCCESS, nạp lại sensor-node.
- `measure_latency.py`: tỷ lệ nhận dùng **dải seq đã gửi** làm mẫu số (dòng log `LAT TX` qua USB có thể rớt trong khi gói
  radio vẫn đi) + báo số dòng log rớt. 9 pytest pass.

### Sửa lỗi trước lượt MQTT (06/10 ~22:45)
- CoreIoT: device màn hình đặt tên sai `waveshare-sreen` (rule-chain tìm `waveshare-screen`) → người dùng đổi tên;
  import rule-chain mới (22:36) và đặt làm Root.
- Màn hình không nối được hotspot: khi chưa từng nối AP, `coreiot_client` chỉ quét kênh 6 (`ESPNOW_CHANNEL`), còn hotspot
  Windows phát cùng kênh Wi-Fi mà PC đang nối (kênh 5). Sửa: chưa biết kênh AP thì quét mọi kênh; đã nối thì khóa kênh AP như
  cũ. Build lại màn hình `yolo_uno_latency` + `yolo_uno` SUCCESS 22:49 (elf có chuỗi "scan all channels").
- Cả 2 board vẫn không nối hotspot (màn hình: "Reason code 15" — lỗi bắt tay xác thực): `credentials.h` mang mật khẩu Wi-Fi cũ,
  `keys.json` đã đổi (khớp hotspot). Sinh lại `credentials.h` 2 board bằng `gen_credentials.py`, build lại 4 env có Wi-Fi
  SUCCESS 22:58–22:59; đã kiểm firmware chứa mật khẩu mới (so khớp, không in).
- sensor-node: chỉ in `LAT MQTT_TX` khi MQTT đang kết nối (trước đó in cả khi publish chắc chắn thất bại → tính nhầm là mất).

### A1 — chỉ ESP-NOW, 2 board cách 1 m (06/10 22:21–22:26)
- File: `data/latency/espnow_1m_20261006_222143.csv` (+ `.json`). Thời lượng 300,9 s.
- Điều kiện: Wi-Fi tắt (màn hình không nối AP, ESP-NOW kênh 6), trong phòng, 1 cảm biến (cổng 9/10) + tấm chắn cố định,
  không người đi lại giữa 2 board. _(bổ sung nhiệt độ/vật cản nếu có)_

| Đại lượng (ms) | n | min | p50 | p95 | p99 | max | mean | std |
|---|---|---|---|---|---|---|---|---|
| ESP-NOW một chiều (RTT/2) | 2990 | 1,52 | 2,73 | 6,16 | 9,37 | 20,98 | 3,20 | 1,60 |
| ESP-NOW RTT | 2990 | 3,03 | 5,46 | 12,33 | 18,74 | 41,96 | 6,41 | 3,21 |
| Tuổi mẫu lúc gửi | 2966 | 0 | 52 | 102 | 134 | 163 | 51,7 | 31,6 |
| Tuổi mẫu + một chiều (đo → màn hình nhận) | 2957 | — | 54,9 | 106,1 | 137,4 | 159,7 | — | — |
| (đối chiếu, mốc PC — có jitter USB, không dùng) | 2961 | −15,9 | 4,88 | 8,43 | 12,61 | 36,65 | 5,35 | 2,24 |

- Gửi 2999 gói (10,0 gói/s), màn hình nhận **2994 (99,83 %)**, echo về 2990 (99,70 %). 5 gói mất **rời rạc**
  (seq 3048, 3123, 3305, 3936, 4270 — không có 2 gói mất liền nhau → khoảng hở lớn nhất ≈ 200 ms, dưới
  `ESPNOW_LINK_TIMEOUT_MS` 1500 ms và `SENSOR_STALE_TIMEOUT_MS` 1000 ms).
- Một chiều > 10 ms: 22 gói (0,74 %); > 15 ms: 3 gói (0,10 %). p50 theo từng phút 2,57–2,96 ms (ổn định).
- 33 dòng log `LAT TX` rớt trên USB (1,1 %) chỉ làm giảm n của tuổi mẫu, không ảnh hưởng RTT.
- Chưa gồm: thời gian màn hình đưa dữ liệu lên UI (timer dispatch 50 ms + vẽ LVGL) — không đo.

### B1 — Hybrid (ESP-NOW + MQTT CoreIoT), 2 board cách 1 m (06/10 23:06–23:21)
- File: `data/latency/hybrid_20261006_230646.csv` (+ `.json`). Thời lượng 900,1 s. Lượt thử trước: `smoke_mqtt_20261006_230311.csv` (30 s, không dùng).
- Điều kiện: sensor-node `yolo_uno_coreiot_latency`, màn hình `yolo_uno_latency`; cả 2 nối **hotspot Windows của PC**
  (2,4 GHz, kênh 5 = kênh Wi-Fi upstream "quán cà phê" công cộng PC đang nối) → Internet → app.coreiot.io; rule-chain
  "Supersonic…" (Root) chuyển `seq` sang shared attribute của `waveshare-screen`. Mốc thời gian MQTT lấy trên PC (sai số vài ms).

| Đại lượng (ms) | n | min | p50 | p95 | p99 | max | mean | std |
|---|---|---|---|---|---|---|---|---|
| **MQTT đầu–cuối** (publish → rule-chain → màn hình nhận) | 450 | 290,0 | **419,9** | **1000,9** | 1949,3 | 2343,3 | 500,5 | 292,5 |
| ESP-NOW một chiều (RTT/2), Wi-Fi bật | 8921 | 1,47 | **2,03** | **4,01** | 6,68 | 19,31 | 2,34 | 1,04 |
| ESP-NOW RTT | 8921 | 2,94 | 4,05 | 8,01 | 13,37 | 38,62 | 4,67 | 2,08 |
| Tuổi mẫu lúc gửi | 8842 | 0 | 51 | 101 | 130 | 155 | 52,4 | 31,8 |
| Tuổi mẫu + ESP-NOW một chiều | 8831 | — | 53,5 | 103,6 | 132,0 | 158,5 | — | — |

- MQTT: 450/450 bản tin tới màn hình (100 %); > 500 ms: 15,3 %, > 1 s: 5,1 %, > 2 s: 0,7 %, > 5 s: 0. p50 theo từng 3 phút
  411–434 ms (ổn định). Không có lần mất kết nối MQTT phía sensor-node (khoảng cách giữa 2 `MQTT_TX` ≤ 2,0 s).
- ESP-NOW: gửi 8932, màn hình nhận 8926 (**99,93 %**), echo 8921 (99,88 %); 6 gói mất rời rạc (không mất liền nhau →
  khoảng hở lớn nhất ≈ 200 ms); > 10 ms: 0,21 %, > 20 ms: 0. 87 dòng log `LAT TX` rớt trên USB (không ảnh hưởng RTT).
- So sánh: trung vị MQTT ≈ **207 ×** ESP-NOW (419,9 / 2,03 ms) → số liệu cho lập luận ESP-NOW là đường cảnh báo chính.
- A1 (Wi-Fi tắt, kênh 6) và B1 (Wi-Fi bật, kênh 5) khác điều kiện kênh/môi trường; không kết luận nguyên nhân chênh p50 (2,73 vs 2,03 ms).
- Hạn chế: MQTT phụ thuộc mạng (hotspot PC + Wi-Fi công cộng + máy chủ CoreIoT); chưa đo thời gian vẽ LVGL; đo trên bàn 1 m.