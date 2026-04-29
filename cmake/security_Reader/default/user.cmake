# Project-specific CMake overlay for the generated MPLAB VS Code build.
# Keep durable customizations here so regenerated files under .generated stay disposable.

# Pin the device family pack and CMSIS pack versions used by this firmware.
set(PIC32CM5164JH01048_DFP
    "${PACK_REPO_PATH}/Microchip/PIC32CM-JH_DFP/1.6.279/PIC32CM-JH01")
set(CMSIS_6_ROOT
    "${PACK_REPO_PATH}/ARM/CMSIS/6.3.0")
set(PIC32CM5164JH01048_PROJECT_ROOT
    "${CMAKE_CURRENT_LIST_DIR}/../../..")

# Use startup, system, and linker scripts from the installed DFP.
set(PIC32CM5164JH01048_STARTUP
    "${PIC32CM5164JH01048_DFP}/gcc/gcc/startup_pic32cm5164jh01048.c")
set(PIC32CM5164JH01048_SYSTEM
    "${PIC32CM5164JH01048_DFP}/gcc/system_pic32cm5164jh01048.c")
set(PIC32CM5164JH01048_LINKER_SCRIPT
    "${PIC32CM5164JH01048_DFP}/gcc/gcc/pic32cm5164jh01048_flash.ld")
# Application-owned sources live outside the generated CMake tree.
set(PIC32CM5164JH01048_APP_SOURCES
    "${PIC32CM5164JH01048_PROJECT_ROOT}/src/main.c"
    "${PIC32CM5164JH01048_PROJECT_ROOT}/src/board_led.c"
    "${PIC32CM5164JH01048_PROJECT_ROOT}/src/bussec_cli.c"
    "${PIC32CM5164JH01048_PROJECT_ROOT}/src/clock_config.c"
    "${PIC32CM5164JH01048_PROJECT_ROOT}/src/host_console.c"
    "${PIC32CM5164JH01048_PROJECT_ROOT}/src/tc0_periodic.c")

# Fail early when the expected pack install or application layout is missing.
foreach(required_file
        "${PIC32CM5164JH01048_STARTUP}"
        "${PIC32CM5164JH01048_SYSTEM}"
        "${PIC32CM5164JH01048_LINKER_SCRIPT}"
        ${PIC32CM5164JH01048_APP_SOURCES})
    if(NOT EXISTS "${required_file}")
        message(FATAL_ERROR "Required PIC32CM5164JH01048 project or DFP file not found: ${required_file}")
    endif()
endforeach()

# Discover the generated executable and object-library targets without depending on names.
get_property(project_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
set(PIC32CM5164JH01048_COMPILE_TARGETS)
foreach(candidate_target IN LISTS project_targets)
    get_target_property(candidate_type "${candidate_target}" TYPE)
    if(candidate_type STREQUAL "EXECUTABLE")
        set(PIC32CM5164JH01048_IMAGE_TARGET "${candidate_target}")
    elseif(candidate_type STREQUAL "OBJECT_LIBRARY")
        list(APPEND PIC32CM5164JH01048_COMPILE_TARGETS "${candidate_target}")
    endif()
endforeach()

if(NOT PIC32CM5164JH01048_IMAGE_TARGET)
    message(FATAL_ERROR "Could not find generated firmware executable target")
endif()

# Remove generated/local duplicate startup files from object libraries before adding our sources.
set(PIC32CM5164JH01048_GENERATED_MAIN
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../main.c")
set(PIC32CM5164JH01048_MAIN_SOURCE
    "${PIC32CM5164JH01048_PROJECT_ROOT}/src/main.c")
set(PIC32CM5164JH01048_LOCAL_STARTUP
    "${PIC32CM5164JH01048_PROJECT_ROOT}/startup_pic32cm5164jh01048.c")
set(PIC32CM5164JH01048_LOCAL_SYSTEM
    "${PIC32CM5164JH01048_PROJECT_ROOT}/system_pic32cm5164jh01048.c")

foreach(compile_target IN LISTS PIC32CM5164JH01048_COMPILE_TARGETS)
    get_target_property(compile_target_sources "${compile_target}" SOURCES)
    if(compile_target_sources)
        list(REMOVE_ITEM compile_target_sources
            "${PIC32CM5164JH01048_GENERATED_MAIN}"
            "${PIC32CM5164JH01048_LOCAL_STARTUP}"
            "${PIC32CM5164JH01048_LOCAL_SYSTEM}")

        list(FIND compile_target_sources "${PIC32CM5164JH01048_MAIN_SOURCE}" main_source_index)
        if(main_source_index EQUAL -1)
            list(APPEND compile_target_sources "${PIC32CM5164JH01048_MAIN_SOURCE}")
        endif()

        set_target_properties("${compile_target}" PROPERTIES
            SOURCES "${compile_target_sources}")
    endif()
endforeach()

# Link the image against the DFP reset/startup path selected above.
target_sources("${PIC32CM5164JH01048_IMAGE_TARGET}" PRIVATE
    "${PIC32CM5164JH01048_STARTUP}"
    "${PIC32CM5164JH01048_SYSTEM}")

list(APPEND PIC32CM5164JH01048_COMPILE_TARGETS "${PIC32CM5164JH01048_IMAGE_TARGET}")

# Apply the same device headers, CMSIS headers, device define, and CPU flags to every target.
foreach(compile_target IN LISTS PIC32CM5164JH01048_COMPILE_TARGETS)
    target_include_directories("${compile_target}" PRIVATE
        "${PIC32CM5164JH01048_PROJECT_ROOT}/include"
        "${PIC32CM5164JH01048_DFP}/include"
        "${PIC32CM5164JH01048_DFP}/include/pio"
        "${CMSIS_6_ROOT}/CMSIS/Core/Include")

    target_compile_definitions("${compile_target}" PRIVATE
        "__PIC32CM5164JH01048__")

    target_compile_options("${compile_target}" PRIVATE
        "-std=c23"
        "-mcpu=cortex-m0plus"
        "-mthumb"
        "-O2"
        "-ffunction-sections"
        "-Wall")
endforeach()

# Keep linker flags explicit so the image uses the PIC32CM5164JH01048 flash memory map.
# The v0.4 design bans runtime heap use and requires a link failure when
# .data + .bss + reserved stack exceed 57344 bytes.
target_link_options("${PIC32CM5164JH01048_IMAGE_TARGET}" PRIVATE
    "-mcpu=cortex-m0plus"
    "-mthumb"
    "-nostartfiles"
    "-Wl,--defsym=HEAP_SIZE=0"
    "-Wl,--defsym=STACK_SIZE=0x2000"
    "-Wl,--defsym=RAM_LENGTH=0xE000"
    "-T${PIC32CM5164JH01048_LINKER_SCRIPT}"
    "-Wl,-Map=${security_Reader_default_output_dir}/${security_Reader_default_image_base_name}.map"
    "-Wl,--print-memory-usage")
