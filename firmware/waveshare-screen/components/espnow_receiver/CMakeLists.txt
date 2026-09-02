idf_component_register(
    SRCS "espnow_receiver.c"
    INCLUDE_DIRS "include" "../../../shared"
    REQUIRES esp_wifi esp_timer
)