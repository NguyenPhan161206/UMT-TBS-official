# Báo cáo Chi tiết Code — report-code/

Báo cáo LaTeX đi sâu vào từng folder / file / hàm / dòng code của
`UMT-TBS-official`, tổ chức theo **4 lớp IoT chuẩn** + CI/Bảo mật + chương
nối các lớp.

## Cách build

```bash
make -C report-code          # → report-code/UMT_TBS_CodeDetail.pdf (3 lượt pdflatex)
make -C report-code verify   # build + grep từ khóa chính
make -C report-code clean    # xoá artifact
```

## Mục lục chương

| File | Chương |
|------|--------|
| `chapters/00_tong_quan.tex` | 0 — Tổng quan hành trình dữ liệu (4 lớp ↔ cây repo, sơ đồ TikZ) |
| `chapters/01_perception.tex` | 1 — Lớp Cảm biến (Perception) |
| `chapters/02_network.tex` | 2 — Lớp Mạng (Network): ESP-NOW + MQTT |
| `chapters/03_processing.tex` | 3 — Lớp Xử lý (Processing): Rule-Chain + tools |
| `chapters/04_application_core.tex` | 4 — Lớp Ứng dụng core: sensor_model + main.c + BSP |
| `chapters/05_application_ui.tex` | 5 — Lớp Ứng dụng UI: ui_dashboard |
| `chapters/06_ci_bao_mat.tex` | 6 — CI & Bảo mật |
| `chapters/07_ket_noi.tex` | 7 — Nối các lớp end-to-end |

## Hình ảnh

- **Sơ đồ vector**: vẽ bằng TikZ ngay trong `main.tex`/chương (committed).
- **Ảnh raster**: đặt vào `pictures/` (xem `pictures/README.md`). Mọi
  `\includegraphics` được bọc `\IfFileExists` nên build không gãy khi thiếu ảnh.
- Build artifacts (`*.pdf`, `*.log`) bị gitignore (R8); PDF phát hành qua
  GitHub Release nếu cần.