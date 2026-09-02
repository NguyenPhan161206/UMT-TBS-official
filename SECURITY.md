# SECURITY.md

## Chính sách Secret (R1)

Repo này chứa firmware ESP32-S3 kết nối cloud **CoreIoT** và Wi-Fi. Tuân thủ tuyệt đối:

1. **Credential chỉ nằm ở `config/keys.json`** (gitignored) hoặc file sinh từ nó
   (`credentials.h` — gitignored, sinh bởi `tools/guard/gen_credentials.py`).
2. **Token đã lộ là token chết.** Mọi token từng xuất hiện trong lịch sử Git của repo cũ
   (`supersonic-warning-system`) phải được vô hiệu trên CoreIoT console và **không tái sử dụng**.
3. **KHÔNG hardcode** token/password trong mã nguồn C/C++/Python hoặc file markdown tracked.
   Vi phạm được phát hiện bởi:
   - `tools/guard/scan_secrets.py` (chạy thủ công / `/verify`)
   - `.opencode/plugin/guard.ts` (chặn lệnh nguy hiểm + cảnh báo)
   - CI (tương lai, Giai đoạn B7): Gitleaks + assert `keys.json` gitignored.

## Báo cáo vi phạm
- Mở issue (nếu repo public) hoặc nhắn trực tiếp maintainer.
- Kèm: file vi phạm, pattern bị phát hiện, mức độ (token cloud / Wi-Fi / khác).
- Không đăng token lên issue/PR công khai.

## Token CoreIoT — Quy trình đúng (cách b)
1. Tạo tài khoản miễn phí tại `app.coreiot.io` trên tài khoản **của chủ dự án**.
2. Tạo 2 device `sensor-node`, `waveshare-screen` → copy access token mới.
3. Ghi vào `config/keys.json` (gitignored). Import rule-chain từ `cloud/coreiot/rule_chain/`.
4. Xác minh bằng `tools/test_mqtt_coreiot.py` (không commit token).

## Môi trường local
- `config/keys.json` nên có quyền `600` (owner-only): `chmod 600 config/keys.json`.
- Không đẩy `AGENTS.md` (bản sao local của template) lên git — đã ignore.