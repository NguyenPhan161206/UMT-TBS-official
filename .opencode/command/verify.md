---
description: Kiểm tra toàn diện trước khi báo DONE / commit (secret scan + guard check + SELFCHECK).
agent: build
---

Chạy chuỗi kiểm tra bắt buộc (R1–R12; đặc biệt R1, R3, R4, R8, R10):

```bash
python3 tools/guard/scan_secrets.py                 # exit 0 = không lộ secret
python3 tools/guard/gen_credentials.py --check      # keys.json đủ field (nếu tồn tại)
python3 tools/guard/check_rulechain_thresholds.py   # best-effort, bỏ qua nếu chưa có file
```

Nếu `$ARGUMENTS` có phạm vi (file/folder), quét thêm `--path <scope>`.
Sau đó tự trả lời `.opencode/docs/SELFCHECK.md` (10 câu) và xuất kết luận:
`PASS — sẵn sàng commit` hoặc `FAIL — <R# nào, file nào, cách sửa>`.

Không bao giờ kết luận PASS nếu `scan_secrets.py` khác 0.