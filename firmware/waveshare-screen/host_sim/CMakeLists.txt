cmake_minimum_required(VERSION 3.16)
project(umt_host_sim LANGUAGES C)

# ─────────────────────────────────────────────────────────────────────────────
# host_sim — G1 T1.3/T1.2
#
# Ở step 1 (T1.3): chỉ mang target `hazard_core_tests` — mini assert-runner C
# thuần (không Unity, không LVGL/SDL) để kiểm chứng hazard_core trên host.
#
# Bước 3 (T1.2) sẽ MỞ RỘNG file này: FetchContent LVGL v9.1.0 + SDL2, thêm
# target `umt_dash_sim` (800x480, --scenario/--replay/--exit-after) compile các
# file UI thật qua stub mỏng. Kiến trúc: docs/ARCHITECTURE_G1_TESTING.md.
# ─────────────────────────────────────────────────────────────────────────────

set(HAZARD_CORE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../components/hazard_core/hazard_core.c")
set(HAZARD_CORE_INC "${CMAKE_CURRENT_SOURCE_DIR}/../components/hazard_core/include")
set(SHARED_INC "${CMAKE_CURRENT_SOURCE_DIR}/../../shared")

add_executable(hazard_core_tests
    "${HAZARD_CORE_SRC}"
    "tests/test_hazard_core.c"
)
target_include_directories(hazard_core_tests PRIVATE
    "${HAZARD_CORE_INC}"
    "${SHARED_INC}"
)
target_compile_features(hazard_core_tests PRIVATE c_std_11)

enable_testing()
add_test(NAME hazard_core_tests COMMAND hazard_core_tests)