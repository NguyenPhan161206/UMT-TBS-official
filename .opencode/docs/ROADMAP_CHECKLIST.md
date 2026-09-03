# ROADMAP_CHECKLIST — Lộ trình V2 (Bản chuẩn cho Agent)

> Nguồn chuẩn hoá lộ trình. Mọi agent đối chiếu checklist này khi nhận việc. Thứ tự ưu tiên:
> **config/enforcement (B0–B1) → contract (B2) → firmware đo được (B3–B5) → CI (B7) → cloud demo (B9)**.

## Giai đoạn 0 — Nền & Config (B0–B1) ✅ XONG

| ID | Mô tả | Trạng thái |
|----|-------|------------|
| B0 | Repo `UMT-TBS-official` (Greenfield, 1 remote), git config, Node ≥20, PlatformIO | ✅ XONG (dir+init+push, 1 remote origin) |
| B0b | CoreIoT cách b: tạo tài khoản miễn phí, 2 device, token MỚI vào `config/keys.json`, import rule-chain | 🟡 MỘT PHẦN: rule-chain import OK (snapshot); ⏳ chờ token MỚI (R11) |
| B1 | Cấu hiến: CONSTITUTION R1–R12, SELFCHECK, ROADMAP_CHECKLIST V2, opencode.json (instructions+permission), agent (dev-orchestrator/arch-guard/secrets-responder), command (plan/step/scaffold/verify/commit/build/flash/test), plugin guard, skills port | ✅ XONG (commit đầu) |
| B6e | Enforcement: `tools/guard/*.py` + plugin guard | ✅ XONG (scan/gen/check_rulechain/check_size + CI gates) |

## Giai đoạn 1 — Shared contract (B2)

| ID | Mô tả | Trạng thái |
|----|-------|------------|
| B2 | `firmware/shared/espnow_protocol.h` (packed struct + channel + slot enum + MAC) | ✅ (66 dòng; packed 30B; static_assert khớp SENSOR_COUNT) |
| B2 | `firmware/shared/thresholds.h` (zone enum + ngưỡng + `SENSOR_PINS` + `SENSOR_COUNT` + static_assert) | ✅ (119 dòng; C11+C++17 build; scan_secrets OK) |

## Giai đoạn 2 — Firmware scaffold (B3–B5)

| ID | Mô tả | Trạng thái |
|----|-------|------------|
| B3 | `sensor-node`: sensor/filter/shared_state/buzzer/espnow + `plugins/coreiot` (USE_COREIOT, env `yolo_uno_coreiot`); unit test host; `static_assert` R4 | ✅ (build 2 env OK, 10/10 host test, log) |
| B4 | `waveshare-screen`: sensor_model + ui_dashboard (tách <400 dòng) + coreiot_client; size-gate | ✅ (build OK: RAM 12.8% / Flash 31.2%; scan_secrets OK; R2/R3/R7 OK; log) |
| B5 | Nối ESP-NOW qua shared header; xoá define trùng (grep == 1); flash-and-observe | ✅ (component espnow_receiver; grep==0 define ngoài shared; build OK: RAM 12.8%/Flash 31.3%; flash-and-observe chờ board — log) |

## Giai đoạn 3 — CI cứng (B7)

| ID | Mô tả | Trạng thái |
|----|-------|------------|
| B7 | GitHub Actions: build 2 env × 2 firmware + host tests + pytest tools + Gitleaks + assert sdkconfig/keys.json + size-gate + `check_rulechain_thresholds.py` | ✅ (`.github/workflows/ci.yml`; push/PR chạy; pin `espressif32@7.0.1` để tái lập build sau lần CI đỏ đầu — xem `docs/logs/CI_FIX_LOG.md`) |
| B7 | Protected branch `main`; PR phải xanh | ⏳ (thủ công trên GitHub — cần quyền admin) |

## Giai đoạn 4 — Cloud & Đo lường (B9 + GĐ sau)

| ID | Mô tả | Trạng thái |
|----|-------|------------|
| B9a | Tool CoreIoT V2: `tools/test_mqtt_coreiot.py` (field `d1..d6/nearest_cm`, zone 100/30), rule-chain snapshot V2, gate R11 | ✅ XONG (tool V2 + rule-chain snapshot; gate R11 OK, 16/16 pytest) |
| B9b | Nghiệm thu CoreIoT: token MỚI → flash → dashboard `warning_status` | ⏳ cần user tạo token MỚI → điền `config/keys.json` → flash → nghiệm thu (xem `docs/TEST_PROTOCOL.md` cấp 3) |
| T5.x | Đo lường hiệu năng (baseline, latency, soak…) — làm sau khi firmware ổn định | ⏳ |

## Tài liệu vận hành / kiểm thử

| ID | Mô tả | Trạng thái |
|----|-------|------------|
| DOC-A | `docs/TEST_PROTOCOL.md` — quy trình test 3 cấp (host / flash-and-observe / cloud) | ✅ XONG |
| DOC-B | `docs/PROGRESS.md` — bảng trạng thái tiến độ chi tiết (B0–B9, T5, CI) | ✅ XONG |

## Thứ tự ưu tiên
1. **Config + enforcement** (B0–B1, B6e) — đang thực thi.
2. **Đo được & kiểm thử được** (B2–B3 host test) trước khi sửa/feature.
3. **CI** (B7) bảo vệ toàn bộ sau khi firmware chạy.
4. **Cloud demo** (B9) sau cùng.

## Quy ước bắt buộc
- Build: `pio run -e yolo_uno` (hoặc `yolo_uno_coreiot` cho bản CoreIoT).
- Flash/monitor: `/dev/ttyACM0` (sensor-node), `/dev/ttyACM1` (waveshare-screen).
- Secret: chỉ `config/keys.json`; KHÔNG hardcode.
- Log: `docs/logs/<COMPONENT>_<TASK>_LOG.md` sau mỗi nhiệm vụ.