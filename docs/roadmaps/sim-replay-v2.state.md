# sim-replay-v2 — Orchestration State
Roadmap: docs/roadmaps/sim-replay-v2.roadmap.json
Nguồn: người dùng replay file của tools/recorder trên sim -> "skip invalid replay row", nạp 0 dòng; duyệt sửa ("ok") 2026-10-03.
Thực thi trực tiếp bởi orchestrator.
Updated: 2026-10-03

| Step | Title | Status | Verified by | Notes |
|------|-------|--------|-------------|-------|
| 1 | Fixture + test ctest (đỏ trước) | DONE | Trước khi sửa sim: đúng 2 test recorder đỏ ("skip invalid"), v2 + 3 test cũ xanh | |
| 2 | Sim đọc 2 định dạng, valid, nhịp thật | DONE | ctest 6/6; sample 30/30 dòng; nhịp 1x/4x/--interval đúng; khong_day valid=0 -> `--`; rỗng/rác exit 4, không tồn tại exit 2; 15/15 kịch bản N/N; -Wall -Wextra sạch | main.c dùng patch script (heredoc dài làm shell lỗi) |
| 3 | Hướng dẫn + CI | DONE | YAML hợp lệ, ci.yml có 2 lệnh replay; hết câu "UDP Socket vào bộ giả lập" | CI Linux không chạy thử được cục bộ |
| 4 | Chạy toàn bộ recordings + log | DONE | 6/6 file có dữ liệu nạp đủ, 0 bỏ qua; test_mqtt (rỗng) exit 4; ảnh chụp nodata + multi; guard OK | UDP vẫn chưa làm |

## Contracts established
- `umt_dash_sim --replay <file>`: nhận payload V2 (`d1..d6`) hoặc file recorder (`distances[]`, `valid[]`, `elapsed_ms`); `--speed <x>`; `--interval <ms>` ép nhịp cố định; exit 4 nếu 0 dòng hợp lệ, exit 2 nếu không mở được file.
- Fixture: `firmware/waveshare-screen/host_sim/tests/fixtures/replay_v2.jsonl` (3 dòng), `replay_recorder.jsonl` (5 dòng, có slot valid=0).

## Deviations from plan
- Sim chưa có bộ nhận UDP cổng 9090 (hướng dẫn cũ hứa có): không nằm trong phạm vi, đã ghi rõ trong hướng dẫn thay vì để câu sai.
- Một lệnh vá `main.c` bằng heredoc dài làm shell báo lỗi cú pháp (chưa sửa gì); đổi sang ghi script vá ra file rồi chạy.
