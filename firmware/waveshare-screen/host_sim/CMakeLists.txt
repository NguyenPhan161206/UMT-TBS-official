cmake_minimum_required(VERSION 3.16)
project(umt_host_sim LANGUAGES C CXX)

# ─────────────────────────────────────────────────────────────────────────────
# host_sim — G1 T1.3/T1.2
#
#  - target `hazard_core_tests` : mini assert-runner C thuần (step 1) — không
#    phụ thuộc LVGL/SDL, chạy trong CI không cần display.
#  - target `umt_dash_sim`      : LVGL v9.1.0 (FetchContent, khớp
#    src/idf_component.yml "lvgl/lvgl: ^9.1.0") + SDL2, compile các file UI THẬT
#    (ui_dashboard.c/layout.c/system.c, sensor_model.c, hazard_core.c) qua stub
#    mỏng (stub/). CLI: --scenario/--replay/--exit-after.
#
# Scenario data: host_sim/gen_scenarios.py đọc tools/scenarios.py (nguồn duy nhất,
# step 2) và SINH ra C header `scenarios_gen.h` tại build dir — C executable
# không thể import Python. Kiến trúc: docs/ARCHITECTURE_G1_TESTING.md.
# ─────────────────────────────────────────────────────────────────────────────

set(HAZARD_CORE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../components/hazard_core/hazard_core.c")
set(HAZARD_CORE_INC "${CMAKE_CURRENT_SOURCE_DIR}/../components/hazard_core/include")
set(SHARED_INC "${CMAKE_CURRENT_SOURCE_DIR}/../../shared")
set(UI_DASH_INC "${CMAKE_CURRENT_SOURCE_DIR}/../components/ui_dashboard/include")
set(UI_DASH_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../components/ui_dashboard")
set(SENSOR_MODEL_INC "${CMAKE_CURRENT_SOURCE_DIR}/../components/sensor_model/include")
set(SENSOR_MODEL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../components/sensor_model/sensor_model.c")

# ---------------------------------------------------------------------------- #
# hazard_core_tests (T1.3) — C thuần, không phụ thuộc môi trường
# ---------------------------------------------------------------------------- #
add_executable(hazard_core_tests
    "${HAZARD_CORE_SRC}"
    "tests/test_hazard_core.c"
)
target_include_directories(hazard_core_tests PRIVATE
    "${HAZARD_CORE_INC}"
    "${SHARED_INC}"
)
target_compile_features(hazard_core_tests PRIVATE c_std_11)

# ---------------------------------------------------------------------------- #
# umt_dash_sim (T1.2) — LVGL v9.1.0 + SDL2, compile file UI thật
# ---------------------------------------------------------------------------- #
include(FetchContent)

# Scenario generator: đọc tools/scenarios.py, xuất C header.
find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(SCENARIOS_GEN_H "${CMAKE_CURRENT_BINARY_DIR}/generated/scenarios_gen.h")
add_custom_command(
    OUTPUT "${SCENARIOS_GEN_H}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/gen_scenarios.py"
            --scenarios "${CMAKE_CURRENT_SOURCE_DIR}/../../../tools/scenarios.py"
            --out "${SCENARIOS_GEN_H}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../../../tools/scenarios.py"
    COMMENT "Generating C scenario data from tools/scenarios.py"
    VERBATIM
)

FetchContent_Declare(
    lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG        v9.1.0
    GIT_SHALLOW    TRUE
)
set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE FILEPATH "" FORCE)
FetchContent_MakeAvailable(lvgl)

find_package(SDL2 REQUIRED)

add_executable(umt_dash_sim
    "main.c"
    "${HAZARD_CORE_SRC}"
    "${SENSOR_MODEL_SRC}"
    "${UI_DASH_SRC_DIR}/ui_dashboard.c"
    "${UI_DASH_SRC_DIR}/ui_dashboard_layout.c"
    "${UI_DASH_SRC_DIR}/ui_dashboard_system.c"
    "${SCENARIOS_GEN_H}"   # chỉ để CMake sinh trước khi build
)
target_include_directories(umt_dash_sim PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"          # lv_conf.h
    "${CMAKE_CURRENT_BINARY_DIR}/generated" # scenarios_gen.h
    "${CMAKE_CURRENT_SOURCE_DIR}/stub"     # freertos/, esp_*.h, coreiot_client.h (giả)
    "${UI_DASH_INC}"
    "${UI_DASH_SRC_DIR}"
    "${SENSOR_MODEL_INC}"
    "${HAZARD_CORE_INC}"
    "${SHARED_INC}"
)
target_link_libraries(umt_dash_sim PRIVATE lvgl SDL2::SDL2)
target_compile_features(umt_dash_sim PRIVATE c_std_11)
if(NOT WIN32)
    target_link_libraries(umt_dash_sim PRIVATE m)
endif()

# ---------------------------------------------------------------------------- #
# tests
# ---------------------------------------------------------------------------- #
enable_testing()
add_test(NAME hazard_core_tests COMMAND hazard_core_tests)

# headless render test: SDL dummy driver hoạt động không cần X (xvfb không bắt
# buộc ở local; CI có thể dùng xvfb-run để đúng DoD). DoD yêu cầu exit 0 sau
# --exit-after 3 --scenario approach.
add_test(NAME umt_dash_sim_render COMMAND umt_dash_sim
    --scenario approach --exit-after 2 --interval 200)
set_tests_properties(umt_dash_sim_render PROPERTIES
    ENVIRONMENT "SDL_VIDEODRIVER=dummy")