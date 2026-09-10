# Nhánh Tiếp Theo — Orchestration State
Roadmap: docs/roadmaps/next-branch.roadmap.json
Created: 2026-09-08
Updated: 2026-09-09 (đảo thứ tự G1 theo kiến trúc — xem Độ lệch)
Priorities: G1 Mở khả năng kiểm thử (T1.x) → G2 P0 còn lại (T2.3, T3.4) → tính năng (T3.1/T3.2/T3.3) → đo & vệ sinh (T5.x)

| Step | Title | Status | Verified by | Notes |
|------|-------|--------|-------------|-------|
| 1 | T1.3 hazard_core — tách logic quyết định thuần + unit test | DONE | pio run yolo_uno các build; host_sim 25 checks; arch_guard/pytest 19 | G1 foundation — xong 2026-09-09 (nhánh nguyen, chưa merge main) |
| 2 | T1.1 tools/scenarios.py + test_mqtt_coreiot.py --scenario | DONE | dry-run 8 mốc; pytest 25; arch_guard/scan OK | schema V2 pin; 4 kịch bản chuẩn — xong 2026-09-09 (chưa merge main) |
| 3 | T1.2 LVGL SDL simulator (host_sim, kéo sớm) | DONE | cmake+ctest 2/2; SDL dummy run exit 0; pio OK; pytest 25; arch/scan OK | FetchContent LVGL v9.1 + SDL2 + stub 10 file; prereq T3.1/T3.2 — xong 2026-09-10 (chưa merge main) |
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
| 16 | T5.x Dọn config cứng (A/B/C theo HARDCODED_CONFIG_NOTES.md) | IN_PROGRESS | — | A1 legend + B1 offset_deg ĐÃ XONG (dọn sớm 2026-09-09); còn A2 guard + C timing + B2 geometry (step 9) |

## Contracts established
- `hazard_core` (components/hazard_core, DONE): `sensor_zone_t hazard_classify(uint16_t)`; `sensor_zone_t hazard_worst_zone(const uint16_t*, const bool*, size_t)`; `hazard_crossing_result_t hazard_eval_crossing(const uint16_t*, const uint16_t*, size_t)` (struct `{bool active; espnow_slot_t sensor;}`, `HAZARD_CROSSING_NO_SENSOR ((espnow_slot_t)ESPNOW_SENSOR_SLOT_COUNT)`). C thuần zero OS/LVGL, only include `thresholds.h`+`espnow_protocol.h`; `CROSSING_DELTA_CM`(40)/`CROSSING_FRONT_THRESHOLD_CM`(150) định nghĩa ở `hazard_core.h` (B5 single-truth). Semantics: `x<DANGER`→DANGER, `x<=CAUTION`→CAUTION; stale-skip; crossing = FRONT<150 && side delta≥40, worst-side. Wiring: `ui_dashboard.c` 3 call-site + `evaluate_hazard`; wrapper `sensor_model_classify` đã xoá (R2 count=0). Host test: `host_sim/` (mini assert-runner, 25 checks, `ctest`-able). Guard: `tools/guard/arch_guard.py` (B1–B7 + mirror A2) — nối AGENTS/CI.
- `tools/scenarios.py` (DONE): `SCENARIO_NAMES=("approach","crossing","slam","normal")`,
  `all_scenarios[name] -> tuple[(d1..d6) cm]*mốc`, `iter_scenario(name)` (KeyError nếu lạ).
  Semantics: approach d1 160→20; crossing delta≥40 ở LF/RF; slam d1 110→60→20; normal mọi >100.
  Tiêu thụ: `test_mqtt_coreiot.py --scenario X` (`build_payload_from_distances(distances, seq)`,
  `nearest_cm=min`, JSONL schema `d1..d6/nearest_cm/has_nearest/timestamp/seq`); T1.4 + host_sim dùng chung.
- `tools/test_mqtt_coreiot.py --scenario <name>`; `tools/record_telemetry.py --seconds N --out file.jsonl`; `tools/replay_telemetry.py --in file.jsonl --dry-run`.
- `host_sim/umt_dash_sim` (DONE, step 3): CLI `--scenario <name>` (approach/crossing/slam/normal), `--replay <jsonl>`, `--exit-after <sec>`, `--interval <ms>`; LVGL v9.1.0 (FetchContent, khớp idf_component.yml) + SDL2, 800x480, LV_COLOR_DEPTH 16, tick = SDL_GetTicks; scenario data sinh từ `tools/scenarios.py` (nguồn duy nhất) qua `host_sim/gen_scenarios.py` → `scenarios_gen.h`; stub 10 file host-only (freertos mutex no-op, esp_log/esp_wifi/esp_system/esp_timer/esp_chip_info/esp_flash/esp_app_desc/coreiot_client MASKED-giả — R1); ctest `hazard_core_tests` + `umt_dash_sim_render` (SDL_VIDEODRIVER=dummy); CI dùng xvfb-run đúng DoD. Không đụng build firmware.
- Bỏ `BUZZER_WARNING_DISTANCE_CM`/`BUZZER_DANGER_DISTANCE_CM`; buzzer kêu khi `nearest_cm <= SENSOR_DANGER_CM` (30), im khi còn lại.
- Config cứng/khó bảo trì: checklist nguồn `docs/HARDCODED_CONFIG_NOTES.md` (A: threshold copy string drift `ui_dashboard_layout.c:299-301` + `test_mqtt_coreiot.py:41-42`; B: `offset_deg` dead + geometry 2 nơi; C: timing literal ẩn; D: loại trừ). Step 16 track; A được bảo vệ bởi arch_guard.py (B5).

## Deviations
- **2026-09-10 — Step 3 (T1.2 host_sim LVGL SDL) DONE, nhánh nguyen:** tạo `host_sim/umt_dash_sim` (LVGL v9.1.0 FetchContent + SDL2) compile file UI THẬT qua stub mỏng. Deviation so với prompt: (1) local không có xvfb-run/sudo → verify bằng `SDL_VIDEODRIVER=dummy` (tương đương headless, exit 0 + ctest 2/2), CI chạy xvfb-run đúng DoD; (2) `ui_dashboard.h` trong target_files không đổi vì đã là contract đúng — không cần sửa; (3) LV_COLOR_DEPTH 16 (RGB565 khớp màn hình). Verify: ctest 2/2, scenario approach 8/8 mốc exit 0, replay 2 dòng OK, pio sensor-node OK, pytest 25, arch_guard/scan_secrets OK. Log: `docs/logs/HOST_SIM_LOG.md`. Sẽ push origin/nguyen.
- **2026-09-09 — Bước 2 (step 2 T1.1) DONE, nhánh nguyen:** thêm `tools/scenarios.py` (4 kịch bản + `iter_scenario`) và `--scenario` vào `test_mqtt_coreiot.py`. Deviation nhỏ so với prompt gốc: `iter_scenario_rows` có thêm tham số `one_round_only=True` cho dry-run (tránh vô hạn khi người dùng kết hợp `--dry-run --loop`); `build_payload_from_distances` raise ValueError nếu không đủ 6 slot. Verify: dry-run đủ key, 8 mốc approach, pytest 25 pass, arch_guard/scan_secrets OK. Log: `docs/logs/TOOLS_SCENARIOS_LOG.md`. ĐÃ commit `6852cb3` + push origin/nguyen.
- **2026-09-09 — Bước 3 DONE (G1 step 1 hazard_core, nhánh nguyen):** tạo `components/hazard_core` (thuần, 3 hàm), host_sim mini assert-runner (25 checks), wiring `ui_dashboard.c` (3 call-site + evaluate_hazard) + xoá wrapper `sensor_model_classify`, bỏ CROSSING_* khỏi theme (B5 single-truth ở hazard_core.h), tạo `tools/guard/arch_guard.py` (B1–B7+mirror A2), nối AGENTS.md/CI. Verify: build waveshare clean OK, host_sim 25/25, arch_guard OK, pytest 19 pass, scan_secrets OK, grep R2/B5 = 0. Lưu ý môi trường máy dev: cần `pip install cmake pytest` vào venv-pio; ×1 lỗi build do `.pio` cache cũ không nhận component mới → `pio run -t clean`.<br>**Chưa merge main** — commit Bước 3 đang chuẩn bị push origin/nguyen. Step 5 (T2.3 dead-path) giờ prereq [1] đã đủ nhưng vẫn cần rule-chain snapshot piggyback.
- **2026-09-09 — Dọn sớm A1+B1 (config cứng step 16) trên nhánh nguyen:** user yêu cầu push lên nhánh `nguyen`. Xử lý trước phần A1 (legend UI → `lv_label_set_text_fmt` từ `SENSOR_*_CM`) + B1 (xoá `offset_deg` dead-field + `k_offsets_deg[]`) vì không phụ thuộc host_sim; nới prereq step 16 [3,9]→[9]. Còn A2 (arch_guard check python mirror) + C (timing literal ẩn) + B2 (geometry EX8) chờ step 1/9. Git nhánh nguyen = origin/main + commit dọn (đã push).
- **2026-09-09 — Thêm step 16 (T5.x dọn config cứng):** người dùng chọn "note lại 1 file riêng" → tạo `docs/HARDCODED_CONFIG_NOTES.md` + đưa vào checklist roadmap (step 16, prereq [3,9]). Không sửa code ở lần này; từng đám được xử lý đúng trong step chạm file (legend→host_sim/T3.x, geometry→T3.2, timing→step 16).
- **2026-09-09 — Đảo thứ tự G1 (kiến trúc 3 lớp + kịch bản chung, nguồn `docs/ARCHITECTURE_G1_TESTING.md`):**
  1. `hazard_core` + refactor (thay vì T1.3a pio-native hẹp) — làm đầu vì là foundation của T1.3/host_sim/T2.3.
  2. `scenarios.py` + `--scenario` (T1.1) — quick win, pin schema cho T1.4 & sim.
  3. `host_sim` (T1.2) — KÉO LÊN từ cuối lên thứ 3: rủi ro/lợi nhuận lớn nhất → fail-fast; là prereq của T3.1/T3.2; không phụ thuộc T1.4 (chỉ cần schema).
  4. `record/replay` (T1.4) — ĐẨY XUỐNG CUỐI: nửa record phụ thuộc board+token (vật lý), nửa replay trivially đúng khi schema pin sẵn.
  - Cập nhật prereq: step 5 → [1] (hazard_core); step 8 → [3] (sim); step 13 → [4] (replay); step 15 note = step 4.

## Context gốc (đừng suy lại mỗi lần)
- **Nhánh làm việc hiện tại = `nguyen`** (đã push lên origin/nguyen từ 2026-09-09). Tạo từ tip main `c561150` + commit docs `421ca79` + dọn config A1/B1; origin/nguyen đã đón đủ 5 commit mới nhất của main (install-guide, CD release...). Không đụng `main`, `khoa`, `anh`/`vy`.
- Fix ESP-NOW (race WiFi vs channel, modem-sleep) đã phân tích nhưng ĐÃ REVERT theo yêu cầu — step 7 sẽ nghiệm thu lại.
- CoreIoT: sensor-node Inactive từ 14:59:36 (thiếu MQTT telemetry — cần 3 xác nhận: `[NET] MQTT connected`, board đang flash env nào, telemetry d1..d6 latest).
- Buzzer: GPIO11 không thể xuất 5V → cần transistor + rail 5V (chưa có câu trả lời active/passive).
- LaTeX: pdflatex TeX Live 2023 tại /home/binhnguyen/.local/bin; report/Makefile `make` (3 lần) + `make verify` grep 4 keyword.