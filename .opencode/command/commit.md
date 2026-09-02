---
description: Commit theo quy trình (status → /verify → scan staged → stage file cụ thể → conventional → push origin).
agent: build
---

1. `git status` + `git diff --stat` — xem có gì sẽ commit.
2. Chạy `/verify` (hoặc tương đương):
   ```bash
   python3 tools/guard/scan_secrets.py --staged   # bắt buộc exit 0
   ```
3. **Stage từng file cụ thể** (CẤM `git add -A` / `git add .` — R9):
   ```bash
   git add <file1> <file2> ...
   ```
4. Xác nhận staged đúng: `git status --short`.
5. Commit message conventional: `feat:|fix:|docs:|refactor:|chore:|test:|ci:` + mô tả ngắn
   (nếu `$ARGUMENTS` có message thì dùng; không thì hỏi hoặc tự viết theo nội dung).
   ```bash
   git commit -m "<type>: <mô tả>"
   ```
6. Push `origin` (chỉ origin, không force):
   ```bash
   git push origin main
   ```
   Reject → `git pull origin main --rebase`, giải quyết xung đột, push lại.

Nếu bất kỳ bước nào fail (scan hit, stage nhầm) → dừng, báo rõ, không bypass.