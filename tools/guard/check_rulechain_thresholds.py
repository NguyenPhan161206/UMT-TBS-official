#!/usr/bin/env python3
"""check_rulechain_thresholds.py — đối chiếu ngưỡng trong rule-chain JSON với
firmware/shared/thresholds.h (R3/R11). Best-effort: bỏ qua sạch nếu file chưa tồn tại.

Usage:
  python3 tools/guard/check_rulechain_thresholds.py
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

RULE_CHAIN = Path("cloud/coreiot/rule_chain/supersonic_rule_chain.json")
THRESHOLDS_H = Path("firmware/shared/thresholds.h")

# Các ngưỡng "nhạy cảm" cần đối chiếu (cm) — mở rộng khi shared/thresholds.h hoàn thiện.
KEYS = ("caution_cm", "danger_cm", "CAUTION_CM", "DANGER_CM")


def extract_json_numbers(text: str) -> list[float]:
    """Best-effort: tìm các con số đứng cạnh từ khoá caution/danger trong file rule-chain."""
    nums = []
    for m in re.finditer(r"(?:caution|danger)[^0-9-]{0,40}(-?\d+(?:\.\d+)?)", text, re.IGNORECASE):
        nums.append(float(m.group(1)))
    return nums


def main() -> int:
    if not THRESHOLDS_H.exists() or not RULE_CHAIN.exists():
        print(f"CHECK-RULECHAIN SKIP: chưa có {RULE_CHAIN} hoặc {THRESHOLDS_H} — bỏ qua.")
        return 0

    rc_text = RULE_CHAIN.read_text(encoding="utf-8")
    rc_vals = set(extract_json_numbers(rc_text))

    th_text = THRESHOLDS_H.read_text(encoding="utf-8", errors="replace")
    th_vals = set()
    for name in KEYS:
        m = re.search(
            rf"\b{re.escape(name)}\s*[=:]\s*(\d+(?:\.\d+)?)", th_text, re.IGNORECASE
        )
        if m:
            th_vals.add(float(m.group(1)))

    if not rc_vals or not th_vals:
        print("CHECK-RULECHAIN SKIP: không trích được số nào (best-effort) — hãy rà tay.")
        return 0

    # Nếu rule-chain không chứa giá trị nào trong thresholds.h → cảnh báo.
    mismatch = th_vals - rc_vals
    if mismatch:
        print(
            "CHECK-RULECHAIN WARN: ngưỡng trong thresholds.h không xuất hiện trong rule-chain: "
            + ", ".join(str(v) for v in sorted(mismatch))
            + " — kiểm tra R3/R11."
        )
        return 1
    print(
        f"CHECK-RULECHAIN OK: thresholds.h {{{', '.join(sorted(str(v) for v in th_vals))}}} "
        f"đều có mặt trong rule-chain."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())