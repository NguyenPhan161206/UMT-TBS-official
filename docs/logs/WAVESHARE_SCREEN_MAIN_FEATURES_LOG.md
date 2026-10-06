# WAVESHARE_SCREEN_MAIN_FEATURES_LOG — T2.3 xe cắt ngang, T5.2 Calibrate, T3.1 biểu tượng vật thể, T5.3/T5.x dọn kỹ thuật

Ngày: 2026-10-05 · Nhánh: `khoa` (working tree, **chưa commit**) · Roadmap: `docs/roadmaps/main-features.roadmap.json` · Ledger: `docs/roadmaps/main-features.state.md`
Bối cảnh: chưa có xe tải và không có board cắm → mọi kiểm chứng là trên máy (unit test, host_sim, pio build). **Chưa quan sát trên phần cứng.**

## 1. Mục tiêu
- **T2.3** (lỗi P0 cuối): cảnh báo xe cắt ngang chạy đúng và kiểm thử được đầu-cuối; dọn đường ép từ cloud chưa bao giờ hoạt động.
- **T5.2**: nút Calibrate có tác dụng. **T3.1**: biểu tượng vật thể tại vị trí phát hiện. **T5.3/T5.x**: comment sai, code chết, hằng cứng.

## 2. Lỗi tìm thấy trong T2.3 (đã sửa)
1. **Báo xe cắt ngang giả khi mất kết nối**: slot bị xoá có khoảng cách 0 → FRONT mất = "gần" (0 < 150), slot bên rớt/nối lại = "đổi ≥ 40 cm". Watchdog ESP-NOW xoá mọi slot rồi đánh giá ngay.
2. **Không nhạy ở 10 khung/s**: so 2 khung liên tiếp, mà ESP-NOW gửi mỗi 100 ms → vật cắt ngang 1 m/s chỉ đổi 10 cm/khung, không bao giờ báo.
3. **Cảnh báo chỉ sáng ~1 khung** rồi tắt.
4. **Báo nhầm từ slot bên SAU** (xe chạy dọc hông khi đầu xe gần vật) — thấy trong ảnh sim `boxed_in`.
5. **Đường cloud chết**: `main.c` đọc `crossing_hazard` nhưng rule-chain không xuất trường này.

## 3. Thay đổi
| File | Thay đổi |
|---|---|
| `components/hazard_core/include/hazard_core.h`, `hazard_core.c` | `hazard_eval_crossing(state, cur, ok, n, now_ms)`: state do người gọi giữ (B2), bỏ slot không hợp lệ, mốc tham chiếu `CROSSING_WINDOW_MS` 500, giữ `CROSSING_HOLD_MS` 3000, chỉ 2 góc trước; vẫn 4 hàm public (B7). Ngưỡng là **ước lượng**. |
| `components/ui_dashboard/ui_dashboard.c` | dùng state + `lv_tick_get()`; timer 250 ms đánh giá lại khi đang giữ (banner tự tắt dù không còn khung); xoá `s_prev_distance_cm`, `s_forced_crossing_warning`, API `ui_dashboard_set_hazard_warning`; nhãn trên sơ đồ dùng cùng số với thanh trái; hằng `UI_*` |
| `components/ui_dashboard/include/ui_dashboard.h`, `ui_dashboard_private.h` | bỏ API/extern cũ; bỏ trường anim chết |
| `components/ui_dashboard/ui_dashboard_layout.c` | Calibrate → trang SETUP; xoá code chết nhấp nháy; hằng `UI_HEADER_H`, `UI_CANVAS_*` |
| `components/ui_dashboard/ui_dashboard_marker.c` | chấm tròn → nhãn bo tròn hiện khoảng cách (cm), màu zone |
| `components/ui_dashboard/ui_dashboard_theme.h` | `UI_HEADER_H`, `UI_CONTENT_H`, `UI_CANVAS_W/H/MARGIN`, `UI_SYS_INFO_REFRESH_MS` |
| `components/ui_dashboard/ui_dashboard_sensor_edit.c` | sửa comment đổ lỗi sai cho `LV_SIZE_CONTENT` |
| `src/main.c` | bỏ parse `crossing_hazard`; comment chu kỳ gửi 100 ms |
| `firmware/shared/espnow_protocol.h` | comment thứ tự slot (không trùng `SENSOR_PINS`; S1..S6 = slot 0..5) — chỉ comment |
| `host_sim/main.c`, `sim_tools.c/.h`, `CMakeLists.txt`, `tests/test_hazard_core.c` | sim đánh giá nguy hiểm sau mỗi mốc như firmware; `--expect-text/--expect-no-text`; test kịch bản; 3 ctest mới |
| `docs/ARCHITECTURE_G1_TESTING.md`, `docs/HARDCODED_CONFIG_NOTES.md`, `docs/CHECKLIST.md` | cập nhật theo code mới; T2.3, T3.1 ✅ (host), T5.2, T5.3 ✅; bảng tổng hợp đếm lại 16/6/6 |

Số dòng (`wc -l`): hazard_core.c 151, hazard_core.h 103, ui_dashboard.c 377, ui_dashboard_layout.c 366, ui_dashboard_marker.c 141, test_hazard_core.c 356, sim_tools.c 360; `host_sim/main.c` 424 (vượt 400 từ phiên trước — xem log hồ sơ xe).

## 4. Kết quả kiểm thử (đo 2026-10-05)
- `hazard_core_tests`: **72 check** (mất link, FRONT stale, slot rớt/nối lại, vật 1 m/s ở 100 ms, khung 500 ms, giữ + hết hạn đúng 3500 ms, tràn uint32, slot sau, và chạy heuristic trên dữ liệu `tools/scenarios.py`: crossing/crossing_right báo ở 100 & 500 ms; normal, approach, fast_pass, stop_and_go, reverse_wall, narrow_lane, overtake_left/right không báo).
- **Đột biến** (mỗi cái làm test FAIL rồi hoàn lại): bỏ kiểm `ok`; bỏ cửa sổ; bỏ giữ; so sánh thời gian không chống tràn (lần đầu test KHÔNG bắt được → sửa test); bỏ timer đánh giá lại; `CROSSING_HOLD_MS`=0; bỏ callback Calibrate.
- `ctest --test-dir build/host_sim`: **13/13** (mới: `umt_dash_sim_crossing_banner`, `umt_dash_sim_normal_no_crossing`, `umt_dash_sim_calibrate_button`).
- Stress 120 lần dựng lại: 138 widget/hồ sơ không trôi, heap LVGL lệch 8 B, còn **47%** trống sau khi dựng UI.
- Guard: `arch_guard` OK, `scan_secrets` OK, `pytest tools/guard/test_guard.py` **30 passed**, `check_rulechain_thresholds` OK.
- Firmware `pio run -e yolo_uno`: **SUCCESS** (162,8 s) — flash **1.317.921 B = 31,9%**, RAM **172.828 B = 52,7%**. Chưa nạp lên board.

## 5. Quyết định
- Xe cắt ngang tính tại màn hình (cả ESP-NOW và MQTT), không qua rule-chain (node JS không giữ được trạng thái giữa bản tin).
- `CROSSING_WINDOW_MS` 500 / `CROSSING_HOLD_MS` 3000 / chỉ 2 góc trước: hợp lý trên kịch bản giả lập; **phải chỉnh lại với dữ liệu thật**.
- Calibrate = mở trang chỉnh cảm biến (giữ nhãn "Calibrate").

## 6. Chưa làm / hạn chế
- Chưa thử trên board/xe: banner xe cắt ngang, nhãn khoảng cách, nút Calibrate trên màn cảm ứng thật.
- Nhãn khoảng cách là chỉ báo tầm trên trục búp (không phân loại phương tiện).
- Hiệu ứng nhấp nháy cung đã xoá vì là code chết; nếu muốn có nhấp nháy ở DANGER thì cần làm mới.

## 7. Chạy / demo
```bash
cmake -S firmware/waveshare-screen/host_sim -B build/host_sim && cmake --build build/host_sim
ctest --test-dir build/host_sim --output-on-failure
build/host_sim/umt_dash_sim --scenario crossing --interval 500 --exit-after 8      # banner xe cắt ngang rồi tự tắt
build/host_sim/umt_dash_sim --scenario approach --interval 1000 --exit-after 10    # nhãn khoảng cách trên sơ đồ
```
Trong cửa sổ sim: bấm "Calibrate" ở thanh trái để mở trang chỉnh cảm biến.
