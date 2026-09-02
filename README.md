# UMT-TBS-official

Hệ thống **Cảnh báo va chạm xe tải (Truck Blind-spot Warning System)** — repo chính thức V2
(Greenfield). ESP32-S3 + JSN-SR04T + Màn hình Waveshare 7" RGB LVGL v9.

## Kiến trúc Hybrid (V2)

| Đường | Vai trò | Giao thức | Độ trễ mục tiêu |
|---|---|---|---|
| **ESP-NOW (chính)** | Cảnh báo critical path giữa sensor-node → waveshare-screen | ESP-NOW, shared header duy nhất | < 50 ms |
| **CoreIoT / MQTT (phụ)** | Giám sát từ xa, Rule-Chain, dashboard (yêu cầu cloud của đề xuất) | MQTT `v1/devices/me/telemetry` | 100–500 ms |

## Cấu hình opencode — 4 tầng enforcement (R1–R12)

| Tầng | Cơ chế | Nơi |
|---|---|---|
| ① Cấu hiến | `instructions` nạp mỗi session | `.opencode/docs/CONSTITUTION.md`, `SELFCHECK.md` |
| ② Vai & lệnh | agent/command/skill | `.opencode/agent/*`, `.opencode/command/*`, `.claude/skills`, `.agents/skills` |
| ③ Permission | `permission.bash` (last-match) | `.opencode/opencode.json` |
| ④ Plugin guard + CI | hook `tool.execute.before` + (tương lai) GitHub Actions | `.opencode/plugin/guard.ts`, `.github/workflows/ci.yml` |

Quy tắc cốt lõi (đầy đủ: `CONSTITUTION.md`):

- **R1 Secrets** — credential chỉ ở `config/keys.json` hoặc file sinh từ nó; literal token/password trong tracked file = P0.
- **R2 Shared contract** — struct/define dùng chung 2 board chỉ ở `firmware/shared/`.
- **R3 Thresholds 1 nguồn** — ngưỡng zone chỉ ở `firmware/shared/thresholds.h`.
- **R8/R9 Git hygiene** — 1 remote; commit conventional; không `git add -A`; sdkconfig/keys không track.
- **R10 Test** — mọi task có DoD kèm lệnh xác minh.

## Cài đặt nhanh

```bash
# 1. Key local (gitignored — KHÔNG commit)
cp config/keys.template.json config/keys.json   # điền token CoreIoT mới vào

# 2. PlatformIO (cho phần firmware — làm sau giai đoạn config)
pip install platformio

# 3. Plugin guard (Node — đã có v20+)
cd .opencode && npm install && cd ..
```

## Trạng thái

- [x] **B0/B1:** Repo mới + cấu hình opencode 4 tầng (commit đầu)
- [ ] B2–B5: `firmware/shared` contract + scaffold 2 firmware + wire ESP-NOW
- [ ] B7: CI build/test/hygiene
- [ ] B9: Nghiệm thu cloud CoreIoT (cách b — token mới)