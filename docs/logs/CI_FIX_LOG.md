# CI Fix Log — B7 (pipeline đỏ lần chạy đầu)

**Ngày:** 2026-09-02
**Task:** Sửa CI `failure` run `33590604857` sau commit `b4372f5` (workflow B7).
**Trạng thái:** ✅ Đã fix + verify local; chờ GitHub Actions xanh sau push.

## Vấn đề 1 — Build waveshare-screen fail: version drift toolchain
**Hiện tượng (CI):** `AttributeError: 'tuple' object has no attribute 'startswith'`
tại `espidf.py:798 _is_cpp_only(flag)` (platform `espressif32@7.1.0`,
`framework-espidf@4.60100.0` = IDF 6.1.0).
**Nguyên nhân:** `platformio.ini` không pin version → CI resolve lên **platform 7.1.0 +
IDF 6.1.0** (mới nhất). Máy local build OK với **7.0.1 + IDF 6.0.1**
(`framework-espidf@4.60001.0`). Builder 7.1.0 vỡ khi gặp build flag dạng tuple từ
managed component (IDF 6.1.0 / component-manager 3.1.1).
**Không dùng được** `framework = espidf@4.60100.0` (bị chặn "This board doesn't
support espidf@4.60100.0 framework!") → chỉ cần pin **platform**:
```ini
platform = espressif32@7.0.1   ; platform tự kéo framework-espidf @ ~4.60001.0 = IDF 6.0.1
```
**Đã sửa:** `firmware/waveshare-screen/platformio.ini` + `firmware/sensor-node/platformio.ini`
(pin `platform = espressif32@7.0.1`, 2 env sensor + 1 env screen).

## Vấn đề 2 — Gitleaks fail: 7 false positives trên giá trị dummy
**Hiện tượng (CI):** `WRN leaks found: 7` — nhưng là **toàn bộ giả**:
`.github/workflows/ci.yml` + `tools/guard/test_guard.py` chứa `ci-dummy-*` (token/password
CI chỉ để build) + fixture test `ABCdef1234567890`.
**Đã sửa:** `.gitleaks.toml` thêm allowlist:
- `ci-dummy-[A-Za-z0-9-]+` (giá trị CI/test)
- `ABCdef1234567890` (fixture quá khứ trong **history commit b4372f5** — gitleaks quét
  full history nên entry bắt buộc phải giữ)

## Vấn đề 3 — scan_secrets tự hít fixture của nó
**Hiện tượng:** `scan_secrets.py` repo-wide flag `COREIOT_SENSOR_NODE_DEVICE_TOKEN =
"ABCdef1234567890"` **ngay trong chính test_guard.py** (offset: code của scanner).
**Đã sửa:** `tools/guard/test_guard.py` nối chuỗi lúc runtime
`token = "ABCdef" + "1234567890"` — source không còn literal 16+ ký tự, file tạm lúc
chạy vẫn chứa token đầy đủ nên test vẫn kiểm chứng đúng hành vi scanner.

## Sự cố phụ — đĩa đầy khi reproduce lỗi CI
Nhân tiện track: thử cài `espressif32@7.1.0` + IDF 6.1.0 local để reproduce làm đĩa
100% (`framework-espidf@4.60001.0/.piopm` bị đứt 0 byte, download hỏng dở).
**Đã dọn:** `pio pkg uninstall -p espressif32` (7.1.0), `pio system prune -f` (cache
2.4G), xoá gói `framework-espidf` (không version) dở dang, xoá platform default
`espressif32` (7.1.0). Đĩa: 100% → ~2.5G trống. `.piopm` thiếu không chặn `pio run`.

## Kết quả kiểm thử (cục bộ)
| Lệnh | Kết quả |
|---|---|
| `pio run -e yolo_uno` (waveshare-screen, pin 7.0.1) | ✅ SUCCESS — RAM 12.8% / Flash 31.3% |
| `pio run -e yolo_uno` (sensor-node, pin 7.0.1) | ✅ SUCCESS |
| `gitleaks detect --config .gitleaks.toml --exit-code 1` | ✅ no leaks found (exit 0) |
| `python3 tools/guard/scan_secrets.py` | ✅ SECRET-SCAN OK (exit 0) |
| `pytest tools/guard/test_guard.py` | ✅ 11 passed |

## Ghi chú
- Pin platform 7.0.1 giúp **tái lập build** (R10): CI == máy local (IDF 6.0.1).
- Nếu mai này muốn nâng IDF lên 6.1: cần verify builder espressif32@7.1.0 xử lý tuple
  flag trước, rồi nâng pin + sửa nếu cần.
- `config/keys.json` local vẫn là dummy (`test-build-*`) — chưa đụng.

---

# CI Fix — 2026-10-11 (main đỏ từ b090062, 21/09)

## Vấn đề 3 — Build sensor-node fail sau merge main → khoa
**Hiện tượng (CI, run PR d3c9756):** `#endif without #if` ở `firmware/sensor-node/src/main.cpp`.
**Nguyên nhân:** resolve conflict tay giữ phần khoa (`slotCm`), bỏ `#if TBS_ALGO_TEST / payload[512] / #else`
nhưng `#endif` nằm ngoài vùng conflict nên còn sót → đóng sớm `#if USE_COREIOT`.
**Đã sửa (1d122f2):** khôi phục `#if TBS_ALGO_TEST char payload[512] #else char payload[256] #endif`.
**Verify local:** `yolo_uno`, `yolo_uno_coreiot`, `yolo_uno_coreiot_algotest` SUCCESS; `pio test -e native` 19/19.
CI main run 38112784603: job firmware xanh.

## Vấn đề 4 — Gitleaks: 2 device token CoreIoT THẬT trong history
**Hiện tượng:** `leaks found: 2` (CI không in chi tiết; chạy gitleaks 8.24.0 local với `--redact -v`).
**Nguyên nhân:** commit `57fc19e` (25/09) dán khối `config/keys.json` (2 device token + SSID/mật khẩu Wi-Fi)
vào `docs/DATASET_RECORDING_GUIDE.md` dòng 40–41; `9bba9c2` (03/10) xoá khỏi file nhưng vẫn nằm trong
history, repo **public**.
**Xử lý:** chủ dự án xoá/thu hồi 2 token trên CoreIoT (11/10). Thêm `.gitleaksignore` với đúng 2 fingerprint
(không allowlist rộng). Không viết lại history (secret đã public, force-push phá lịch sử nhóm).
**Verify local:** `gitleaks detect --source . --config .gitleaks.toml` → `no leaks found` (91 commit);
`scan_secrets.py` OK.
**Còn lại:** đổi mật khẩu Wi-Fi bị lộ cùng chỗ (gitleaks không có rule cho nó). Tài liệu không bao giờ dán
giá trị thật từ `keys.json` — dùng `config/keys.template.json`.
