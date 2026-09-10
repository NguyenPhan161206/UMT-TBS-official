# THRESHOLD_30CM_BOUNDARY_LOG

Ngày: 2026-09-11 · Nhánh: nguyen · Loại: fix (P1 từ review G1)

## Mục tiêu
Đồng bộ semantic tại **boundary 30.0cm** giữa firmware C, rule-chain cloud và python
mirror (R3/R11). Trước đó có lệch: firmware dùng `distance < 30` (30cm -> CAUTION),
trong khi `firmware/shared/thresholds.h` (nguồn duy nhất R3) ghi rõ
`SENSOR_ZONE_DANGER = 2 /* x <= DANGER_CM */`, rule-chain cloud `dist <= 30.0`
và python mirror `distance <= DANGER_CM` đều cho 30cm -> DANGER.
Hệ quả trước fix: telemetry đúng 30cm qua cloud ra DANGER nhưng màn hình firmware tự
phân loại CAUTION (legend "< 30cm : Danger"); buzzer sensor-node cũng im tại 30cm.

## Quyết định
Chọn hướng **an toàn**: 30cm = DANGER (khớp contract thresholds.h, cloud, python,
mirror A2). Sửa firmware C + UI label + buzzer về `<=`, giữ nguyên cloud/python.

## File đã sửa (5)
| File | Thay đổi |
|---|---|
| `firmware/waveshare-screen/components/hazard_core/hazard_core.c` | `distance_cm < SENSOR_DANGER_CM` → `<=` (hazard_classify: 30 -> DANGER) |
| `firmware/waveshare-screen/components/hazard_core/include/hazard_core.h` | Cập nhật comment contract `x <= DANGER_CM -> DANGER` + ghi chú mirror |
| `firmware/waveshare-screen/components/ui_dashboard/ui_dashboard_layout.c` | Legend `"< %dcm : Danger"` → `"<= %dcm : Danger"` |
| `firmware/sensor-node/src/buzzer.cpp` | `nearestCm < SENSOR_DANGER_CM` → `<=` (buzzer liên tục từ 30cm) |
| `firmware/waveshare-screen/host_sim/tests/test_hazard_core.c` | CHECK boundary 30cm: CAUTION → DANGER |

Không đổi: `tools/test_mqtt_coreiot.py` (đã `<=`), rule-chain cloud (đã `<= 30.0`),
`tools/guard/test_guard.py` (đã assert 30.0 = DANGER).

## Kết quả kiểm thử
- Grep: không còn `< SENSOR_DANGER_CM` trong `firmware/` (0 match).
- `cmake --build /tmp/host_sim` (via venv) → SUCCESS (hazard_core_tests + umt_dash_sim).
- `/tmp/host_sim/hazard_core_tests` → **25 checks PASSED** (rc=0).
- `/home/binhnguyen/.venv-pio/bin/ctest --output-on-failure` (trong /tmp/host_sim)
  → **2/2 PASSED** (hazard_core_tests + umt_dash_sim_render, 2.23s).
- `pytest tools/guard/test_guard.py -q` (venv python) → **29 passed**.
- `scan_secrets.py` → OK · `arch_guard.py` → OK · `check_rulechain_thresholds.py` → OK.
- `pio run -e yolo_uno_coreiot` (full path venv) → SUCCESS (5.44s; buzzer.cpp compile OK).

## Hướng dẫn vận hành/demo
- Màn hình waveshare: legend zone Danger hiển thị "<= 30cm : Danger"; tại 30cm hiển thị
  OVERALL DANGER (trước fix là CAUTION).
- Buzzer sensor-node: kêu liên tục từ ngưỡng 30cm (bao gồm chính xác 30cm).
- Không cần thay đổi cloud rule-chain hay script test — đã cùng semantic.

## Ghi chú
- Việc dọn macro buzzer cũ (`BUZZER_WARNING_DISTANCE_CM`/`BUZZER_DANGER_DISTANCE_CM`)
  vẫn nằm trong roadmap next-branch step T3.4 (không đụng trong fix này).
- Commit đề xuất: `fix: đồng bộ boundary DANGER_CM (x <= 30) giữa firmware C, cloud rule-chain, python mirror`.