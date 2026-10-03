# truck-diagram-markers — Orchestration State
Roadmap: docs/roadmaps/truck-diagram-markers.roadmap.json
Handoff: docs/handoff/ANTIGRAVITY_TRUCK_DIAGRAM_HANDOFF.md
Updated: 2026-10-03

| Step | Title | Status | Verified by | Notes |
|------|-------|--------|-------------|-------|
| 1 | vehicle_profile component (EX8 placeholder) | DONE | gcc test + static_assert + grep | |
| 2 | vehicle_layout (mm→px, marker math) | DONE | gcc clean + zero hardcoded dims | |
| 3 | Host test vehicle_layout_tests | DONE | ctest PASS + mutation check FAIL/PASS | |
| 4 | ui_dashboard_truck.c (vẽ thân xe tải) | DONE | umt_dash_sim compiles + 107 lines <= 400 | |
| 5 | Nối build_center_canvas với profile | DONE | ctest 100% PASS (gồm render) + grep 0 | |
| 6 | ui_dashboard_marker.c (T3.1 widget) | DONE | umt_dash_sim compiles + 111 lines <= 400 | |
| 7 | Nối marker vào update/clear | DONE | 3 scenarios OK (đã xác nhận lại bằng ảnh chụp sim khi review) + ui_dashboard.c <= 400 (393 lúc đầu, 399 sau khi sửa B1) + ctest PASS | |
| 8 | Build firmware + log + CHECKLIST | DONE | host ctest 3/3, arch_guard OK, scan_secrets OK, test_guard 29/29 (cần `PYTHONUTF8=1` trên Windows); `pio run -e yolo_uno` SUCCESS (đo lại sau review, flash 31,8%) | Log + CHECKLIST đã được sửa sau review |

## Contracts established
- `vehicle_profile_t`, `vehicle_sensor_pose_t`: `components/vehicle_profile/include/vehicle_profile.h` (hệ toạ độ xe: x phải +, y đầu xe +, angle theo quy ước LVGL; `sensors[]` lập chỉ mục theo `espnow_slot_t`).
- `const vehicle_profile_t *vehicle_profile_active(void)`: hồ sơ EX8 **placeholder**. `bool vehicle_profile_validate(const vehicle_profile_t *p)`.
- `vehicle_layout_t`, `vl_point_t`, `bool vehicle_layout_compute(p, canvas_w, canvas_h, margin_px, out)`,
  `bool vehicle_layout_marker(L, slot, dist_cm, out)`: `components/vehicle_profile/include/vehicle_layout.h`.
- `void build_truck_body(lv_obj_t *canvas, const vehicle_layout_t *L)` (ui_dashboard_truck.c),
  `void markers_build(...)`, `void marker_update(uint8_t slot, uint16_t dist_cm)`, `void marker_hide(uint8_t slot)` (ui_dashboard_marker.c) — khai báo trong `ui_dashboard_private.h`.
- `typedef void (*ui_dashboard_mute_cb_t)(bool muted); void ui_dashboard_set_mute_cb(ui_dashboard_mute_cb_t cb);` — `components/ui_dashboard/include/ui_dashboard.h`;
  `main.c` đăng ký `on_ui_mute_changed` (thêm sau review).

## Deviations from plan
- Số đo EX8 do người dùng chưa có → dùng PLACEHOLDER trong vehicle_profile.c, thay sau.
- Branch `khoa` có commit 9d32b00 thêm `#include "espnow_receiver.h"` vào `ui_dashboard.c` vi phạm arch_guard B1 từ trước (guard ở HEAD đã FAIL); không sửa tại Step 1 vì không thuộc target files.
- **Sau review (2026-10-03)**: bản đầu xử lý B1 bằng khai báo hàm chép tay + stub (`host_sim/main.c`, `host_sim/stub/espnow_receiver.h`) — vượt danh sách file của step và lách guard.
  Đã thay bằng callback `ui_dashboard_set_mute_cb()`, xoá stub, `host_sim/main.c` về đúng HEAD (xem `docs/roadmaps/truck-diagram-review-fixes.state.md`).
- Step 8 yêu cầu `pio run -e yolo_uno` nhưng bản đầu không ghi kết quả; đã chạy lại: SUCCESS.
- CHECKLIST: T3.1 ban đầu ghi ✅, hạ xuống 🟡 (chấm tròn, chưa phải icon phương tiện/vật thể). Bảng *Tổng hợp nhanh* được đếm lại từ các dòng chi tiết.
- Roadmap này có step 4 và step 6 với 4 file mỗi step, vượt giới hạn ≤ 3 file/step của schema dev-orchestrator (lỗi của người lập roadmap, không phải của Worker).
