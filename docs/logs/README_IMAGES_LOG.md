# README_IMAGES_LOG — Thêm ảnh minh họa vào README

- **Ngày**: 2026-09-02
- **Mục tiêu**: bổ sung ảnh trực quan vào `README.md` để minh họa cảm biến,
  kiến trúc Hybrid, rule-chain CoreIoT và dashboard; tuân R8 (nén ảnh, tránh phình repo).

## Quyết định (đã chốt với user)
1. **Giữ sơ đồ ASCII** kiến trúc hiện tại, thêm ảnh bổ sung.
2. **Dùng 4 ảnh hữu ích** từ `picture/` (repo cũ V1): architecture, sr04t,
   rule-chain, dashboard. **Bỏ** `image.png` (không rõ nội dung).
3. **Nén trước khi nhúng** — đặc biệt `architecture.png` 6.6 MB.

## File đã sửa/tạo
- `README.md` — chèn 4 ảnh (cú pháp `<img>` Markdown):
  - `docs/images/sr04t.png` — mục giới thiệu (dòng ~10).
  - `docs/images/architecture.png` — mục Kiến trúc, sau sơ đồ ASCII (dòng ~37).
  - `docs/images/rule-chain.png` — mục Nghiệm thu CoreIoT B9 (dòng ~122).
  - `docs/images/dashboard-screen-img.jpg` — mục Nghiệm thu CoreIoT B9 (dòng ~127).
- `docs/images/` — 4 ảnh đã nén (nguồn từ `picture/`):
  - `architecture.png` 6.6 MB → **340 KB** (`convert -resize 1400x -strip -colors 256 -quality 80`).
  - `sr04t.png` 292 KB → 288 KB (`convert -strip -quality 90`).
  - `rule-chain.png` 56 KB (giữ nguyên).
  - `dashboard-screen-img.jpg` 92 KB (giữ nguyên).

## Kết quả verify
- 4 ảnh được `README.md` tham chiếu đúng đường dẫn `docs/images/*`.
- `python3 tools/guard/scan_secrets.py` → `SECRET-SCAN OK` (exit 0).
- `/tmp/opencode/gitleaks/gitleaks detect --source . --config .gitleaks.toml` → no leaks.
- Kích thước ảnh nén: tổng ~776 KB (không phình repo, tuân R8).
- README cú pháp Markdown hợp lệ (không build firmware bị ảnh hưởng).

## Hướng dẫn usage / demo
- Mở `README.md` trên GitHub (hoặc editor hỗ trợ preview Markdown) để xem ảnh.
- Muốn thay ảnh: ghi đè file tương ứng trong `docs/images/` (không đổi tên), giữ
  kích thước đã nén.

## Ghi chú
- Model không xem được nội dung ảnh — dựa trên tên file + ngữ cảnh lịch sử để chọn
  vị trí nhúng. Nếu cần ảnh khác chính xác hơn, thay file trong `docs/images/`.
- Thư mục gốc `picture/` (ràc, không tham chiếu) **không nằm** trong commit này.
