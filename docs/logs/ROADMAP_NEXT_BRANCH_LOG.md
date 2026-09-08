# ROADMAP NEXT-BRANCH — IMPLEMENTATION LOG

- **Ngày**: 2026-09-08
- **Mục tiêu**: Lập roadmap chi tiết cho nhánh phát triển tiếp theo, xuất dưới dạng LaTeX theo yêu cầu người dùng; nguồn thực thi dạng JSON (dev-orchestrator MODE 1).
- **Cơ sở**: Bảng đối chiếu master plan 20 task (đã rà code trước đó) → ưu tiên "đo được trước → P0 → tính năng".

## File đã tạo/sửa
| File | Trạng thái |
|------|-----------|
| `docs/roadmaps/next-branch.roadmap.json` | MỚI — 15 bước, đủ DoD + `verification_command` (R10) |
| `docs/roadmaps/next-branch.state.md` | MỚI — ledger + context gốc |
| `report/chapters/06_lo_trinh_nhanh_tiep.tex` | MỚI — chương 6: 4 giai đoạn (G1 kiểm thử → G2 P0 → G3 tính năng → G4 đo & vệ sinh) |
| `report/main.tex` | SỬA — thêm `\IfFileExists{chapters/06_lo_trinh_nhanh_tiep.tex}` |

## Kết quả kiểm thử (DoD)
- `json.load` → JSON OK: 15/15 steps khớp `total_steps`.
- `make` trong `report/` → Output written on main.pdf **(28 trang**, tăng từ 22) + `cp` → UMT_TBS_BaoCao.pdf.
- `make verify` → 4 keyword OK (Cấu trúc Dự án / Cấu trúc Thư mục / Báo cáo Dự án / Hệ thống).
- `pdftotext` → xác nhận có chương "6 Lộ trình nhánh tiếp theo", Hình 6.1, bảng G1–G4 trong PDF.
- `scan_secrets.py` → SECRET-SCAN OK (không chứa token).

## Ghi chú vận hành / demo
- Khi muốn tái lập PDF: `cd report && make && make verify`.
- Bắt đầu thực thi nhánh này: `pio run -e yolo_uno` cho từng bước (mỗi bước có lệnh xác minh riêng trong `next-branch.roadmap.json`).
- Quyết định/nghi vấn treo cần người dùng chốt trước bước 7 và 10: (1) buzzer active hay passive + có lắp transistor cấp 5V không; (2) thiết bị sensor-node Inactive CoreIoT — xác nhận 3 mục trong `.state.md`.
- Bước 5 (T2.3 crossing hazard): rule-chain là snapshot track — sau khi sửa JSON cần import lại trên CoreIoT console (R11). Fix ESP-NOW đã bị revert trước đó sẽ được nghiệm thu lại ở bước 7 (flash-and-observe).