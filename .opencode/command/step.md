---
description: Tạo prompt thực thi cho một bước (dev-orchestrator MODE 2) và xác minh DoD.
agent: build
---

Gọi skill `dev-orchestrator` ở **MODE 2 — Worker Execution Prompt**. Tham số `$1` = số bước (Step ID)
trong `docs/roadmaps/<slug>.roadmap.json`.

Đọc ledger `docs/roadmaps/<slug>.state.md` (nếu có), tạo payload self-contained cho Worker Agent theo
`references/worker-prompt-template.md`, chạy Worker, sau đó **tự mình xác minh DoD** (chạy lệnh
verification) và cập nhật ledger trước khi đề xuất bước kế tiếp.

Mọi DoD phải tôn trọng CONSTITUTION R1–R12. Nếu môi trường thiếu toolchain firmware (board build),
DoD cục bộ = host test/guard test; build thật dồn cho CI (B7).

```
Skill(skill="dev-orchestrator")
```