# REPORT_BUILD_LOG.md — Bộ tài liệu LaTeX + README

- **Ngày:** 2026-09-02
- **Người thực hiện:** dev-orchestrator (roadmap `docs/roadmaps/documentation.roadmap.json`)
- **Mục tiêu:** viết bộ tài liệu dự án gồm README (markdown) và báo cáo LaTeX 4 chương,
  dựa trên README/báo cáo cũ repo V1 nhưng cập nhật đúng hiện trạng V2.

## File đã tạo/sửa
| File | Loại | Nội dung |
|------|------|----------|
| `report/main.tex` | tạo | Preamble pdflatex + babel vietnamese + vntex (T5) + amssymb; bìa + abstract + TOC; `\input` 4 chương qua `\IfFileExists`. |
| `report/chapters/01_cau_truc_du_an.tex` | tạo | Chương 1: Cấu trúc Dự án — Hybrid V2, bảng 2 đường (ESP-NOW <50ms / CoreIoT 100–500ms), sơ đồ TikZ 4 khối, 5 tầng kiến trúc, ma trận phần tử↔file↔R#. |
| `report/chapters/02_cau_truc_thu_muc.tex` | tạo | Chương 2: Cấu trúc Thư mục — cây ASCII chuẩn Git, bảng chú giải, mục gitignored (R8), liên hệ thư mục↔tầng. |
| `report/chapters/03_bao_cao_du_an.tex` | tạo | Chương 3: Báo cáo Dự án — mục tiêu, phạm vi B0–B9, tiến độ (B2–B5/B7 ✅, B9 🔨), kết quả đo (RAM 12.8%/Flash 31.3%, 16 pytest, CI run 33592664432), hạn chế, hướng phát triển. |
| `report/chapters/04_he_thong.tex` | tạo | Chương 4: Báo cáo Chi tiết Hệ thống — H/W + GPIO map, firmware 2 board, shared contract, CoreIoT/rule-chain, CI, R1–R12, kiểm thử, vận hành. |
| `report/Makefile` | tạo | `make` = 3 lượt pdflatex → `UMT_TBS_BaoCao.pdf`; `clean`; `verify` (grep từ khóa PDF). |
| `report/README.md` | tạo | Hướng dẫn build + bảng mục lục chương. |
| `README.md` | sửa | Viết lại từ bản cũ (giữ nội dung còn đúng): Hybrid V2, tree, quick start, build/flash ACM0/ACM1, kiểm thử, B9, R1–R12, git workflow, trạng thái lộ trình đúng. |
| `report/chapters/05_cong_nghe.tex` | tạo | Chương 5: Công nghệ sử dụng — PlatformIO (2 core, Component Manager), CoreIoT/ThingsBoard (rule-chain 7 node + script JS), ESP-NOW, LVGL/GT911, FreeRTOS, CI; bảng nhu cầu→công nghệ. |
| `.gitignore` | sửa | Thêm `/report/*.pdf`, `/report/*.log` (R8). |
| `docs/roadmaps/documentation.roadmap.json` + `.state.md` | tạo | Roadmap + ledger (dev-orchestrator). |

## Kết quả kiểm thử (lệnh + kết quả)
```bash
make -C report clean all
# ==> report/UMT_TBS_BaoCao.pdf (230 KB, đủ bìa + TOC + 4 chương)

pdftotext report/UMT_TBS_BaoCao.pdf - | grep -cE "Chi tiết Hệ thống|GPIO|Rule-Chain|Cluster-EMA"
# 19

pdftotext report/UMT_TBS_BaoCao.pdf - | grep -cE "PlatformIO|CoreIoT|ThingsBoard|ESP-NOW|Component Manager|Rule-Chain"
# 91 (keyword chương 5)

wc -l report/chapters/*.tex
# 124 / 132 / 107 / 237 / 238 — tất cả < 400 dòng (R7/SELFCHECK 6)

python3 tools/guard/scan_secrets.py | tail -1
# SECRET-SCAN OK: no secret patterns found.

/tmp/opencode/gitleaks/gitleaks detect --source . --no-banner
# no leaks found

~/.venv-platformio/bin/python -m pytest tools/guard/test_guard.py -q
# 16 passed in 0.78s

python3 tools/guard/check_rulechain_thresholds.py
# CHECK-RULECHAIN OK: thresholds.h {100.0, 30.0} đều có mặt trong rule-chain.

git check-ignore report/UMT_TBS_BaoCao.pdf && git check-ignore report/report.log
# exit 0 (không track PDF/log)
```

## Hướng dẫn vận hành/demo
- Build lại báo cáo: `make -C report` (hoặc `make -C report verify`); xoá artifact: `make -C report clean`.
- PDF không commit (R8); muốn phát hành → GitHub Release/LFS.
- Số liệu/ngưỡng trong báo cáo đồng nhất với `firmware/shared/thresholds.h` và `docs/logs/*`.

## Ghi chú
- Đã xử lý 3 lỗi build: ký tự Unicode tree (U+251C) không có trong T5/T1 → chuyển cây sang ASCII; thiếu `\usepackage{amssymb}` cho `\checkmark`; `\caption` ngoài `longtable`; một `}` thừa thiếu `)`.
- Dev hardware/thật chưa có; nội dung flash/demo là hướng dẫn (B9-B chờ user).