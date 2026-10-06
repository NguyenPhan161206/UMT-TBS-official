# main-features — Orchestration State
Roadmap: docs/roadmaps/main-features.roadmap.json
Updated: 2026-10-05
Bối cảnh: người dùng chưa có xe tải (không đo vật lý được) và không có board cắm → chỉ làm phần kiểm chứng được trên máy (host_sim, ctest, pio build).
Người dùng chọn: T2.3 xe cắt ngang, T5.2 Calibrate + T3.1 biểu tượng, T5.3/T5.x dọn kỹ thuật. Không commit.

| Step | Title | Status | Verified by | Notes |
|------|-------|--------|-------------|-------|
| 1 | hazard_core: crossing có trạng thái | DONE | hazard_core_tests (50 lúc này); arch_guard OK; 4 đột biến (bỏ ok, bỏ cửa sổ, bỏ giữ, so sánh không chống tràn) đều FAIL | test tràn số ban đầu không bắt được đột biến → đã sửa mốc thời gian |
| 2 | Test crossing trên dữ liệu kịch bản | DONE | hazard_core_tests 70 → 72 | crossing/crossing_right báo ở 100 & 500 ms; 8 kịch bản khác không báo |
| 3 | ui_dashboard dùng crossing có trạng thái + timer đánh giá lại | DONE | ctest 10/10 | timer 250 ms chỉ đánh giá lại khi đang giữ |
| 4 | Bỏ parse crossing_hazard ở main.c; sim đánh giá nguy hiểm | DONE | ctest; ảnh sim: banner hiện ở 450 ms, tự tắt ở 4500 ms | sim giờ hiện đúng OVERALL (trước luôn "SAFE [SENSOR FAULT]") |
| 5 | Xoá API set_hazard_warning + s_forced_crossing_warning | DONE | grep = 0; ctest | |
| 6 | Sim --expect-text + ctest xe cắt ngang | DONE | 2 ctest mới; bỏ timer / HOLD=0 → FAIL | |
| 7 | T5.2 Calibrate mở SETUP | DONE | ctest `umt_dash_sim_calibrate_button`; bỏ callback → FAIL | |
| 8 | T3.1 marker hiện khoảng cách | DONE | ảnh sim; stress: heap 47% trống, delta 8 B | nhãn dùng cùng số với thanh trái (vùng chết 3 cm) |
| 9 | T5.3 sửa comment sai | DONE | ctest | espnow_protocol.h (shared), main.c, sensor_edit.c |
| 10 | T5.x xoá code chết nhấp nháy | DONE | grep blink = 0; ctest 13/13 | |
| 11 | T5.x đặt tên hằng UI | DONE | grep literal = 0; ctest 13/13 | UI_HEADER_H, UI_CONTENT_H, UI_CANVAS_*, UI_SYS_INFO_REFRESH_MS |
| 12 | Build firmware + guard + log | DONE | pio SUCCESS (flash 1.317.921 B, RAM 172.828 B); ctest 13/13; arch/scan/pytest 30/rulechain OK | log `docs/logs/WAVESHARE_SCREEN_MAIN_FEATURES_LOG.md`; chưa nạp board |
| 13 | CHECKLIST + tài liệu kiến trúc | DONE | CHECKLIST 16/6/6; ARCHITECTURE_G1_TESTING + HARDCODED_CONFIG_NOTES cập nhật | đính chính luôn khẳng định sai trong log hồ sơ xe |

## Lỗi tìm thấy khi lập kế hoạch (lý do của step 1)
1. **Báo xe cắt ngang giả khi mất kết nối**: slot bị xoá có khoảng cách 0. FRONT mất → 0 < 150 = "gần"; slot bên rớt/nối lại → |0 − cũ| ≥ 40.
   Watchdog ESP-NOW xoá mọi slot rồi gọi `ui_dashboard_evaluate_hazard()` → mỗi lần mất link đều có thể bật cảnh báo.
2. **Không nhạy ở 10 khung/s**: so sánh 2 khung liên tiếp, mà `ESPNOW_SEND_INTERVAL_MS` = 100 → vật cắt ngang 1 m/s chỉ đổi 10 cm/khung, không bao giờ ≥ 40.
3. **Cảnh báo chỉ sáng 1 khung** rồi tắt (không có thời gian giữ).
4. **Đường cloud chết**: `main.c` đọc `crossing_hazard` nhưng rule-chain không xuất trường này.

## Quyết định
- Xe cắt ngang tính TẠI MÀN HÌNH từ khoảng cách (cả ESP-NOW và MQTT đều đi qua `evaluate_hazard`); bỏ đường ép từ cloud (không triển khai rule-chain được và cũng không cần).
- `hazard_eval_crossing` nhận state do người gọi giữ (B2 vẫn đúng), không thêm hàm public (B7 tối đa 4).
- Hằng mới ở `hazard_core.h` (B5): `CROSSING_WINDOW_MS` 500, `CROSSING_HOLD_MS` 3000 — giá trị ước lượng, chỉnh khi có dữ liệu thật.
- Calibrate = mở trang SETUP (chỉnh vị trí cảm biến), giữ nhãn "Calibrate".

## Contracts established
- `hazard_crossing_result_t hazard_eval_crossing(hazard_crossing_state_t *st, const uint16_t *cur_cm, const bool *ok, size_t n, uint32_t now_ms)`
  — `components/hazard_core/include/hazard_core.h`; state zero-init; hằng `CROSSING_WINDOW_MS` 500, `CROSSING_HOLD_MS` 3000 (+ `CROSSING_DELTA_CM` 40, `CROSSING_FRONT_THRESHOLD_CM` 150).
- ĐÃ XOÁ: `ui_dashboard_set_hazard_warning(bool)` (public), `s_forced_crossing_warning`, `s_prev_distance_cm`; `main.c` không đọc `crossing_hazard` nữa.
- host_sim: `--expect-text TEXT@ms`, `--expect-no-text TEXT@ms` (chỉ label đang hiển thị), `sim_script_add(sim_item_kind_t, const char *)`.
- `ui_dashboard_theme.h`: `UI_HEADER_H`, `UI_CONTENT_H`, `UI_CANVAS_W/H/MARGIN`, `UI_SYS_INFO_REFRESH_MS`.

## Deviations from plan
1. **Thêm (ngoài plan, trong step 8 → hazard_core)**: crossing chỉ xét 2 góc TRƯỚC (LEFT_FRONT, RIGHT_FRONT). Ảnh sim `boxed_in` báo "xe cắt ngang - S6 (R-Rear)":
   heuristic cũ quét cả slot bên sau, nên xe chạy dọc hông sau khi đầu xe gần vật cũng bị báo "cắt ngang phía trước". Test đổi: RR/LR đổi nhanh → không báo.
2. Step 8 sửa thêm `ui_dashboard.c`: nhãn trên sơ đồ dùng số đang hiện ở thanh trái (vùng chết 3 cm) — trước đó cùng cảm biến hiện "24 cm" và "22".
3. Test giữ cảnh báo (step 1) viết lại sau lần chạy đầu: giả định khung thưa sai với thiết kế cửa sổ tham chiếu (kích hoạt lại khi mốc cũ hơn cửa sổ). Code không đổi.
4. `sed -i` đổi CRLF → LF ở `ui_dashboard_layout.c` (và script Python ở phiên trước đổi vài file khác); git (autocrlf=true) chỉ thấy thay đổi thật, không ảnh hưởng.
