# HARDCODE_CLEANUP_LOG — R2 FOV single-source + sensor-node timing macros

Ngày: 2026-09-10
Nhánh: `nguyen` (chưa merge main)
Roadmap: `docs/roadmaps/hardcode-cleanup.roadmap.json` (3 steps, tất cả DONE)

## Mục tiêu
Khắc phục 2 hạn chế hardcode phát hiện trong khảo sát kiến trúc:
1. **P0 / vi phạm R2**: `SENSOR_BEAM_FOV_DEG` định nghĩa 2 nơi (shared + waveshare)
   và `make_arc()` hardcode `half_fov = 37` tách rời nguồn FOV → drift khi đổi ngưỡng.
2. **P1 / mục C**: literal timing sensor-node không tên (poll 20ms x3, mutex 10ms x3,
   settle 500ms, serial 200ms) rải trong main.cpp/buzzer.cpp/shared_state.cpp.

## File đã sửa
- `firmware/waveshare-screen/components/sensor_model/include/sensor_model.h` —
  xoá `#define SENSOR_BEAM_FOV_DEG 75` (R2: nguồn duy nhất = `firmware/shared/thresholds.h`).
- `firmware/waveshare-screen/components/ui_dashboard/ui_dashboard_layout.c` —
  `half_fov` = `SENSOR_BEAM_FOV_DEG / 2` (75/2 = 37, giá trị không đổi, pixel giữ nguyên).
- `firmware/sensor-node/include/task_cfg.h` (mới) — macro per-firmware local:
  `SENSOR_SETTLE_DELAY_MS 500`, `SERIAL_SETUP_DELAY_MS 200`,
  `TASK_POLL_INTERVAL_MS 20`, `MUTEX_TIMEOUT_MS 10` (KHÔNG đưa vào shared/, R2).
- `firmware/sensor-node/src/main.cpp` — 4 literal → macro (settle, 2x poll, serial).
- `firmware/sensor-node/src/buzzer.cpp` — poll 20ms → `TASK_POLL_INTERVAL_MS`.
- `firmware/sensor-node/src/shared_state.cpp` — 3x mutex 10ms → `MUTEX_TIMEOUT_MS`.
- `docs/HARDCODED_CONFIG_NOTES.md` — mục B (FOV) và mục C (sensor-node) ghi ĐÃ XỬ LÝ.
- `docs/roadmaps/hardcode-cleanup.{json,state}` (mới) — roadmap + ledger step DONE.

## Kết quả kiểm thử (2026-09-10)
- R2: `grep -rn SENSOR_BEAM_FOV_DEG firmware/` (trừ .pio) = **2** (1 `#define` + 1 usage).
- Build: `pio run -e yolo_uno` (waveshare) SUCCESS; `yolo_uno` + `yolo_uno_coreiot`
  (sensor-node) SUCCESS cả 2.
- host_sim: `cmake -S host_sim -B /tmp/opencode/host_sim_final && cmake --build ...`
  SUCCESS; `ctest` **2/2 passed**.
- Guard: literal timing sensor-node còn sót = **0**; `pytest tools/guard/test_guard.py`
  **29 passed**; `arch_guard.py` OK; `scan_secrets.py` OK; `check_rulechain_thresholds.py` OK.

## Hướng dẫn vận hành / demo
Không đổi hành vi runtime (giá trị giữ nguyên, chỉ đặt tên + gộp nguồn). Verify nếu cần:
```bash
export PATH="/home/binhnguyen/.venv-pio/bin:$PATH"
grep -rn "SENSOR_BEAM_FOV_DEG" firmware/ | grep -v .pio        # mong đợi 2 dòng
grep -rn "pdMS_TO_TICKS([0-9]" firmware/sensor-node/src       # mong đợi rỗng
cd firmware/waveshare-screen && pio run -e yolo_uno
cd firmware/sensor-node && pio run -e yolo_uno && pio run -e yolo_uno_coreiot
cmake -S firmware/waveshare-screen/host_sim -B /tmp/opencode/host_sim_final && \
  cmake --build /tmp/opencode/host_sim_final && (cd /tmp/opencode/host_sim_final && ctest)
python3 -m pytest tools/guard/test_guard.py -q
python3 tools/guard/arch_guard.py && python3 tools/guard/scan_secrets.py
```
Đổi FOV trong tương lai: sửa 1 dòng `thresholds.h`, arc dashboard tự cập nhật (nhờ macro).