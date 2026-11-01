# seedgen: command-line randomizer seed generator (preset + seed -> spoiler JSON).
# Runs the game's real generation code with no window, ROM, or o2r files. See README.md.
# EXCLUDE_FROM_ALL: only built on request with `--target seedgen`.

set(SOH_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(RANDO_DIR ${SOH_DIR}/soh/Enhancements/randomizer)

file(GLOB_RECURSE SEEDGEN_REGION_SOURCES CONFIGURE_DEPENDS ${RANDO_DIR}/location_access/*.cpp)

set(SEEDGEN_RANDO_SOURCES
    3drando/fill.cpp
    3drando/hint_list.cpp
    3drando/hint_list/hint_list_exclude_dungeon.cpp
    3drando/hint_list/hint_list_exclude_overworld.cpp
    3drando/hint_list/hint_list_item.cpp
    3drando/hints.cpp
    3drando/item_pool.cpp
    3drando/menu.cpp
    3drando/playthrough.cpp
    3drando/shops.cpp
    3drando/spoiler_log.cpp
    3drando/starting_inventory.cpp
    RCToRandInf.cpp
    SeedContext.cpp
    ShuffleBeehives.cpp
    ShuffleBeggar.cpp
    ShuffleCows.cpp
    ShuffleCrates.cpp
    ShuffleFairies.cpp
    ShuffleFreestanding.cpp
    ShuffleGrass.cpp
    ShuffleHazardRooms.cpp
    ShuffleIcicles.cpp
    ShufflePots.cpp
    ShuffleRedIce.cpp
    ShuffleRocks.cpp
    ShuffleSigns.cpp
    ShuffleSilverRupees.cpp
    ShuffleSongs.cpp
    ShuffleTrees.cpp
    ShuffleWonderItems.cpp
    Traps.cpp
    dungeon.cpp
    entrance.cpp
    fishsanity.cpp
    hint.cpp
    item.cpp
    item_list.cpp
    item_location.cpp
    item_override.cpp
    location.cpp
    location_access.cpp
    location_list.cpp
    logic.cpp
    option.cpp
    option_descriptions.cpp
    randomizer_check_objects.cpp
    randomizer_entrance_tracker.cpp # for the entranceData table written to the spoiler log
    settings.cpp
    static_data.cpp
    trial.cpp
)
list(TRANSFORM SEEDGEN_RANDO_SOURCES PREPEND ${RANDO_DIR}/)

add_executable(seedgen EXCLUDE_FROM_ALL
    seedgen.cpp
    game_stubs.cpp
    ${SEEDGEN_RANDO_SOURCES}
    ${SEEDGEN_REGION_SOURCES}
    ${SOH_DIR}/soh/Enhancements/custom-message/CustomMessageManager.cpp
    ${SOH_DIR}/soh/Enhancements/custom-message/text.cpp
    ${SOH_DIR}/soh/Enhancements/debugger/performanceTimer.cpp
    ${SOH_DIR}/soh/ObjectExtension/ObjectExtension.cpp
    ${SOH_DIR}/soh/ShipUtils.cpp
    ${SOH_DIR}/soh/util.cpp
    ${SOH_DIR}/src/boot/build.c
    ${SOH_DIR}/src/code/z_inventory.c
)

# Same flags as the game, so generation can't drift from it. spdlog and fmt are the exception:
# header-only here, so the binary doesn't depend on those shared libraries at runtime.
target_include_directories(seedgen PRIVATE ${SOH_DIR}/soh $<TARGET_PROPERTY:soh,INCLUDE_DIRECTORIES>)
target_compile_definitions(seedgen PRIVATE
    "$<FILTER:$<TARGET_PROPERTY:soh,COMPILE_DEFINITIONS>,EXCLUDE,^(SPDLOG_COMPILED_LIB|SPDLOG_SHARED_LIB|FMT_SHARED)$>"
    SPDLOG_HEADER_ONLY
    FMT_HEADER_ONLY
)
target_compile_options(seedgen PRIVATE
    $<TARGET_PROPERTY:soh,COMPILE_OPTIONS>
    -ffunction-sections
    -fdata-sections
)

# The English text is embedded, so the binary is a single self-contained file.
set(SEEDGEN_LANG_FILE ${SOH_DIR}/assets/custom/lang/en_US.json)
set_source_files_properties(seedgen.cpp PROPERTIES
    COMPILE_DEFINITIONS "SEEDGEN_LANG_FILE=\"${SEEDGEN_LANG_FILE}\""
    OBJECT_DEPENDS ${SEEDGEN_LANG_FILE}
)

# --gc-sections drops the gameplay and GUI code that shares files with generation.
target_link_options(seedgen PRIVATE -Wl,--gc-sections)

# Fully static when the toolchain has a static libc, so it runs in any container.
execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libc.a
    OUTPUT_VARIABLE SEEDGEN_LIBC_A OUTPUT_STRIP_TRAILING_WHITESPACE)
if(IS_ABSOLUTE "${SEEDGEN_LIBC_A}")
    target_link_options(seedgen PRIVATE -static)
else()
    message(STATUS "seedgen: no static libc found, linking glibc dynamically")
    target_link_options(seedgen PRIVATE -static-libstdc++ -static-libgcc)
endif()
