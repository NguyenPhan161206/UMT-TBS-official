# sensor-release-confirm — Orchestration State
Roadmap: docs/roadmaps/sensor-release-confirm.roadmap.json
Nguồn: phân tích "cảm biến kém chính xác hơn trước" (commit 21/9: fast-track nhả cảnh báo chỉ với 1 mẫu).
Người dùng duyệt: "ok áp dụng bản sửa đi" (gồm nạp COM7). Thực thi trực tiếp bởi orchestrator theo yêu cầu.
Updated: 2026-10-03

| Step | Title | Status | Verified by | Notes |
|------|-------|--------|-------------|-------|
| 1 | Hằng số FILTER_RELEASE_CONFIRM_SAMPLES (thresholds.h) | DONE | native 16/16; `pio run -e yolo_uno` SUCCESS; đặt thử 1 và 10 → static_assert chặn build | build waveshare-screen kiểm ở step 4 |
| 2 | Test nhả cần N mẫu xa (phải đỏ trước) | DONE | Trước khi sửa lọc: đúng 4 test đỏ (đầu ra nhảy sang 300/190/320 sau 1 mẫu xa), 15 test khác pass | 3 test mới + sửa `test_filter_fast_track_crossing` |
| 3 | distance_filter: chiều RA cần N mẫu | DONE | native 19/19 pass; mô phỏng filter trong repo = bản đề xuất B (echo ảo lẻ tẻ 20%: 0,1% đầu ra xa, trước 34,8%) | test LCG ban đầu đỏ vì chuỗi có 5 echo ảo LIÊN TIẾP (nhả hợp lệ): sửa ngưỡng test 3 → 10, không đổi lọc |
| 4 | Build 2 env, nạp COM7, đo lại, log + CHECKLIST | DONE (đo lại CHƯA so sánh được) | sensor-node 2 env + waveshare SUCCESS; nạp COM7 `Hash of data verified`; arch_guard/scan_secrets OK, pytest 29 (PYTHONUTF8=1); CHECKLIST T3.5 → 🟡, bảng tổng hợp khớp | Log sau nạp: cảnh đã đổi (không còn vật 22,6 cm) → không so sánh được; cần người dùng đặt lại vật rồi bắt lại. Wi-Fi/ESP-NOW chưa nối là vấn đề riêng |

## Số liệu cơ sở (trước khi sửa)
- Log thật S3 (LEFT_REAR) 25 s: 253 mẫu, 48% báo xa (>100 cm), 37 lần nhảy >=30 cm (88 lần/phút), không có REJECT.
- Mô phỏng bộ lọc HEAD vs đề xuất: xem phân tích trong hội thoại 2026-10-03 (kịch bản echo ảo lẻ tẻ 20%: HEAD 35% sai, đề xuất 0,1%).

## Contracts established
(điền sau mỗi step)

## Deviations from plan
(điền khi có)
