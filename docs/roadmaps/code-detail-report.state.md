# Roadmap State — code-detail-report (Báo cáo chi tiết Code theo 4 lớp IoT)

- Project slug: `code-detail-report`
- Roadmap: `docs/roadmaps/code-detail-report.roadmap.json`
- Created: 2026-09-02
- Per AGENTS.md default workflow (dev-orchestrator MODE 1 → MODE 2).

## Status

| Step | Task | Status |
|------|------|--------|
| 1 | Scaffold report-code/ (main.tex + Makefile + README + pictures + .gitignore) | DONE |
| 2 | Ch.00 Tổng quan hành trình dữ liệu + sơ đồ TikZ | DONE |
| 3 | Ch.01 Lớp Cảm biến (Perception) | DONE |
| 4 | Ch.02 Lớp Mạng (Network) | DONE |
| 5 | Ch.03 Lớp Xử lý (Processing) | DONE |
| 6 | Ch.04 Ứng dụng core (sensor_model/main.c/BSP) | DONE |
| 7 | Ch.05 Ứng dụng UI (ui_dashboard) | DONE |
| 8 | Ch.06 CI & Bảo mật | DONE |
| 9 | Ch.07 end-to-end + verify + log + commit + push | DONE |

## Contracts established
- Document: `report-code/main.tex` (class `report`, pdflatex + babel vietnamese + vntex T5), build `make -C report-code` → `report-code/UMT_TBS_CodeDetail.pdf`.
- 8 chương: `report-code/chapters/00_tong_quan.tex` … `07_ket_noi.tex` — mỗi file ≤ 400 dòng (R7).
- Sơ đồ = TikZ (commit); ảnh raster = `report-code/pictures/` được track, mọi `\includegraphics` bọc `\IfFileExists`.
- Build artifacts gitignore: `/report-code/*.pdf`, `/report-code/*.log`.
- DoD mỗi bước: `make -C report-code clean all` exit 0 + `pdftotext` keyword ≥1 + `wc -l` ≤ 400.

## Deviations
- (chưa có). Toàn bộ 9 bước roadmap code-detail-report đã DONE (2026-09-02); xem `docs/logs/CODE_DETAIL_REPORT_LOG.md`.