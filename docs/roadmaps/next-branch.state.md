# Nhánh Tiếp Theo — Orchestration State
Roadmap: docs/roadmaps/next-branch.roadmap.json
Created: 2026-09-08
Priorities: G1 Mở khả năng kiểm thử (T1.x) → G2 P0 còn lại (T2.3, T3.4) → tính năng (T3.1/T3.2/T3.3) → đo & vệ sinh (T5.x)

| Step | Title | Status | Verified by | Notes |
|------|-------|--------|-------------|-------|
| 1 | T1.1 test_mqtt_coreiot.py: schema V2 + --scenario | TODO | — | --scenario approach/crossing/slam/normal |
| 2 | T1.3a Host unit test sensor_model_classify | TODO | — | waveshare test/ + env native |
| 3 | T1.4 record_telemetry.py + replay_telemetry.py | TODO | — | JSONL thật, tái hiện cảnh |
| 4 | T1.2 LVGL SDL simulator trên PC | TODO | — | host_sim/, cần CMake + SDL2 + LVGL v9 |
| 5 | T2.3 Khôi phục crossing_hazard (rule-chain + firmware) | TODO | — | dead-path main.c:84-87 |
| 6 | T3.4 Ngưỡng buzzer DANGER-only theo SENSOR_DANGER_CM | TODO | — | bỏ BUZZER_WARNING/DANGER riêng |
| 7 | T2.4+x Nghiệm thu link/data-cũ + buzzer trên board | TODO | — | cần board + token thật |
| 8 | T3.1 Biểu tượng vật thể tại vị trí phát hiện | TODO | — | cần step 4 (sim) |
| 9 | T3.2 Sơ đồ xe tải EX8 theo profile | TODO | — | bỏ hình gắn cứng |
| 10 | T3.3 Mạch khuếch đại buzzer 5V (transistor) | TODO | — | GPIO11 không ra 5V; cần trả lời active/passive |
| 11 | T5.2 Nút Calibrate: callback hoặc bỏ | TODO | — | hiện là label không event |
| 12 | T5.3 Đối chiếu comment ID cảm biến | TODO | — | sensor_model.h vs espnow_protocol.h |
| 13 | T5.5+T5.6 Đo baseline + tầm/góc búp | TODO | — | cần board + không gian |
| 14 | T5.7 Đo độ trễ đầu-cuối 2 nhánh (chốt T0.2) | TODO | — | ESP-NOW <50ms; MQTT 100-500ms |
| 15 | T5.8+T5.9 Soak 24/72h + báo nhầm/spot sót | TODO | — | cần step 3 + 14 |

## Contracts to establish
- `crossing_hazard` bool do node 3 rule-chain xuất (JS: FRONT<=150 && side delta>=40), waveshare đọc tại `main.c:84-87` — kết thúc dead-path.
- Bỏ `BUZZER_WARNING_DISTANCE_CM`/`BUZZER_DANGER_DISTANCE_CM`; buzzer kêu khi `nearest_cm <= SENSOR_DANGER_CM` (30), im khi còn lại.
- `tools/record_telemetry.py --seconds N --out file.jsonl` + `tools/replay_telemetry.py --in file.jsonl`.

## Deviations
(TBD — cập nhật khi worker thực hiện.)

## Context gốc (đừng suy lại mỗi lần)
- Code hiện = 6326682 = origin/main (main là branche duy nhất + khoa cũ hơn; không có nhánh nguyenphan).
- Fix ESP-NOW (race WiFi vs channel, modem-sleep) đã phân tích nhưng ĐÃ REVERT theo yêu cầu — step 7 sẽ nghiệm thu lại.
- CoreIoT: sensor-node Inactive từ 14:59:36 (thiếu MQTT telemetry — cần 3 xác nhận: `[NET] MQTT connected`, board đang flash env nào, telemetry d1..d6 latest).
- Buzzer: GPIO11 không thể xuất 5V → cần transistor + rail 5V (chưa có câu trả lời active/passive).
- LaTeX: pdflatex TeX Live 2023 tại /home/binhnguyen/.local/bin; report/Makefile `make` (3 lần) + `make verify` grep 4 keyword.