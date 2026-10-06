# vehicle-profile-override — Orchestration State (T4.1c)
Roadmap: docs/roadmaps/vehicle-profile-override.roadmap.json
Tiền đề: docs/roadmaps/vehicle-profile.state.md phải có step 8 (vehicle_settings init/select) và step 15 (trang SETUP + rebuild) ở trạng thái DONE.
Updated: 2026-10-05

| Step | Title | Status | Verified by | Notes |
|------|-------|--------|-------------|-------|
| 1 | vehicle_settings: API ghi đè từng cảm biến + test | DONE | vehicle_settings_tests 150/150; 3 đột biến (select giữ override, clear không khôi phục, bỏ validate) đều FAIL | API: `set_override`, `clear_override`, `clear_all_overrides`, `override_mask` |
| 2 | Widget chỉnh tay cảm biến | DONE | pio SUCCESS; ảnh sim (`--snapshot`) | file `ui_dashboard_sensor_edit.c`; hàng cao cố định, bỏ LV_SIZE_CONTENT (treo LVGL) |
| 3 | Gắn widget vào trang SETUP + nghiệm thu NVS | DONE (sim) / CHƯA kiểm chứng trên board | ctest `umt_dash_sim_setup_ui` pass; ảnh sim: FRONT lùi 200 mm → dịch 9 px, `FRONT*`, Reset trả về đúng từng pixel; vị trí ngoài khung xe bị từ chối | không có board nên chưa thử reset giữ ghi đè |
| 4 | Log T4.1c + CHECKLIST + kiểm tra toàn bộ | DONE | log mục 4.2/6; CHECKLIST T4.1c 🟡; guard OK | |

## Contracts established
(Chưa có. Hợp đồng dự kiến, chốt khi step 1 DONE.)
- `bool vehicle_settings_set_override(espnow_slot_t slot, const vehicle_sensor_pose_t *pose)` — validate hồ sơ sau ghi đè, sai thì không đổi gì.
- `bool vehicle_settings_clear_override(espnow_slot_t slot)`, `bool vehicle_settings_clear_all_overrides(void)`, `uint8_t vehicle_settings_override_mask(void)`.
- `lv_obj_t *build_sensor_editor(lv_obj_t *parent); void sensor_editor_refresh(void)` (ui_dashboard_private.h).

## Quyết định đã chốt
- Chỉnh bằng nút −/+ (x, y bước 50 mm; góc bước 5°, quay vòng 0..359), bản nháp chỉ áp dụng khi bấm Apply; không có bàn phím/nhập số trực tiếp (màn cảm ứng, thêm bàn phím vượt phạm vi).
- Đổi hồ sơ gốc xoá override (xem ledger vehicle-profile, quyết định 7).
- Override lưu cùng blob NVS của hồ sơ (schema đã có chỗ từ version 1).

## Ngoài phạm vi
- **T4.1d** (người dùng tự tạo hồ sơ mới): để dành giai đoạn sau theo yêu cầu.

## Deviations from plan
(Chưa có.)
