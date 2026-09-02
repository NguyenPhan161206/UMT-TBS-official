idf_component_register(
    SRCS
        "ui_dashboard.c"
        "ui_dashboard_layout.c"
        "ui_dashboard_system.c"
    INCLUDE_DIRS "include" "../../../shared"
    REQUIRES lvgl sensor_model coreiot_client freertos esp_wifi esp_system
             esp_app_format esp_hw_support spi_flash
)