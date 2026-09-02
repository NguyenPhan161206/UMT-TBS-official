# CONTRIBUTING.md — Quy tắc đóng góp (V2)

> Mọi commit phải tuân theo **CONSTITUTION.md (R1–R12)** — đọc trước khi làm việc.

## Git Workflow

```bash
git pull origin main --rebase      # luôn rebase, không merge commit
# ... edit ...
/verify                            # opencode command: scan secret + selfcheck
git add <file cụ thể>              # CẤM: git add -A / git add .
git commit -m "<type>: <mô tả>"
git push origin main               # chỉ push origin; reject → pull --rebase, xử lý, push lại
```

### Commit message (conventional)
- `feat:` tính năng mới · `fix:` sửa lỗi · `refactor:` tái cấu trúc · `docs:` tài liệu ·
  `chore:` vệ sinh/build · `test:` thêm/sửa test · `ci:` CI.
- `$ git log --oneline` nên đọc như một câu chuyện.

## Quy tắc bắt buộc

1. **Không bao giờ commit secret.** Token CoreIoT, Wi-Fi password không được nằm trong file tracked.
   Chỉ đọc từ `config/keys.json` hoặc `credentials.h` (sinh từ keys.json, gitignored).
2. **Không `git add -A` / `git add .`.** Luôn stage file cụ thể đã kiểm tra.
3. **Một remote.** Chỉ push `origin`. Không thêm remote cá nhân khác; không force push.
4. **DoD phải có lệnh xác minh chạy được** — "code compiles" không phải DoD.
5. **Contract dùng chung chỉ ở `firmware/shared/`** (R2) — ngưỡng chỉ ở `thresholds.h` (R3).
6. **Không demo/dead code trong production** (R5/R6) — demo để ở `prototypes/`.
7. **File > 400 dòng cần tách** (R7).
8. **Kèm log** `docs/logs/<COMPONENT>_<TASK>_LOG.md` sau mỗi nhiệm vụ (AGENTS).

## Cách làm việc với kế hoạch (roadmap)
- Dùng `/plan` để phân rã, `/step N` để thực thi từng bước, `/verify` trước khi báo xong.
- Cập nhật `docs/roadmaps/<slug>.state.md` sau mỗi bước DONE.

## Xử lý vi phạm
- Plugin guard `/verify` từ chối → tìm hiểu lý do, sửa, không bypass.
- Nếu cần `git add -A` (hiếm, chỉ khi refactor toàn repo): ghi rõ trong commit message và
  liệt kê nhóm file trong pull request.