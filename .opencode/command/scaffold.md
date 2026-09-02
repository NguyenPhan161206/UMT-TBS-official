---
description: Toàn văn bản mẫu cho bước scaffold B0-B2 khi tạo repo mới (config + enforcement).
agent: plan
---

Dùng khi bắt đầu repo V2 (hoặc tái dựng trên máy mới từ `AGENTS.template.md`). Làm theo thứ tự:

1. `cp AGENTS.template.md AGENTS.md` (file local, gitignored).
2. `cp config/keys.template.json config/keys.json` rồi **nhập token CoreIoT MỚI** (cách b — không
   tái dùng token đã lộ). `chmod 600 config/keys.json`.
3. Kiểm môi trường: `node --version` (cần v20+; nếu thiếu → plugin guard hoạt động "optional"),
   `python3 --version`, `pip install platformio` (cho firmware, tuỳ chọn lúc này).
4. Cài plugin: `cd .opencode && npm install && cd ..`.
5. Xác minh config: đọc qua từng file (`opencode.json`, `CONSTITUTION`, `SELFCHECK`, `ROADMAP_CHECKLIST`,
   agent, command, plugin) + `python3 tools/guard/scan_secrets.py` exit 0.
6. Commit đầu tiên (chỉ file cụ thể — không `git add -A`) với message
   `chore: scaffold opencode config + enforcement layer (V2)`.

Ghi chú: bước 2 là thủ công trên web (tạo account CoreIoT free, 2 device, import rule-chain) — in
hướng dẫn từng dòng và dừng chờ user thao tác nếu cần.