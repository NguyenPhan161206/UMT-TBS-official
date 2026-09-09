# Nhánh Tiếp Theo — Orchestration State
Roadmap: docs/roadmaps/next-branch.roadmap.json
Created: 2026-09-08
Updated: 2026-09-09 (đảo thứ tự G1 theo kiến trúc — xem Độ lệch)
Priorities: G1 Mở khả năng kiểm thử (T1.x) → G2 P0 còn lại (T2.3, T3.4) → tính năng (T3.1/T3.2/T3.3) → đo & vệ sinh (T5.x)

| Step | Title | Status | Verified by | Notes |
|------|-------|--------|-------------|-------|
| 1 | T1.3 hazard_core — tách logic quyết định thuần + unit test | TODO | — | foundation G1/G2; test boundary 19/30/31... |
| 2 | T1.1 tools/scenarios.py + test_mqtt_coreiot.py --scenario | TODO | — | kịch bản 1 chỗ; pin JSONL schema |
| 3 | T1.2 LVGL SDL simulator (host_sim, kéo sớm) | TODO | — | FetchContent LVGL v9.1 + SDL2 + stub; prereq T3.1/T3.2 |
| 4 | T1.4 record_telemetry.py + replay_telemetry.py | TODO | — | record cần board+token → để cuối G1 |
| 5 | T2.3 Khôi phục crossing_hazard (rule-chain) | TODO | — | firmware nhận sẵn (main.c:84-87); dead-path là rule-chain snapshot thiếu field |
| 6 | T3.4 Ngưỡng buzzer DANGER-only theo SENSOR_DANGER_CM | TODO | — | bỏ BUZZER_WARNING/DANGER riêng |
| 7 | T2.4+x Nghiệm thu link/data-cũ + buzzer trên board | TODO | — | cần board + token thật |
| 8 | T3.1 Biểu tượng vật thể tại vị trí phát hiện | TODO | — | cần step 3 (sim — đã kéo sớm) |
| 9 | T3.2 Sơ đồ xe tải EX8 theo profile | TODO | — | bỏ hình gắn cứng |
| 10 | T3.3 Mạch khuếch đại buzzer 5V (transistor) | TODO | — | GPIO11 không ra 5V; cần trả lời active/passive |
| 11 | T5.2 Nút Calibrate: callback hoặc bỏ | TODO | — | hiện là label không event |
| 12 | T5.3 Đối chiếu comment ID cảm biến | TODO | — | sensor_model.h vs espnow_protocol.h |
| 13 | T5.5+T5.6 Đo baseline + tầm/góc búp | TODO | — | cần board + không gian; replay = step 4 |
| 14 | T5.7 Đo độ trễ đầu-cuối 2 nhánh (chốt T0.2) | TODO | — | ESP-NOW <50ms; MQTT 100-500ms |
| 15 | T5.8+T5.9 Soak 24/72h + báo nhầm/spot sót | TODO | — | cần step 4 + 14 |
| 16 | T5.x Dọn config cứng (A/B/C theo HARDCODED_CONFIG_NOTES.md) | TODO | — | note→checklist lúc đầu; code dọn khi step chạm file; prereq step 3+9 |

## Contracts to establish
- `hazard_core` (components/hazard_core): `hazard_classify`/`hazard_worst_zone`/`hazard_eval_crossing` — C thuần, zero OS/LVGL dep; `CROSSING_DELTA_CM`(40)/`CROSSING_FRONT_THRESHOLD_CM`(150) về đây.
- `crossing_hazard` bool do node 3 rule-chain xuất (JS: FRONT<=150 && side delta>=40), waveshare đọc tại `main.c:84-87` → `s_forced_crossing_warning` OR `hazard_eval_crossing` — kết thúc dead-path.
- `tools/scenarios.py`: kịch bản approach/crossing/slam/normal + JSONL schema = payload V2 telemetry (d1..d6/nearest_cm/has_nearest/timestamp/seq).
- `tools/test_mqtt_coreiot.py --scenario <name>`; `tools/record_telemetry.py --seconds N --out file.jsonl`; `tools/replay_telemetry.py --in file.jsonl --dry-run`.
- Bỏ `BUZZER_WARNING_DISTANCE_CM`/`BUZZER_DANGER_DISTANCE_CM`; buzzer kêu khi `nearest_cm <= SENSOR_DANGER_CM` (30), im khi còn lại.
- Config cứng/khó bảo trì: checklist nguồn `docs/HARDCODED_CONFIG_NOTES.md` (A: threshold copy string drift `ui_dashboard_layout.c:299-301` + `test_mqtt_coreiot.py:41-42`; B: `offset_deg` dead + geometry 2 nơi; C: timing literal ẩn; D: loại trừ). Step 16 track; A được bảo vệ bởi arch_guard.py (B5).

## Deviations
- **2026-09-09 — Thêm step 16 (T5.x dọn config cứng):** người dùng chọn "note lại 1 file riêng" → tạo `docs/HARDCODED_CONFIG_NOTES.md` + đưa vào checklist roadmap (step 16, prereq [3,9]). Không sửa code ở lần này; từng đám được xử lý đúng trong step chạm file (legend→host_sim/T3.x, geometry→T3.2, timing→step 16).
- **2026-09-09 — Đảo thứ tự G1 (kiến trúc 3 lớp + kịch bản chung, nguồn `docs/ARCHITECTURE_G1_TESTING.md`):**
  1. `hazard_core` + refactor (thay vì T1.3a pio-native hẹp) — làm đầu vì là foundation của T1.3/host_sim/T2.3.
  2. `scenarios.py` + `--scenario` (T1.1) — quick win, pin schema cho T1.4 & sim.
  3. `host_sim` (T1.2) — KÉO LÊN từ cuối lên thứ 3: rủi ro/lợi nhuận lớn nhất → fail-fast; là prereq của T3.1/T3.2; không phụ thuộc T1.4 (chỉ cần schema).
  4. `record/replay` (T1.4) — ĐẨY XUỐNG CUỐI: nửa record phụ thuộc board+token (vật lý), nửa replay trivially đúng khi schema pin sẵn.
  - Cập nhật prereq: step 5 → [1] (hazard_core); step 8 → [3] (sim); step 13 → [4] (replay); step 15 note = step 4.

## Context gốc (đừng suy lại mỗi lần)
- Code hiện = origin/main (main là branche duy nhất + khoa cũ hơn; không có nhánh nguyenphan).
- Fix ESP-NOW (race WiFi vs channel, modem-sleep) đã phân tích nhưng ĐÃ REVERT theo yêu cầu — step 7 sẽ nghiệm thu lại.
- CoreIoT: sensor-node Inactive từ 14:59:36 (thiếu MQTT telemetry — cần 3 xác nhận: `[NET] MQTT connected`, board đang flash env nào, telemetry d1..d6 latest).
- Buzzer: GPIO11 không thể xuất 5V → cần transistor + rail 5V (chưa có câu trả lời active/passive).
- LaTeX: pdflatex TeX Live 2023 tại /home/binhnguyen/.local/bin; report/Makefile `make` (3 lần) + `make verify` grep 4 keyword.