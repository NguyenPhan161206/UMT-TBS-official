# TEST_PROTOCOL_DOCS_LOG — Tạo tài liệu test & trạng thái

## Mục tiêu
Bổ sung 2 tài liệu còn thiếu trong ROADMAP_CHECKLIST (DOC-A `TEST_PROTOCOL.md`,
DOC-B `PROGRESS.md`) và đồng bộ link trong README, trạng thái
`ROADMAP_CHECKLIST.md`, đảm bảo DoD đo được (R10).

> Bước B6 (sau): bổ sung hướng dẫn **tạo token CoreIoT** từng bước (đường dẫn UI
> chính xác: `/devices` chứ không phải `/home`) vào README + TEST_PROTOCOL; không
> ghi token thật (R1/R11). Xem phần "Vòng 2 — B6" bên dưới.

## File đã sửa / tạo
- `docs/TEST_PROTOCOL.md` — quy trình test 3 cấp:
  - Cấp 1: host/unit không cần board (`scan_secrets.py`,
    `gen_credentials.py --check`, `check_rulechain_thresholds.py`,
    `pytest test_guard.py` 16 tests, `pio test -e native` 10/10).
  - Cấp 2: flash-and-observe (2 board, mốc ESP-NOW LINKED / 500ms send / 1500ms
    timeout, đổi zone theo khoảng cách, buzzer).
  - Cấp 3: nghiệm thu CoreIoT B9 (tạo token MỚI R11 → `config/keys.json` →
    `gen_credentials.py` → `test_mqtt_coreiot.py` → dashboard `warning_status`).
- `docs/PROGRESS.md` — bảng trạng thái B0..T5.x + nhóm việc chờ user/đang làm.
- `README.md` — link 2 tài liệu mới vào mục "Kiểm thử cục bộ" + "Báo cáo & tài liệu".
- `.opencode/docs/ROADMAP_CHECKLIST.md` — đánh dấu XONG cho B0/B1/B6e/B9a;
  tách B9a/B9b; thêm bảng DOC-A/DOC-B.

## Kết quả kiểm thử / verify
- `docs/TEST_PROTOCOL.md`, `docs/PROGRESS.md`, `README.md`,
  `ROADMAP_CHECKLIST.md` đều < 400 dòng (R7).
- `python3 tools/guard/scan_secrets.py` → SECRET-SCAN OK (không literal secret;
  tài liệu chỉ hướng dẫn).
- Gitleaks: no leaks.
- CI trước đó (run `1560c25`) xanh; dự kiến CI cho commit docs này xanh.

## Hướng dẫn vận hành / demo
- Xem quy trình test từng cấp tại `docs/TEST_PROTOCOL.md`.
- Theo dõi trạng thái tổng tại `docs/PROGRESS.md`.
- Bước tiếp theo (B9b): user tạo token CoreIoT MỚI → điền `config/keys.json` →
  `gen_credentials.py --check` → flash 2 board → chạy `test_mqtt_coreiot.py
  --loop --interval 2` → kiểm tra `warning_status` trên dashboard.

## Vòng 2 — B6: hướng dẫn tạo token CoreIoT
- Mục tiêu: cập nhật cả README + TEST_PROTOCOL để user tạo token/device đúng nơi
  (`/devices`, **không** phải `/home`).
- File sửa:
  - `README.md` (mục "Nghiệm thu CoreIoT B9"): link chi tiết sang TEST_PROTOCOL
    cấp 3 + tóm tắt 5 bước tạo token.
  - `docs/TEST_PROTOCOL.md` (mục 3.1): mở rộng thành 7 bước kèm đường dẫn UI
    (`/devices`, `/ruleChains`), sinh `credentials.h`, import rule-chain.
- Verify: `scan_secrets.py` exit 0; gitleaks no leaks; R7 (README 222d,
  TEST_PROTOCOL 149d < 400); link `docs/TEST_PROTOCOL.md` tồn tại.
- Trạng thái B9b: vẫn ⏳ chờ user điền token MỚI vào `config/keys.json` + board.
