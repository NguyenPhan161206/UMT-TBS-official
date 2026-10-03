# more-scenarios — Orchestration State
Roadmap: docs/roadmaps/more-scenarios.roadmap.json
Nguồn: yêu cầu người dùng "muốn có thêm nhiều kịch bản nữa" (2026-10-03). Thực thi trực tiếp bởi orchestrator.
Updated: 2026-10-03

| Step | Title | Status | Verified by | Notes |
|------|-------|--------|-------------|-------|
| 1 | Test kịch bản mới (đỏ trước) | DONE | Trước khi thêm dữ liệu: đúng 1 test đỏ (`test_new_scenarios_semantics`, KeyError), 29 test khác pass | đổi tên test "4 timeline" → `test_scenarios_named_timelines` |
| 2 | 11 kịch bản vào tools/scenarios.py | DONE | pytest 30 passed; `python tools/scenarios.py` liệt kê 15; 4 kịch bản gốc == HEAD | |
| 3 | Sim --list, help động, hướng dẫn | DONE | build sim OK; `--list` in 15 dòng exit 0; ctest 3/3 | |
| 4 | Chạy đủ 15 trên sim + log + tài liệu | DONE | 15/15 replay N/N mốc exit 0; 15/15 MQTT dry-run exit 0; ảnh chụp 4 kịch bản đúng slot/màu; guard OK | CI giữ nguyên (chỉ approach/slam) |

## Danh sách kịch bản mới (thứ tự slot d1..d6 = FRONT, REAR, LEFT_FRONT, LEFT_REAR, RIGHT_FRONT, RIGHT_REAR)
overtake_right, overtake_left, reverse_wall, reverse_pedestrian, pedestrian_front, crossing_right,
narrow_lane, boxed_in, threshold_flap, fast_pass, stop_and_go.

## Contracts established
(điền sau mỗi step)

## Deviations from plan
(điền khi có)
