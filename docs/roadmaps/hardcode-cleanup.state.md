# hardcode-cleanup — Orchestration State
Roadmap: docs/roadmaps/hardcode-cleanup.roadmap.json
State: ALL STEPS DONE (2026-09-10).

| Step | Title | Status | Verified by | Notes |
|------|-------|--------|-------------|-------|
| 1 | R2: SENSOR_BEAM_FOV_DEG single-source + derive half_fov | DONE | grep=2 (1 define+1 usage); pio waveshare SUCCESS; host_sim ctest 2/2 | comment không còn tên symbol |
| 2 | Name sensor-node timing literals (task_cfg.h + main.cpp) | DONE | pio sensor-node 2 env SUCCESS | 4 macro trong task_cfg.h |
| 3 | Wire task_cfg.h into buzzer.cpp + shared_state.cpp | DONE | grep==0; pytest 29 passed; 4 guard OK | + HARDCODED_CONFIG_NOTES cập nhật |

## Contracts established
- `firmware/shared/thresholds.h` = DUY NHẤT nơi định nghĩa `SENSOR_BEAM_FOV_DEG` (R2/R3).
- `firmware/sensor-node/include/task_cfg.h` = sensor-node local timing macros
  (`SENSOR_SETTLE_DELAY_MS 500`, `SERIAL_SETUP_DELAY_MS 200`,
  `TASK_POLL_INTERVAL_MS 20`, `MUTEX_TIMEOUT_MS 10`) — KHÔNG đưa vào shared/.

## Deviations from plan
- (none)