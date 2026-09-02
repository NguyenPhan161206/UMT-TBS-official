# CI Pipeline Log — B7

**Ngày:** 2026-09-02
**Task:** B7 — GitHub Actions: build 2 env × 2 firmware + host tests + pytest tools +
Gitleaks + assert sdkconfig/keys.json + size-gate + `check_rulechain_thresholds.py`.
**Trạng thái:** ✅ Workflow committed & pushed (protected branch còn thủ công).

## Mục tiêu
Cứng hoá CI để: (1) mọi thay đổi firmware được build ít nhất ở env mặc định + env
CoreIoT (R6), (2) host test chạy không cần board (R10), (3) chặn secret P0 qua
Gitleaks + `scan_secrets.py` (R1), (4) đảm bảo `sdkconfig`/`keys.json` không bao giờ
bị track (R8), (5) firmware không vượt phân vùng flash (size-gate).

## File đã thêm / sửa
- **MỚI** `.github/workflows/ci.yml` — 2 job `firmware` + `security`.
- **MỚI** `tools/guard/check_size.py` — size-gate CLI: cặp `<file> <max_bytes>`, exit 0/1/2.
- **MỚI** `tools/guard/test_guard.py` — 11 pytest case cho guard scripts (gen_credentials,
  scan_secrets, check_rulechain_thresholds, check_size); toàn bộ dùng `tmp_path`, không đụng file tracked.
- **MỚI** `.gitleaks.toml` — giữ default rules + allowlist benign (commit hash, hex literal, MAC).
- **SỬA** `.opencode/docs/ROADMAP_CHECKLIST.md` — B7 ✅ (workflow), row protected branch giữ ⏳.

## Thiết kế workflow (`ci.yml`)
**Job firmware** (ubuntu-latest, timeout 40p):
1. `checkout@v4` → `setup-python 3.12` → `pip install -U platformio==6.1.19` (pin trùng máy local — R10).
2. Sinh **dummy credentials** (`config/keys.json` = giá trị `ci-dummy-*`; gitignored, không vào git) →
   `gen_credentials.py --out` cho sensor-node `include/credentials.h` và screen
   `components/coreiot_client/include/credentials.h`.
3. Build: sensor-node `yolo_uno` → sensor-node `yolo_uno_coreiot` → host tests `pio test -e native` →
   waveshare-screen `yolo_uno`.
4. Size-gate: sensor 1_200_000 B (app partition ~1.3MB), screen 3_700_000 B (factory 4MB, 92.5%).

**Job security** (timeout 15p): Gitleaks 8.24.0 (binary, full-history, `--exit-code 1`) →
`scan_secrets.py` → `gen_credentials.py --check` → `pytest tools/guard/test_guard.py` →
`check_rulechain_thresholds.py` (best-effort; hiện SKIP vì chưa có rule-chain snapshot) →
R8 assert `git ls-files` không chứa `sdkconfig`/`keys.json`.

## Kết quả kiểm thử (cục bộ, cùng lệnh với CI)
| Lệnh | Kết quả |
|---|---|
| `~/.venv-platformio/bin/python -m pytest tools/guard/test_guard.py -q` | ✅ 11 passed |
| `python3 tools/guard/check_size.py <coreiot>.bin 1200000 <screen>.bin 3700000` | ✅ 59.2% / 35.0% |
| `python3 tools/guard/scan_secrets.py` | ✅ exit 0 |
| `python3 tools/guard/gen_credentials.py --check` | ✅ OK (keys.json dummy hợp lệ) |
| `python3 tools/guard/check_rulechain_thresholds.py` | ✅ SKIP (chưa có snapshot) |
| `pio test -e native` (sensor-node) | ✅ 10/10 PASSED |
| `pio run -e yolo_uno_coreiot` (với credentials.h sinh dummy) | ✅ SUCCESS — xác nhận đường dẫn `include/credentials.h` resolve (PlatformIO auto-include `include/`) |
| YAML parse `.github/workflows/ci.yml` | ✅ jobs: firmware, security |

## Ghi chú vận hành
- **CHƯA làm (thủ công, cần quyền admin GitHub):** bật protected branch `main` + yêu cầu PR xanh
  trước khi merge. Cần bấm ở GitHub repo settings (branch protection rules).
- CI dùng **dummy token** (`ci-dummy-*`) chỉ để build; **không** phải token thật (R1/R11).
  Trước khi flash thật (B9), thay `config/keys.json` bằng token mới rồi chạy `gen_credentials.py`.
- `check_rulechain_thresholds.py` đang SKIP vì `cloud/coreiot/rule_chain/supersonic_rule_chain.json`
  chưa tồn tại (B0b/R11). Khi import rule-chain xong, thêm snapshot thì chốt này thành gate thực.
- Gitleaks chạy binary pinned (không cần license action; hoạt động cả repo private).

## Demo / kiểm tra lại
- Sau khi push: mở GitHub → Actions → chọn workflow `CI` → cả 2 job phải xanh.
- Muốn chạy lại cục bộ nhanh: `python3 -m pytest tools/guard/test_guard.py -q`
  (cần `pytest`; trên máy dev đã cài vào `~/.venv-platformio`).