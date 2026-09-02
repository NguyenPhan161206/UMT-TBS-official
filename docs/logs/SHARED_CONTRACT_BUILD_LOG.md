# Shared Contract Build - Log

Mục tiêu: tạo `firmware/shared/` — nguồn contract duy nhất (R2) giữa 2 board
sensor-node & waveshare-screen: `thresholds.h` (ngưỡng + layout, R3/R4) và
`espnow_protocol.h` (wire schema ESP-NOW, R2). Khắc phục lỗ hổng của cựu repo
`supersonic-warning-system` nơi `espnow_sensor_msg_t` bị copy trùng 2 nơi và ngưỡng
cảnh báo rải 4 chỗ.

## Môi trường
- OS: Linux, GCC 13 (host) cho test C11/C++17.
- Repo V2: `UMT-TBS-official` (Greenfield). Giai đoạn B2 của ROADMAP_CHECKLIST.

## Các file thay đổi (mới)
- `firmware/shared/thresholds.h` (119 dòng) — ngưỡng zone/buzzer/đo, `SENSOR_PINS`
  (6 cặp tr/echo, tránh GPIO47/48 PSRAM), `SENSOR_COUNT = sizeof(...)` + `static_assert`.
- `firmware/shared/espnow_protocol.h` (66 dòng) — `espnow_sensor_msg_t` (packed, 30B),
  `espnow_slot_t` enum, `ESPNOW_CHANNEL=1`, `ESPNOW_PEER_MAC`, `ESPNOW_SENSOR_SLOT_COUNT`,
  + static_assert khớp `SENSOR_COUNT`.

## Kết quả kiểm thử (DoD)

### 1. Compile host — C11 & C++17
```bash
gcc -std=c11 -Wall -Wextra -I. /tmp/tbs_test_thresholds.c ...   # C11 build OK, run exit 0
g++ -std=c++17 ... /tmp/tbs_test_thresholds.cpp ...             # C++17 build OK, run exit 0
gcc -std=c11 ... /tmp/tbs_test_proto.c ...                      # C11 OK, msg size = 30 bytes (packed)
g++ -std=c++17 ... /tmp/tbs_test_proto.cpp ...                  # C++17 OK, run exit 0
```
Cả 2 board (Arduino C++ / ESP-IDF C) include được cùng header.

### 2. R2 — shared contract 1 nguồn
```bash
grep -rn "espnow_sensor_msg_t;" firmware/ | wc -l   # 1  (chỉ espnow_protocol.h)
grep -rn "espnow_sensor_msg_t" firmware/            # 2 dòng, CÙNG 1 file (1 comment + 1 def)
```

### 3. R3 — ngưỡng không lặp ngoài shared
```bash
grep -rEn "SENSOR_CAUTION_CM|SENSOR_DANGER_CM|BUZZER_WARNING_DISTANCE_CM|BUZZER_DANGER_DISTANCE_CM" firmware/ | grep -v "firmware/shared/"
# (rỗng) → OK
```

### 4. R4 — SENSOR_COUNT suy từ sizeof + static_assert
```bash
grep -n "SENSOR_COUNT" firmware/shared/thresholds.h
# line 106: #define SENSOR_COUNT (sizeof(SENSOR_PINS)/sizeof(SENSOR_PINS[0]))
# line 115: TBS_STATIC_ASSERT(SENSOR_COUNT == 6, ...)
```

### 5. R1 — no secret
```bash
python3 tools/guard/scan_secrets.py   # SECRET-SCAN OK, exit 0
```

### 6. R7 — kích thước file
`thresholds.h` 119 dòng, `espnow_protocol.h` 66 dòng — đều ≤ 400.

### 7. Rule-chain đối chiếu (R11, best-effort)
```bash
python3 tools/guard/check_rulechain_thresholds.py
# CHECK-RULECHAIN SKIP (chưa có cloud/coreiot/rule_chain/* -> làm ở B9). Đúng trạng thái.
```

## Ghi chú / quyết định
- Ngưỡng giữ giá trị thực tế cựu repo (theo yêu cầu): zone 100/30 cm, buzzer 50/20 cm.
- `SensorPinConfig` → đổi tên `sensor_pin_cfg_t` (C-compatible, shared dùng được cả 2 board).
- `_Static_assert` → macro `TBS_STATIC_ASSERT` bọc theo C/C++.
- Slot enum ESP-NOW thống nhất với thứ tự vật lý `SENSOR_PINS` (không còn `SENSOR_ESPNOW_SLOT`
  map phụ — thứ tự đã thẳng hàng).

## Hướng dẫn demo / bước tiếp
- Giai đoạn B3: scaffold `firmware/sensor-node` include 2 header này (include path trỏ `../shared`).
- Giai đoạn B4: `firmware/waveshare-screen` include 2 header này.
- B9: tạo `cloud/coreiot/rule_chain/supersonic_rule_chain.json` rồi chạy lại `check_rulechain_thresholds.py`.
