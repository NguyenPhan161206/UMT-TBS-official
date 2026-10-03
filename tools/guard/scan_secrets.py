#!/usr/bin/env python3
"""scan_secrets.py — quét secret trong file tracked / staged diff.

R1: token CoreIoT, Wi-Fi password, API key KHÔNG được nằm trong file tracked.
Exit code 0 = sạch; 1 = có hit (P0).

Usage:
  python3 tools/guard/scan_secrets.py                # quét toàn file tracked trong git
  python3 tools/guard/scan_secrets.py --staged       # chỉ quét dữ liệu staged (git diff --cached)
  python3 tools/guard/scan_secrets.py --path tools/  # quét thư mục/file cụ thể
  python3 tools/guard/scan_secrets.py --json         # output dạng JSON cho CI/plugin
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding="utf-8")
        sys.stderr.reconfigure(encoding="utf-8")
    except Exception:
        pass

# Lưu ý: placeholder trong template (like "<...>") KHÔNG tính là secret.
SECRET_PATTERNS: list[tuple[str, re.Pattern[str]]] = [
    (
        "thingsboard-access-token",
        re.compile(
            r"(?:access[_-]?token|device[_-]?token|COREIOT\w*TOKEN)\s*"
            r"[=:]\s*[\"']([A-Za-z0-9_]{16,})[\"']",
            re.IGNORECASE,
        ),
    ),
    (
        "generic-key-value",
        re.compile(
            r"(?:\b|(?<=_))(?:wifi[_-]?)?(token|password|passwd|secret|api[_-]?key|access[_-]?key)\s*"
            r"[=:]\s*[\"']([A-Za-z0-9_\-!@#$%^&*]{8,})[\"']",
            re.IGNORECASE,
        ),
    ),
    (
        "doc-pass-phrase",
        re.compile(
            r"\b(?:pass|password)\s+[\"']([A-Za-z0-9_\-!@#$%^&*]{8,32})[\"']",
            re.IGNORECASE,
        ),
    ),
    (
        "doc-wifi-slash-pair",
        re.compile(
            r"(?:wifi|hotspot|ssid|credential|keys\.json)[^\n\r]{0,100}?\b([a-zA-Z0-9_\-]{2,20}\s*/\s*[a-zA-Z0-9_\-!@#$%^&*]{8,30})\b",
            re.IGNORECASE,
        ),
    ),
    ("github-pat", re.compile(r"\bghp_[A-Za-z0-9]{20,}\b")),
    ("aws-access-key", re.compile(r"\bAKIA[0-9A-Z]{16}\b")),
    ("private-key-block", re.compile(r"-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----")),
]

KNOWN_PATH_PREFIXES = (
    "firmware/", "components/", "tools/", "src/", "include/", "docs/",
    "lib/", "test/", "build/", "report/", "cloud/", "app/", ".github/",
)

# Giá trị placeholder chấp nhận được (template/example) — KHÔNG báo.
PLACEHOLDER_RE = re.compile(
    r"^<[A-Za-z0-9_]+>$|^your[_-]?(token|key|password|wifi_password)$|^xxx+$|^example$",
    re.IGNORECASE,
)


def git_tracked_files() -> list[str]:
    out = subprocess.run(
        ["git", "ls-files"], check=False, capture_output=True, text=True,
        encoding="utf-8", errors="replace"
    )
    if out.returncode != 0:
        return []
    return [ln.strip() for ln in out.stdout.splitlines() if ln.strip()]


def git_staged_diff() -> str:
    out = subprocess.run(
        ["git", "diff", "--cached", "--no-color"], check=False, capture_output=True, text=True,
        encoding="utf-8", errors="replace"
    )
    return out.stdout or ""


def scan_lines(path: str, lines: list[str], hits: list[dict]) -> None:
    for i, line in enumerate(lines, start=1):
        for name, rx in SECRET_PATTERNS:
            m = rx.search(line)
            if not m:
                continue
            val = m.group(0)
            captured = m.group(1) if m.groups() and m.group(1) else val
            captured_clean = captured.strip("\"' \t")
            # Bỏ qua placeholder
            if PLACEHOLDER_RE.search(captured_clean) or (
                captured_clean.startswith("<") and captured_clean.endswith(">")
            ):
                continue
            # Bỏ qua nếu là đường dẫn mã nguồn / thư mục dự án
            if any(captured_clean.lower().startswith(p) for p in KNOWN_PATH_PREFIXES):
                continue
            if any(captured_clean.lower().endswith(ext) for ext in (".py", ".h", ".c", ".cpp", ".md", ".json", ".yml", ".txt")):
                continue
            if name == "doc-wifi-slash-pair":
                parts = captured_clean.split("/")
                if len(parts) == 2:
                    left, right = parts[0].strip(), parts[1].strip()
                    # Bỏ qua GitHub actions / repo tags
                    if "@v" in right or left.lower() in ("softprops", "actions"):
                        continue
                    has_digits = any(c.isdigit() for c in right)
                    has_alpha = any(c.isalpha() for c in right)
                    has_special = any(c in "!@#$%^&*" for c in right)
                    if not ((has_alpha and has_digits) or has_special):
                        continue
            hits.append({"file": path, "line": i, "pattern": name, "match": val})


def main() -> int:
    ap = argparse.ArgumentParser(description="Scan secrets in tracked files (R1).")
    ap.add_argument("--staged", action="store_true", help="scan staged diff only")
    ap.add_argument("--path", type=str, default=None, help="scan specific file/dir")
    ap.add_argument("--json", action="store_true", help="JSON output")
    args = ap.parse_args()

    hits: list[dict] = []

    if args.staged:
        diff = git_staged_diff()
        lines = diff.splitlines()
        scan_lines("(staged diff)", lines, hits)
    else:
        paths: list[str]
        if args.path:
            p = Path(args.path)
            paths = (
                [str(f) for f in sorted(p.rglob("*")) if f.is_file() and ".git" not in str(f)]
                if p.is_dir()
                else [str(p)]
            )
        else:
            paths = git_tracked_files()
        for f in paths:
            try:
                text = Path(f).read_text(encoding="utf-8", errors="replace")
            except OSError:
                continue
            scan_lines(f, text.splitlines(), hits)

    if args.json:
        print(json.dumps({"hits": hits, "count": len(hits)}, indent=2, ensure_ascii=False))
    else:
        if hits:
            print("SECRET-SCAN FAIL (%d hit(s)):" % len(hits))
            for h in hits:
                print(f"  - [{h['pattern']}] {h['file']}:{h['line']}  {h['match'][:60]}")
        else:
            print("SECRET-SCAN OK: no secret patterns found.")

    return 1 if hits else 0


if __name__ == "__main__":
    sys.exit(main())