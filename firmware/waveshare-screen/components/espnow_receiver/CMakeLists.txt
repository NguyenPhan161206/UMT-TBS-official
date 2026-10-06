idf_component_register(
    SRCS "espnow_receiver.c"
    INCLUDE_DIRS "include" "../../../shared"
    REQUIRES esp_wifi esp_timer
)

# Env yolo_uno_latency truyền -DTBS_LATENCY_PROBE=1 qua board_build.cmake_extra_args (DMXT-57).
if(TBS_LATENCY_PROBE)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE TBS_LATENCY_PROBE=1)
endif()