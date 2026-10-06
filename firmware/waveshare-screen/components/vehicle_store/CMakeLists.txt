idf_component_register(
    SRCS "vehicle_store_nvs.c"
    INCLUDE_DIRS "include"
    REQUIRES vehicle_profile nvs_flash
)
