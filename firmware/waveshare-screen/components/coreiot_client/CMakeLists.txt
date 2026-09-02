idf_component_register(
    SRCS "coreiot_client.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_event esp_netif nvs_flash mqtt esp_timer
)