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

---

# ROADMAP NEXT-BRANCH — ĐẢO THỨ TỰ G1 (CẬP NHẬT)

- **Ngày**: 2026-09-09
- **Mục tiêu**: Đảo thứ tự 4 bước Giai đoạn 1 theo kiến trúc kiểm thử được duyệt (`docs/ARCHITECTURE_G1_TESTING.md`) — không thay đổi phạm vi công việc, chỉ đổi chuỗi + prerequisites.

## Lý do đảo (tóm tắt)
- **hazard_core lên đầu** (bước 1): foundation — host_sim (bước 3) và T2.3 (bước 5) đều phụ thuộc; T1.3 cũ chỉ là test hẹp qua pio-native (sai cơ chế cho espidf).
- **host_sim kéo từ cuối lên thứ 3**: item rủi ro/lợi nhuận lớn nhất → fail-fast; là tiên quyết T3.1/T3.2; chỉ cần schema (đã pin ở bước 2), không chờ T1.4.
- **record/replay đẩy xuống cuối**: nửa record phụ thuộc board + token thật (vật lý), nửa replay trivially đúng khi schema đã pin.

## File đã sửa
| File | Thay đổi |
|------|---------|
| `docs/roadmaps/next-branch.roadmap.json` | Đảo step 1–4: 1=hazard_core, 2=scenarios+T1.1, 3=host_sim, 4=T1.4; prerequisites đổi: step5→[1], step8→[3], step13→[4], step15 note=step4 |
| `docs/roadmaps/next-branch.state.md` | Cập nhật bảng ledger + Contracts (hazard_core, scenarios.py) + mục Deviations |
| `docs/ARCHITECTURE_G1_TESTING.md` | MỚI (nguồn kiến trúc: 3 lớp + kịch bản chung) |

## Kết quả kiểm thử (DoD)
- `python3 -m json.tool docs/roadmaps/next-branch.roadmap.json` → JSON OK, 15/15 steps.
- Sensor-node regression: `pio test -e native` → 10/10 PASSED (không đụng firmware).
- Chưa thực thi bước nào (chỉ lập kế hoạch lại).

## Ghi chú
- Thứ tự mới bắt đầu thực thi: bước 1 hazard_core → 2 scenarios/T1.1 → 3 host_sim → 4 T1.4 → 5 T2.3 ...
- Lệnh verify từng bước trong `next-branch.roadmap.json`. Khi chạm board: flash-and-observe bắt buộc (AGENTS.md).