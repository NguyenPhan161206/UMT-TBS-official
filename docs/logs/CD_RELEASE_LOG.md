# CD RELEASE — IMPLEMENTATION LOG

## Mục tiêu
Đưa **Đề xuất 1 CD** vào hoạt động: khi push tag `v*` → GitHub Actions tự build 3 env
(sensor-node yolo_uno, sensor-node yolo_uno_coreiot, waveshare-screen yolo_uno), size gate,
tạo checksum, upload firmware.bin + SHA256SUMS.txt lên GitHub Release — để ai cũng tải được
firmware mà không cần môi trường build (R8: release binary qua GitHub Release, không track .bin).

## File đã tạo/sửa
| File | Trạng thái |
|------|-----------|
| `docs/CD_RELEASE.md` | MỚI — decision record: trigger `v*`, naming artifact, sha256, permissions, không flash tự động, quy trình release tay |
| `.github/workflows/release.yml` | MỚI — job build (copy pattern ci.yml: dummy credentials R1, build 3 env, `pio test -e native`, size gate) + staging `dist/` + upload `softprops/action-gh-release@v2` |
| `docs/roadmaps/cd-release.roadmap.json` + `.state.md` | MỚI — kế hoạch 4 bước, ledger (step 1–3 DONE, step 4 DOC) |
| `docs/PROGRESS.md` | SỬA — hàng CD + lịch sử 2026-09-09 + sửa ngưỡng buzzer cũ (đã lỗi thời từ 9dcf877) |

## Kết quả kiểm thử (DoD)
- **Step 1** (contract): `grep -nE 'v\*|sha256|contents: write|firmware-(sensor-node|waveshare)'` → 4 mốc đạt.
- **Step 2** (workflow): `python3 -c "import yaml; yaml.safe_load(...)"` → **YAML OK**; grep mốc `tags/action-gh-release@v2/sha256sum/check_size.py/firmware.bin` đủ; `scan_secrets.py` → OK.
- **Step 3** (end-to-end): push `v0.1.0-preview` (commit `fbd3977`, `20038d4`) → run ID `34305027836` **success** (~23s trên log query đầu; checkout/setup hoàn tất). Release page có **4 asset**:
  - `firmware-sensor-node-yolo_uno-v0.1.0-preview.bin` (694800 B)
  - `firmware-sensor-node-yolo_uno_coreiot-v0.1.0-preview.bin` (717280 B)
  - `firmware-waveshare-screen-yolo_uno-v0.1.0-preview.bin` (1287616 B)
  - `SHA256SUMS.txt` (358 B)
  - Tải về `/tmp/cd_verify` → `sha256sum -c SHA256SUMS.txt` → **3/3 bin OK**.
- Ghi chú (không fail): annotation "Node.js 20 deprecated" cho checkout@v4/setup-python@v5/action-gh-release@v2 — chỉ cảnh báo.

## Hướng dẫn vận hành / demo
- **Tạo release mới**: `git tag v0.1.1 && git push origin v0.1.1` → đợi Actions xanh → GitHub → Releases → tải `.bin`; verify `sha256sum -c SHA256SUMS.txt`.
- **Flash firmware tải về**: `pio run -t upload --upload-port /dev/ttyACM0` (hoặc dùng esptool với `.bin`) — xem `docs/INSTALLATION_SENSOR_NODE.md`.
- **Xoá bản test**: `gh release delete v0.1.0-preview --yes && git push origin :refs/tags/v0.1.0-preview`.
- CD **không** tự flash board (vật lý, flash-and-observe tay theo AGENTS.md).

## Liên kết
- Contract CD: `docs/CD_RELEASE.md`
- Ledger/Roadmap: `docs/roadmaps/cd-release.state.md`, `docs/roadmaps/cd-release.roadmap.json`
- Workflow: `.github/workflows/release.yml`