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
set(VEHICLE_PROFILE_INC "${CMAKE_CURRENT_SOURCE_DIR}/../components/vehicle_profile/include")
# GLOB để thêm file vehicle_*.c / ui_dashboard*.c mới không phải sửa CMake.
file(GLOB VEHICLE_PROFILE_SRCS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/../components/vehicle_profile/vehicle_*.c")
file(GLOB UI_DASH_SRCS CONFIGURE_DEPENDS
    "${UI_DASH_SRC_DIR}/ui_dashboard*.c")

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
# vehicle_layout_tests (T3.2/T3.1) — C thuần, không phụ thuộc môi trường
# ---------------------------------------------------------------------------- #
add_executable(vehicle_layout_tests
    ${VEHICLE_PROFILE_SRCS}
    "tests/test_vehicle_layout.c"
)
target_include_directories(vehicle_layout_tests PRIVATE
    "${VEHICLE_PROFILE_INC}"
    "${SHARED_INC}"
)
target_compile_features(vehicle_layout_tests PRIVATE c_std_11)
if(NOT WIN32)
    target_link_libraries(vehicle_layout_tests PRIVATE m)
endif()

# ---------------------------------------------------------------------------- #
# vehicle_settings_tests (T4.1b) — C thuần: codec blob NVS, resolve hồ sơ + override
# ---------------------------------------------------------------------------- #
add_executable(vehicle_settings_tests
    ${VEHICLE_PROFILE_SRCS}
    "tests/test_vehicle_settings.c"
)
target_include_directories(vehicle_settings_tests PRIVATE
    "${VEHICLE_PROFILE_INC}"
    "${SHARED_INC}"
)
target_compile_features(vehicle_settings_tests PRIVATE c_std_11)
if(NOT WIN32)
    target_link_libraries(vehicle_settings_tests PRIVATE m)
endif()

# vehicle_override_tests (T4.1c) — ghi đè từng cảm biến (dùng chung store giả với vehicle_settings_tests)
add_executable(vehicle_override_tests
    ${VEHICLE_PROFILE_SRCS}
    "tests/test_vehicle_override.c"
)
target_include_directories(vehicle_override_tests PRIVATE
    "${VEHICLE_PROFILE_INC}"
    "${SHARED_INC}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests"
)
target_compile_features(vehicle_override_tests PRIVATE c_std_11)
if(NOT WIN32)
    target_link_libraries(vehicle_override_tests PRIVATE m)
endif()
target_include_directories(vehicle_settings_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")

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
# T2.3: hazard_core_tests chạy heuristic xe cắt ngang trên chính dữ liệu kịch bản (cần file sinh ra).
target_sources(hazard_core_tests PRIVATE "${SCENARIOS_GEN_H}")
target_include_directories(hazard_core_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../managed_components/lvgl__lvgl/CMakeLists.txt")
    FetchContent_Declare(
        lvgl
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../managed_components/lvgl__lvgl"
    )
else()
    FetchContent_Declare(
        lvgl
        GIT_REPOSITORY https://github.com/lvgl/lvgl.git
        GIT_TAG        v9.1.0
        GIT_SHALLOW    TRUE
    )
endif()
set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE FILEPATH "" FORCE)
FetchContent_MakeAvailable(lvgl)

if(WIN32)
    FetchContent_Declare(
        SDL2_prebuilt
        URL https://github.com/libsdl-org/SDL/releases/download/release-2.30.10/SDL2-devel-2.30.10-mingw.zip
    )
    FetchContent_MakeAvailable(SDL2_prebuilt)
    set(SDL2_ROOT "${sdl2_prebuilt_SOURCE_DIR}/x86_64-w64-mingw32")
    file(READ "${SDL2_ROOT}/lib/cmake/SDL2/sdl2-config.cmake" _sdl2_cfg)
    string(REPLACE "/tmp/tardir/SDL2-2.30.10/build-mingw/install-x86_64-w64-mingw32" "${SDL2_ROOT}" _sdl2_cfg "${_sdl2_cfg}")
    file(WRITE "${SDL2_ROOT}/lib/cmake/SDL2/sdl2-config.cmake" "${_sdl2_cfg}")
    list(PREPEND CMAKE_PREFIX_PATH "${SDL2_ROOT}")
    target_include_directories(lvgl PRIVATE "${SDL2_ROOT}/include")
endif()

find_package(SDL2 REQUIRED)

add_executable(umt_dash_sim
    "main.c"
    "sim_tools.c"
    "${HAZARD_CORE_SRC}"
    "${SENSOR_MODEL_SRC}"
    ${UI_DASH_SRCS}
    ${VEHICLE_PROFILE_SRCS}
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
    "${VEHICLE_PROFILE_INC}"
    "${SHARED_INC}"
)
if(WIN32)
    # MinGW + SDL2 prebuilt: cần SDL2main (WinMain) đứng TRƯỚC SDL2 và thư mục include riêng.
    target_include_directories(umt_dash_sim PRIVATE "${SDL2_ROOT}/include")
    target_link_libraries(umt_dash_sim PRIVATE lvgl SDL2::SDL2main SDL2::SDL2)
else()
    # Linux/CI: giữ nguyên cách link đã chạy ổn từ trước (không SDL2main, không SDL2_ROOT).
    target_link_libraries(umt_dash_sim PRIVATE lvgl SDL2::SDL2)
endif()
target_compile_features(umt_dash_sim PRIVATE c_std_11)
if(NOT WIN32)
    target_link_libraries(umt_dash_sim PRIVATE m)
else()
    target_link_options(umt_dash_sim PRIVATE -mconsole)
    add_custom_command(TARGET umt_dash_sim POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${sdl2_prebuilt_SOURCE_DIR}/x86_64-w64-mingw32/bin/SDL2.dll"
        $<TARGET_FILE_DIR:umt_dash_sim>
    )
endif()

# ---------------------------------------------------------------------------- #
# tests
# ---------------------------------------------------------------------------- #
enable_testing()
add_test(NAME hazard_core_tests COMMAND hazard_core_tests)
add_test(NAME vehicle_layout_tests COMMAND vehicle_layout_tests)
add_test(NAME vehicle_settings_tests COMMAND vehicle_settings_tests)
add_test(NAME vehicle_override_tests COMMAND vehicle_override_tests)

# headless render test: SDL dummy driver hoạt động không cần X (xvfb không bắt
# buộc ở local; CI có thể dùng xvfb-run để đúng DoD). DoD yêu cầu exit 0 sau
# --exit-after 3 --scenario approach.
add_test(NAME umt_dash_sim_render COMMAND umt_dash_sim
    --scenario approach --exit-after 2 --interval 200)
# T4.1b: đổi hồ sơ xe + dựng lại sơ đồ 120 lần — FAIL nếu số widget trôi hoặc heap LVGL rò > 1 KB.
add_test(NAME umt_dash_sim_profile_stress COMMAND umt_dash_sim --stress-profiles 120)
# T4.1b/c: kịch bản UI bằng chuột ảo — mở tab SETUP, chọn FRONT, lùi 200 mm, Apply, về COLLISION, Reset;
# FAIL nếu sim treo/crash hoặc không lưu được ảnh chụp (xem ảnh ở ${CMAKE_BINARY_DIR}/ui_setup_*.bmp).
add_test(NAME umt_dash_sim_setup_ui COMMAND umt_dash_sim --scenario approach --interval 40 --exit-after 8
    --click 447,20@2000 --click 460,89@2400 --click 540,200@2800 --click 540,200@3100 --click 540,200@3400
    --click 540,200@3700 --click 460,305@4100 --snapshot "${CMAKE_BINARY_DIR}/ui_setup_applied.bmp@4600"
    --click 259,20@5000 --snapshot "${CMAKE_BINARY_DIR}/ui_collision_overridden.bmp@5700"
    --click 447,20@6000 --click 584,305@6400 --snapshot "${CMAKE_BINARY_DIR}/ui_setup_reset.bmp@6900")
# T2.3: xe cắt ngang phải hiện banner rồi tự tắt sau CROSSING_HOLD_MS (3000) dù không còn khung mới;
# kịch bản normal không bao giờ hiện. (khung 100 ms: các mốc kết thúc ~400 ms, lần kích hoạt cuối ~200 ms)
add_test(NAME umt_dash_sim_crossing_banner COMMAND umt_dash_sim --scenario crossing --interval 100 --exit-after 6
    --expect-text "CROSSING TRAFFIC HAZARD@450" --expect-text "S3 (L-Front)@450"
    --expect-no-text "CROSSING TRAFFIC HAZARD@4500" --expect-text "None detected@4500")
add_test(NAME umt_dash_sim_normal_no_crossing COMMAND umt_dash_sim --scenario normal --interval 100 --exit-after 3
    --expect-no-text "CROSSING TRAFFIC HAZARD@300" --expect-no-text "CROSSING TRAFFIC HAZARD@2500"
    --expect-text "OVERALL: SAFE@2500")
# T5.2: nút Calibrate (thanh trái, toạ độ 90,335) mở trang chỉnh cảm biến.
add_test(NAME umt_dash_sim_calibrate_button COMMAND umt_dash_sim --scenario normal --interval 40 --exit-after 4
    --expect-no-text "SENSOR POSITION@1500" --click 90,335@2000 --expect-text "SENSOR POSITION@2800")
# Hết pool LVGL (LV_ASSERT_MALLOC) là vòng lặp vô hạn: có TIMEOUT thì test FAIL thay vì treo CI.
set_tests_properties(umt_dash_sim_render umt_dash_sim_profile_stress umt_dash_sim_setup_ui
    umt_dash_sim_crossing_banner umt_dash_sim_normal_no_crossing umt_dash_sim_calibrate_button
    PROPERTIES TIMEOUT 60)
# replay (T1.4 -> T1.2): hai định dạng — payload V2 (d1..d6) và file của tools/recorder (distances[] + valid[]).
# 'skip invalid' trên stderr nghĩa là sim không hiểu dòng đó -> test phải FAIL.
set(REPLAY_FIXTURES "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
add_test(NAME umt_dash_sim_replay_v2 COMMAND umt_dash_sim
    --replay "${REPLAY_FIXTURES}/replay_v2.jsonl" --interval 50)
add_test(NAME umt_dash_sim_replay_recorder COMMAND umt_dash_sim
    --replay "${REPLAY_FIXTURES}/replay_recorder.jsonl")
# valid=0 phải thành "không dữ liệu" (in d3=--), không được đưa số 0 cm vào UI.
add_test(NAME umt_dash_sim_replay_recorder_invalid_slots COMMAND umt_dash_sim
    --replay "${REPLAY_FIXTURES}/replay_recorder.jsonl")
set_tests_properties(umt_dash_sim_replay_v2 PROPERTIES
    PASS_REGULAR_EXPRESSION "fed 3 row\\(s\\)" FAIL_REGULAR_EXPRESSION "skip invalid")
set_tests_properties(umt_dash_sim_replay_recorder PROPERTIES
    PASS_REGULAR_EXPRESSION "fed 5 row\\(s\\)" FAIL_REGULAR_EXPRESSION "skip invalid")
set_tests_properties(umt_dash_sim_replay_recorder_invalid_slots PROPERTIES
    PASS_REGULAR_EXPRESSION "d3=--" FAIL_REGULAR_EXPRESSION "skip invalid")

if(NOT WIN32)
    set_tests_properties(umt_dash_sim_render umt_dash_sim_replay_v2 umt_dash_sim_replay_recorder
        umt_dash_sim_replay_recorder_invalid_slots PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy")
endif()
