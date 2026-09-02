#!/usr/bin/env python3
"""check_size.py — size-gate cho firmware binary (R10 / B7).

Đảm bảo mỗi firmware.bin không vượt quá cap đã khai báo (bytes). Chạy trong CI
sau khi build; không cần board. Exit code gọn để workflow kiểm tra được.

Usage:
  python3 tools/guard/check_size.py <firmware.bin> <max_bytes> [<firmware.bin> <max_bytes> ...]

Exit:
  0 — mọi file tồn tại và ≤ max_bytes
  1 — có file vượt cap hoặc thiếu (file không tồn tại)
  2 — sai số lượng đối số / max_bytes không hợp lệ

Ví dụ:
  python3 tools/guard/check_size.py \
      .pio/build/yolo_uno/firmware.bin 1200000 \
      .pio/build/yolo_uno_coreiot/firmware.bin 1200000
"""
from __future__ import annotations

import sys
from pathlib import Path


def main(argv: list[str]) -> int:
    if len(argv) < 2 or len(argv) % 2 != 0:
        print(
            "ERROR: cần cặp <firmware.bin> <max_bytes> (đủ 2 đối số cho mỗi file).",
            file=sys.stderr,
        )
        return 2

    failures: list[str] = []
    total_ok = 0

    for i in range(0, len(argv), 2):
        raw_path, raw_max = argv[i], argv[i + 1]
        try:
            max_bytes = int(raw_max)
            if max_bytes <= 0:
                raise ValueError
        except ValueError:
            print(f"ERROR: max_bytes không hợp lệ: {raw_max!r}", file=sys.stderr)
            return 2

        p = Path(raw_path)
        if not p.is_file():
            failures.append(f"{raw_path}: MISSING (file không tồn tại)")
            continue

        size = p.stat().st_size
        pct = 100.0 * size / max_bytes
        if size > max_bytes:
            failures.append(
                f"{raw_path}: {size} B > cap {max_bytes} B ({pct:.1f}%) — VƯỢT"
            )
        else:
            total_ok += 1
            print(f"SIZE-GATE OK: {raw_path}: {size} B / {max_bytes} B ({pct:.1f}%)")

    if failures:
        print("SIZE-GATE FAIL:", file=sys.stderr)
        for f in failures:
            print(f"  - {f}", file=sys.stderr)
        return 1

    print(f"SIZE-GATE OK: {total_ok} file(s) trong giới hạn.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))