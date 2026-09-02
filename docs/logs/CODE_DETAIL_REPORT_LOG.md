# CODE_DETAIL_REPORT_LOG — Báo cáo chi tiết Code theo 4 lớp IoT

- **Ngày**: 2026-09-02
- **Roadmap**: `docs/roadmaps/code-detail-report.roadmap.json` (9 bước, MODE 1 + MODE 2 dev-orchestrator)
- **Sản phẩm**: `report-code/UMT_TBS_CodeDetail.pdf` + nguồn `report-code/`

## Mục tiêu
Viết báo cáo chi tiết code cho hệ thống **Hybrid V2** theo 4 lớp IoT, kèm
truy vết end-to-end "55 cm" — cụ thể hoá lại toàn bộ triển khai thật trong repo
(thay vì chỉ "sẽ làm").

## Phạm vi & file đã tạo/sửa
- `report-code/main.tex` — document chính (class report, 8 chương 00–07, macro `\pic`, `\IfFileExists`).
- `report-code/Makefile` — build/clean/verify (3 lượt pdflatex → PDF).
- `report-code/README.md`, `report-code/pictures/README.md` — hướng dẫn + danh sách ảnh chờ.
- `report-code/chapters/00_tong_quan.tex` (132d) — bảng 4 lớp↔repo + sơ đồ TikZ 2 đường.
- `report-code/chapters/01_perception.tex` (195d) — ISR echo, DistanceFilter, shared_state, tasks, GPIO map, buzzer.
- `report-code/chapters/02_network.tex` (183d) — ESP-NOW packed 30B, espnow_client, networkTask, MQTT CoreIoT 2 board.
- `report-code/chapters/03_processing.tex` (161d) — test_mqtt_coreiot.py, rule-chain 7 nút, JS transform, check thresholds R11.
- `report-code/chapters/04_application_core.tex` (124d) — main.c entry, sensor_model.{c,h}, BSP RGB LCD.
- `report-code/chapters/05_application_ui.tex` (114d) — ui_dashboard evaluate_hazard/mute/layout/system/theme.
- `report-code/chapters/06_ci_bao_mat.tex` (104d) — ci.yml 2 job, Gitleaks, guards, luồng bí mật R1.
- `report-code/chapters/07_ket_noi.tex` (104d) — truy vết 55 cm 2 đường, ma trận file↔lớp, chuỗi xác minh.
- `docs/roadmaps/code-detail-report.roadmap.json` + `.state.md` — ledger roadmap.
- `.gitignore` — thêm `/report-code/*.pdf`, `/report-code/*.log`.

## Chỉ số & ràng buộc
- Toàn bộ 8 chương đều ≤ 400 dòng (R7): 132/195/183/161/124/114/104/104.
- Tổng 1117 dòng nguồn báo cáo; PDF đầu ra 304937 B (17 trang chưa kể bia/TOC).

## Kết quả kiểm thử / verification
- `make -C report-code clean all` → exit 0, tạo `UMT_TBS_CodeDetail.pdf`.
- `pdftotext ... | grep -c <keyword>` — từng chương ≥ 1 (chương 2: 21, chương 3: 14, chương 7: 56 tổng).
- 8 chương đều ≤ 400 dòng (R7).
- `python3 tools/guard/scan_secrets.py` → `SECRET-SCAN OK` (exit 0).
- `/tmp/opencode/gitleaks/gitleaks detect --source . --config .gitleaks.toml` → `no leaks found`.
- `git check-ignore report-code/UMT_TBS_CodeDetail.pdf` → bị ignore (R8).
- Lỗi LaTeX đã sửa trong quá trình build:
  - unescaped `_` trong `\code{...}` (ESC-NOW `ESPNOW\_PEER\_MAC`, `coreiot\_client\_init()`, `ui\_dashboard\_*`, `SLOT\_FRONT`, `mask\_secret`).
  - `&` trong `\chapter{CI & Bảo mật}` → `CI \& Bảo mật`.
  - Unicode sau thay bằng công thức: `≤`→`$\le$`, `↔`→`$\leftrightarrow$`.

## Hướng dẫn usage / demo
1. **Build lại PDF**: `make -C report-code clean all` (tạo `report-code/UMT_TBS_CodeDetail.pdf`).
2. **Đọc lại**: mở `report-code/UMT_TBS_CodeDetail.pdf` (hoặc dùng `pdftotext UMT_TBS_CodeDetail.pdf - | less`).
3. **Xem cả bộ**: báo cáo chính `report/` (5 chương) + báo cáo chi tiết code này bổ trợ kỹ thuật.

## Ghi chú
- Các ảnh raster thật (hardware, dashboard) chưa chụp/thumb chưa có — `pictures/README.md`
  liệt kê 5 ảnh chờ; mọi `\includegraphics` đều bọc `\IfFileExists` + dùng macro `\pic`
  nên báo cáo vẫn build được khi chưa có ảnh.
- Thư mục gốc `picture/` (số ít, ảnh placeholder cũ) không được tham chiếu bởi báo cáo và
  không nằm trong commit này.
