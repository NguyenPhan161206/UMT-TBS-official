idf_component_register(
    SRCS
        "ui_dashboard.c"
        "ui_dashboard_layout.c"
        "ui_dashboard_system.c"
        "ui_dashboard_truck.c"
        "ui_dashboard_marker.c"
    INCLUDE_DIRS "include" "../../../shared" "../hazard_core/include" "../vehicle_profile/include"
    REQUIRES lvgl sensor_model hazard_core vehicle_profile coreiot_client freertos esp_wifi esp_system
             esp_app_format esp_hw_support spi_flash
)