# CD Release — Orchestration State
Roadmap: docs/roadmaps/cd-release.roadmap.json
Created: 2026-09-09
Updated: 2026-09-09 (đã viết contract + workflow)

| Step | Title | Status | Verified by | Notes |
|------|-------|--------|-------------|-------|
| 1 | Contract CD + chuẩn artifact (docs/CD_RELEASE.md) | DONE | grep v*/sha256/contents write/naming | 4 mốc đạt |
| 2 | Tạo .github/workflows/release.yml | DONE | YAML OK + grep mốc + scan_secrets | softprops@v2, staging dist/, sha256sum |
| 3 | Thử nghiệm tag v0.1.0-preview | TODO | — | **cần user cho phép push tag remote** |
| 4 | Log vận hành CD + tài liệu | TODO | — | sẽ ghi sau khi tag chạy xong |

## Contracts established
- Trigger: `on: push: tags: ['v*']` → build + tạo/upload GitHub Release.
- Artifact naming (CI staging `dist/`, `<tag>` = `github.ref_name`):
  - `firmware-sensor-node-yolo_uno-<tag>.bin` ← `firmware/sensor-node/.pio/build/yolo_uno/firmware.bin`
  - `firmware-sensor-node-yolo_uno_coreiot-<tag>.bin` ← `firmware/sensor-node/.pio/build/yolo_uno_coreiot/firmware.bin`
  - `firmware-waveshare-screen-yolo_uno-<tag>.bin` ← `firmware/waveshare-screen/.pio/build/yolo_uno/firmware.bin`
  - `SHA256SUMS.txt` = `sha256sum *.bin` trong `dist/`
- Permissions: `contents: write` dùng `GITHUB_TOKEN`; upload bằng `softprops/action-gh-release@v2`; không literal secret (R1).
- Workflow đứng riêng `release.yml`; không flash board (vật lý).

## Deviations from plan
- Step 2 verification đã gia cố: thêm `python3 -c import yaml` safe_load (PyYAML có trên máy) — DoD trước chỉ grep, yếu (theo nhận xét "ID-CD phân bố").

## Context gốc (đừng suy lại mỗi lần)
- CI hiện tại: `.github/workflows/ci.yml` (job firmware + job security). Chưa có CD trước đây.
- Credential: mọi job tự sinh dummy `credentials.h` (pattern ci.yml) — R1.
- PlatformIO pin `platformio==6.1.19`, python 3.12.
- `gh` CLI 2.97 có sẵn trên máy; chưa có tag nào trên origin.
- Binary không track trong git (R8); GitHub Release là nơi phát hành chuẩn.