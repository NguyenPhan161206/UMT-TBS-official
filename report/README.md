# Báo cáo UMT-TBS-official (LaTeX)

Bộ báo cáo kỹ thuật tổng hợp cho repo
[`UMT-TBS-official`](https://github.com/NguyenPhan161206/UMT-TBS-official) — kiến trúc
Hybrid V2 (ESP-NOW + CoreIoT/ThingsBoard).

## Yêu cầu

- TeX Live có `pdflatex` + gói `vntex` (T5) để gõ tiếng Việt (đã xác nhận trên máy dev:
  `pdflatex`, `vntex.sty`, `t5enc.def`).
- Toàn bộ package đã dùng: geometry, graphicx, booktabs, longtable, array, xcolor,
  hyperref, float, tikz, fancyhdr, titlesec, enumitem, caption, listings, babel.

## Build

```bash
make -C report            # 3 lượt pdflatex → report/UMT_TBS_BaoCao.pdf
make -C report verify     # build + kiểm tra từ khóa chính trong PDF (cần pdftotext)
make -C report clean      # xoá artifact build
```

## Nội dung (4 chương)

| Chương | File | Nội dung |
|---|---|---|
| 1 — Cấu trúc Dự án | `chapters/01_cau_truc_du_an.tex` | Kiến trúc Hybrid V2, 5 tầng, luồng dữ liệu 2 đường, ma trận phần tử↔file↔R# |
| 2 — Cấu trúc Thư mục | `chapters/02_cau_truc_thu_muc.tex` | Cây thư mục repo + chú giải, mục gitignored (R8) |
| 3 — Báo cáo Dự án | `chapters/03_bao_cao_du_an.tex` | Mục tiêu, yêu cầu, tiến độ roadmap B0–B9, kết quả, hạn chế, hướng phát triển |
| 4 — Báo cáo Chi tiết Hệ thống | `chapters/04_he_thong.tex` | Phần cứng/GPIO, firmware, shared contract, CoreIoT/rule-chain, CI, bảo mật R1–R12, kiểm thử, vận hành |

## Ghi chú

- **PDF không được commit** (R8: binary qua GitHub Release/LFS) — bị gitignore ở
  `/report/*.pdf`, `/report/*.log`.
- Số liệu/ngưỡng trong báo cáo phải khớp `firmware/shared/thresholds.h` và các log ở
  `docs/logs/` (xem `docs/roadmaps/documentation.roadmap.json`).