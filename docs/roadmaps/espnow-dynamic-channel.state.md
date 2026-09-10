# espnow-dynamic-channel — Orchestration State
Roadmap: docs/roadmaps/espnow-dynamic-channel.roadmap.json
State: ALL STEPS DONE (2026-09-10).

| Step | Title | Status | Verified by | Notes |
|------|-------|--------|-------------|-------|
| 1 | Sensor-node: sync ESP-NOW peer channel to live home channel | DONE | pio 2 env SUCCESS | grep esp_wifi_get_channel+mod_peer present |
| 2 | Document dynamic-channel contract + receiver log clarity | DONE | waveshare build SUCCESS; pytest 29 | — |

## Contracts established
- ESPNOW_CHANNEL (espnow_protocol.h) = default/fallback khi chưa associate AP.
- Khi STA nối AP: kênh thật do AP quyết; sensor-node sync peer qua
  esp_wifi_get_channel + esp_now_mod_peer trước mỗi send.

## Deviations from plan
- (none)