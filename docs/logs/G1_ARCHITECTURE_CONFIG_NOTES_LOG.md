# G1 Architecture Update + Config Notes LOG

> Updated: 2026-09-09
> Component: docs (kiến trúc G1 + config cứng checklist)

## Mục tiêu
1. Cập nhật kiến trúc Giai đoạn 1 vào `docs/ARCHITECTURE_G1_TESTING.md` theo 2 quyết
   định đã chốt: (1) decouple `hazard_core` khỏi `sensor_reading_t` + bỏ
   `sensor_model_classify`, (2) `tools/guard/arch_guard.py` sinh kèm step 1.
2. Ghi chú tất cả chỗ config cứng / khó bảo trì vào 1 file md riêng + đưa vào checklist
   roadmap.

## File đã sửa / tạo
- `docs/ARCHITECTURE_G1_TESTING.md` — cập nhật API hazard_core (primitive arrays, bỏ
  wrapper), thêm bảng quy tắc B1–B7, phương thức guard arch_guard.py, thứ tự thực thi
  đã đảo, test mini assert-runner (không Unity).
- `docs/HARDCODED_CONFIG_NOTES.md` — **mới**: rà soát config cứng toàn firmware:
  - **A** (drift thật): legend UI string `"> 100cm"/"30-100cm"/"< 30cm"` tại
    `ui_dashboard_layout.c:299-301` + python mirror `test_mqtt_coreiot.py:41-42`.
  - **B**: `offset_deg` dead-field (`sensor_model.c:13-20,38` + `sensor_model.h:34`,
    không nơi nào đọc) + hình học cảm biến 2 nơi (2 convention).
  - **C**: timing/tuning literal ẩn (pdMS_TO_TICKS(20)/(10), delay(200), stack 4096/2048,
    priority, payload[256]) trong `main.cpp`, `buzzer.cpp`, `shared_state.cpp`;
    `ui_dashboard.c:129` (2000ms), `ui_dashboard_layout.c:35-36` (400 blink).
  - **D** (loại trừ): số px layout LVGL, hằng vật lý, test-asset, R4 static_assert.
  - Kèm bảng "blast radius" (sửa 1 chỗ → đụng 1 chỗ).
- `docs/roadmaps/next-branch.roadmap.json` — total_steps 15→16; thêm step 16
  "T5.x Dọn config cứng (A/B/C)" prereq [3,9], target_files + DoD + verify.
- `docs/roadmaps/next-branch.state.md` — thêm row 16, contract mới, deviation
  2026-09-09 ghi nguồn checklist.

## Kết quả kiểm thử
- `python3 -c json.load(next-branch.roadmap.json)` → OK (16/16 steps).
- Chưa chạy CI / guard (không thay đổi code firmware trong task này — chỉ docs + roadmap).
- Scan secret: không thêm credential nào.

## Hướng dẫn vận hành / demo
- Đọc kiến trúc G1: `docs/ARCHITECTURE_G1_TESTING.md`.
- Checklist dọn config cứng khi code chạm từng file: `docs/HARDCODED_CONFIG_NOTES.md`
  (step 16 roadmap — những đám A/B/C được xử lý đúng trong step sở hữu từng file).