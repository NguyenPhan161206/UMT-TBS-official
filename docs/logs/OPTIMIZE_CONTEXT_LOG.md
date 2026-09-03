# OPTIMIZE_CONTEXT_LOG

## Mục tiêu
Tối ưu OpenCode context size: giảm nội dung nạp vào mỗi session từ **385 dòng (5 files)** xuống
**~183 dòng (2 files)** để streaming nhanh hơn, model tập trung hơn — **mà không mất bất kỳ
quy tắc/enforcement nào** (R1–R12).

## Quyết định (theo nguyên tắc dev-orchestrator "quyết rồi nói")
- **Q1 → A**: chỉ bỏ `AGENTS.template.md` khỏi instructions, giữ file trên đĩa (vẫn cần cho
  `cross_device_reconfig` skill).
- **Q2 → A**: chuyển `ROADMAP_CHECKLIST.md` thành skill on-demand (`roadmap_checklist`).

## File đã sửa

| File | Thao tác |
|------|----------|
| `.opencode/opencode.json` | instructions cuối: chỉ `AGENTS.md` + `CONSTITUTION.md` |
| `.opencode/docs/CONSTITUTION.md` | Rút gọn 102 → 83 dòng (bỏ phần "Tại sao" mỗi rule, giữ Luật/Kiểm tra/Hành động; còn đủ 12 rules) |
| `.opencode/docs/SELFCHECK.md` | **XOÁ** — thay bằng skill `selfcheck` |
| `.opencode/docs/ROADMAP_CHECKLIST.md` | **XOÁ** — thay bằng skill `roadmap_checklist` |
| `.opencode/command/verify.md` | Đổi link SELFCHECK.md → gọi skill `selfcheck` |
| `.opencode/command/plan.md` | Đổi link ROADMAP_CHECKLIST.md → gọi skill `roadmap_checklist` |
| `.opencode/agent/dev-orchestrator.md` | Đổi link ROADMAP_CHECKLIST.md → gọi skill `roadmap_checklist` |
| `.agents/skills/selfcheck/SKILL.md` | **TẠO** (từ SELFCHECK.md, thêm frontmatter) |
| `.agents/skills/roadmap_checklist/SKILL.md` | **TẠO** (từ ROADMAP_CHECKLIST.md, thêm frontmatter) |

## Kết quả kiểm thử
- `opencode.json` hợp lệ JSON (json.load OK); instructions = 2 files; skills.paths đã cover `.agents/skills`.
- Cả 2 file instructions tồn tại. Cả 2 skill mới tồn tại, đúng path.
- Không còn dangling reference tới file đã xoá trong config/command/agent (grep = 0, loại trừ session/log).
- `CONSTITUTION.md` còn đủ **12 rules**.
- `scan_secrets.py` exit 0 (R1).

## Thống kê giảm context

| Metric | Trước | Sau | Giảm |
|--------|-------|-----|------|
| Files nạp mỗi session | 5 | 2 | -60% |
| Dòng nạp mỗi session | 385 | 183 | -52% |

## Hướng dẫn vận hành / demo
- **Thay đổi có hiệu lực ở session mới** (opencode.json đọc lúc khởi động).
- Khi hoàn thành task / trước commit: `/verify` vẫn hoạt động, tự gọi skill `selfcheck`.
- Khi `/plan` hoặc tra cứu tiến độ: agent gọi skill `roadmap_checklist`.
- `AGENTS.template.md` vẫn giữ nguyên trên đĩa phục vụ `cross_device_reconfig` (chỉ không nạp vào context).
