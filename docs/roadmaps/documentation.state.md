# Roadmap State — UMT-TBS documentation (LaTeX reports)

- Project slug: `documentation`
- Roadmap: `docs/roadmaps/documentation.roadmap.json`
- Created: 2026-09-02
- Per AGENTS.md default workflow (dev-orchestrator MODE 1 → MODE 2).

## Status

| Step | Task | Status |
|------|------|--------|
| 1 | Scaffold report/ (main.tex + Makefile + README) | DONE |
| 2 | Gitignore report build artifacts | DONE |
| 3 | Chương 1–2: cấu trúc dự án + thư mục | DONE |
| 4 | Chương 3: báo cáo dự án | DONE |
| 5 | Chương 4: báo cáo chi tiết hệ thống | DONE |
| 6 | README.md gốc (markdown) | DONE |
| 7 | Verify cuối + log build | DONE |

## Contracts established
- Báo cáo: 1 PDF tổng hợp `report/UMT_TBS_BaoCao.pdf` (không commit — R8) từ `report/main.tex` (class `report`, pdflatex + babel vietnamese + vntex T5 + amssymb).
- 4 chương: `report/chapters/01_cau_truc_du_an.tex`, `02_cau_truc_thu_muc.tex`, `03_bao_cao_du_an.tex`, `04_he_thong.tex` — mỗi file ≤ 400 dòng (R7/SELFCHECK #6).
- Build: `make -C report` (3 lượt pdflatex) → `report/UMT_TBS_BaoCao.pdf`.
- Build artifacts bị gitignore: `/report/*.pdf`, `/report/*.log` (step 2).
- README.md gốc giữ nội dung V1 còn đúng; token/Wi-Fi chỉ nhắc đến `config/keys.json` (R1).

## Deviations
- (chưa có)