# voice-alert — Orchestration State
Roadmap: docs/roadmaps/voice-alert.roadmap.json
Handoff: docs/handoff/ANTIGRAVITY_VOICE_ALERT_HANDOFF.md
Updated: 2026-10-03

| Step | Title | Status | Verified by | Notes |
|------|-------|--------|-------------|-------|
| 1 | tools/voice: WAV → mảng C | TODO | — | |
| 2 | Tài sản placeholder + mảng C | TODO | — | giọng thật thay sau |
| 3 | voice_policy + test native | TODO | — | |
| 4 | sharedStateGetNearestIndex + sensor_slot_map.h | TODO | — | độc lập, làm song song với 1–3 được |
| 5 | voice_player (I2S) | TODO | — | cần chốt chân I2S |
| 6 | voice_alert task + cờ USE_VOICE_ALERT | TODO | — | |
| 7 | Tài liệu phần cứng + log + CHECKLIST | TODO | — | |
| 8 | Nghiệm thu phần cứng | BLOCKED | — | chưa có loa/amp |

## Contracts established
(điền sau mỗi step — chữ ký chính xác nằm trong handoff, mục 3)

## Deviations from plan
- Người dùng chưa có phần cứng âm thanh → step 7 viết tài liệu chọn linh kiện; step 8 BLOCKED tới khi có loa + amp.
- Chưa có file giọng thật → dùng WAV placeholder (âm hiệu sin), thay sau bằng TTS/thu âm.
