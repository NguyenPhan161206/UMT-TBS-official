# ROADMAP_CHECKLIST — Lộ trình V2 (Bản chuẩn cho Agent)

> Nguồn chuẩn hoá lộ trình. Mọi agent đối chiếu checklist này khi nhận việc. Thứ tự ưu tiên:
> **config/enforcement (B0–B1) → contract (B2) → firmware đo được (B3–B5) → CI (B7) → cloud demo (B9)**.

## Giai đoạn 0 — Nền & Config (B0–B1) ✅ ĐANG LÀM (commit đầu trong phạm vi này)

| ID | Mô tả | Trạng thái |
|----|-------|------------|
| B0 | Repo `UMT-TBS-official` (Greenfield, 1 remote), git config, Node ≥20, PlatformIO | ✅ dir+init; ⏳ push |
| B0b | CoreIoT cách b: tạo tài khoản miễn phí, 2 device, token MỚI vào `config/keys.json`, import rule-chain | ⏳ (thủ công, cần user) |
| B1 | Cấu hiến: CONSTITUTION R1–R12, SELFCHECK, ROADMAP_CHECKLIST V2, opencode.json (instructions+permission), agent (dev-orchestrator/arch-guard/secrets-responder), command (plan/step/scaffold/verify/commit/build/flash/test), plugin guard, skills port | 🔨 trong PR này |
| B6e | Enforcement: `tools/guard/*.py` + plugin guard | 🔨 trong PR này |

## Giai đoạn 1 — Shared contract (B2)

| ID | Mô tả | Trạng thái |
|----|-------|------------|
| B2 | `firmware/shared/espnow_protocol.h` (packed struct + channel + slot enum + MAC) | ✅ (66 dòng; packed 30B; static_assert khớp SENSOR_COUNT) |
| B2 | `firmware/shared/thresholds.h` (zone enum + ngưỡng + `SENSOR_PINS` + `SENSOR_COUNT` + static_assert) | ✅ (119 dòng; C11+C++17 build; scan_secrets OK) |

## Giai đoạn 2 — Firmware scaffold (B3–B5)

| ID | Mô tả | Trạng thái |
|----|-------|------------|
| B3 | `sensor-node`: sensor/filter/shared_state/buzzer/espnow + `plugins/coreiot` (USE_COREIOT, env `yolo_uno_coreiot`); unit test host; `static_assert` R4 | ✅ (build 2 env OK, 10/10 host test, log) |
| B4 | `waveshare-screen`: sensor_model + ui_dashboard (tách <400 dòng) + coreiot_client; size-gate | ⏳ |
| B5 | Nối ESP-NOW qua shared header; xoá define trùng (grep == 1); flash-and-observe | ⏳ |

## Giai đoạn 3 — CI cứng (B7)

| ID | Mô tả | Trạng thái |
|----|-------|------------|
| B7 | GitHub Actions: build 2 env × 2 firmware + host tests + pytest tools + Gitleaks + assert sdkconfig/keys.json + size-gate + `check_rulechain_thresholds.py` | ⏳ |
| B7 | Protected branch `main`; PR phải xanh | ⏳ |

## Giai đoạn 4 — Cloud & Đo lường (B9 + GĐ sau)

| ID | Mô tả | Trạng thái |
|----|-------|------------|
| B9 | Nghiệm thu CoreIoT: `tools/test_mqtt_coreiot.py` token mới → rule-chain → dashboard `warning_status` | ⏳ |
| T5.x | Đo lường hiệu năng (baseline, latency, soak…) — làm sau khi firmware ổn định | ⏳ |

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