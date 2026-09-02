---
description: Chạy các bài kiểm thử (unit test, script guard, mô phỏng) theo phạm vi.
agent: build
---

Chạy bộ kiểm thử phù hợp với phạm vi `$ARGUMENTS`:

- **Guard/enforcement**: `python3 tools/guard/scan_secrets.py` (exit 0),
  `python3 tools/guard/check_rulechain_thresholds.py`.
- **Unit test C++ host** (DistanceFilter, thresholds): tìm trong `firmware/*/test/` — build với g++
  thuần (không phụ thuộc Arduino):
  ```bash
  g++ -std=c++17 -I firmware/shared ... test/<name>.cpp -o /tmp/test_<name> && /tmp/test_<name>
  ```
- **Python**: `python3 -m pytest tools/` (nếu có) hoặc chạy script trực tiếp.
- **Mô phỏng LVGL/SDL** (nếu có): chạy binary mô phỏng trên PC.

Báo kết quả đậu/rớt từng nhóm; nếu rớt: file + nguyên nhân + đề xuất sửa. Mọi test phải có thể
"FAIL" — test luôn đậu không phải DoD.