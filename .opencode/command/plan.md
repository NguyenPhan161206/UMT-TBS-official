---
description: Phân rã kế hoạch thành roadmap atomic (dev-orchestrator MODE 1).
agent: plan
---

Gọi skill `dev-orchestrator` ở **MODE 1 — Decomposition**. Trước hết đọc:
- `AGENTS.md` và gọi skill `roadmap_checklist` (ưu tiên B0–B9)
- `.opencode/docs/CONSTITUTION.md` (R1–R12 — DoD phải tôn trọng)

Tạo `docs/roadmaps/<slug>.roadmap.json` (theo `references/roadmap-schema.json` của skill) và xuất
bảng tóm tắt các bước atomic (1–3 file/bước, mỗi bước kiểm chứng độc lập), đường tới hạn, và các
quyết định kiến trúc phải gọi tên.

Nếu `$ARGUMENTS` có sẵn kế hoạch/Master Plan, dùng nó. Nếu không, người dùng sẽ dán kế hoạch.

```
Skill(skill="dev-orchestrator")
```