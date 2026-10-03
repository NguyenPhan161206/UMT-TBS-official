# truck-diagram-review-fixes — Orchestration State
Roadmap: docs/roadmaps/truck-diagram-review-fixes.roadmap.json
Nguồn: review của truck-diagram-markers (docs/roadmaps/truck-diagram-markers.state.md); người dùng duyệt làm cả 3 mục (a)(b)(c).
Updated: 2026-10-03
Thực thi: trực tiếp bởi orchestrator theo yêu cầu người dùng (không qua Worker riêng).

| Step | Title | Status | Verified by | Notes |
|------|-------|--------|-------------|-------|
| 1 | Callback Mute thay cho gọi thẳng espnow_receiver | DONE | grep espnow_receiver trong ui_dashboard = 0; arch_guard OK; ui_dashboard.c 399 dòng | (a) — còn đúng 1 dòng dư địa R7 |
| 2 | Nối callback ở main.c, xoá stub/code chết host_sim | DONE | `pio run -e yolo_uno` SUCCESS (flash 31,8%); ctest 3/3; host_sim/main.c == HEAD; harness tạm: click -> cb(true), cb(false), NULL không crash | (a) |
| 3 | CI chạy vehicle_layout_tests; CMake if(WIN32) | DONE | ci.yml YAML parse OK + 1 lần chạy vehicle_layout_tests; reconfigure + build + ctest 3/3 (Windows); link SDL2main vẫn trước SDL2 | (c) — nhánh Linux (`else()`) chưa chạy cục bộ (không có WSL), CI là lần chạy đầu |
| 4 | Sửa log, CHECKLIST, ledger cho đúng sự thật | DONE | script đếm lại bảng Tổng hợp nhanh = KHỚP (tổng 28 / ✅12 / 🟡7 / ❌9); grep các cụm sai trong log = 0; mọi số dòng trong log khớp `wc -l`; scan_secrets OK | (b) — G2/G5 của bảng cũ đã lệch từ trước nên được sửa theo dòng chi tiết |

## Contracts established
(điền sau mỗi step)

## Deviations from plan
(điền khi có)
