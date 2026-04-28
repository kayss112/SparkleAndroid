cmake_minimum_required(VERSION 3.22)
project(sparkle)

# ------------------ c++ configuration ------------------

set(CMAKE_CXX_STANDARD 20)

# required by clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------ platform configuration ------------------

option(USE_GLFW "Use GLFW instead of the native window system" OFF)
option(ENABLE_PROFILER "Build with profiler support" OFF)
option(ENABLE_TEST_CASES "Build with test case support" ON)

macro(assign_bool var)
     if(${ARGN})
         set(${var} ON)
     else()
         set(${var} OFF)
     endif()
endmacro()

assign_bool(GLFW USE_GLFW)

if(GLFW)
    set(READABLE_PLATFORM_NAME glfw)
    add_executable(sparkle "")
elseif(ANDROID)
    set(READABLE_PLATFORM_NAME android)
    add_library(sparkle SHARED "")
else()
    message(FATAL_ERROR "Not a valid NATIVE_PLATFORM")
endif()

if(CMAKE_BUILD_TYPE)
    set(BUILD_CONFIG ${CMAKE_BUILD_TYPE})
else()
    set(BUILD_CONFIG Multiple)
endif()

message("Build configuration: ${BUILD_CONFIG}")
message("Framework: ${READABLE_PLATFORM_NAME}")
message("Compiler: ${CMAKE_CXX_COMPILER_ID}")
message("Compiler simulate: ${CMAKE_CXX_SIMULATE_ID}")

set(OUTPUT_ROOT ${CMAKE_SOURCE_DIR}/build_system/${READABLE_PLATFORM_NAME}/output)
set(PRODUCT_OUTPUT_DIRECTORY ${OUTPUT_ROOT}/build)

# ------------------ source files ------------------

set(INCLUDES
    "libraries/include/"
    "frameworks/include/"
)

FILE(GLOB_RECURSE HEADERS
    "libraries/*.h"
    "frameworks/*.h"
)
FILE(GLOB_RECURSE SOURCES
    "libraries/source/*.cpp"
)

if(ENABLE_TEST_CASES)
    target_compile_definitions(sparkle PRIVATE ENABLE_TEST_CASES=1)
    FILE(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
    target_sources(sparkle PRIVATE ${TEST_SOURCES})
    target_include_directories(sparkle PRIVATE "tests/")
else()
    target_compile_definitions(sparkle PRIVATE ENABLE_TEST_CASES=0)
endif()

# ------------------ compiler configuration ------------------

# global macro header that should be included in every source file imiplicitly
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    target_compile_options(sparkle PRIVATE /FI core/GlobalMacro.h)
else()
    target_compile_options(sparkle PRIVATE -include core/GlobalMacro.h)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(sparkle PRIVATE /MP /Zc:preprocessor /utf-8)
    target_compile_options(sparkle PRIVATE
        /W4
        /WX
        /wd4068                         # unknown pragma: we use compiler specific pragma extensively, so this is necessary
        /wd4324                         # structure was padded due to alignment specifier: we need that for gpu data alignment
        /wd4702                         # unreachable code: not so necessary
        /wd4715                         # not all control paths return a value: too many false alerts
        /wd4714                         # function marked as __forceinline not inlined: we understand inline is just a suggestion
        )
else()
    target_compile_options(sparkle PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        -Wno-c++98-compat               # we are targeting c++20
        -Wno-c++98-compat-pedantic      # we are targeting c++20
        -Wno-c99-extensions             # we are targeting c++20
        -Wno-c++20-compat               # we are targeting c++20
        -Wno-extra-semi-stmt            # not that necessary
        -Wno-unused-macros              # too much false alerts
        -Wno-covered-switch-default     # -Wswitch-default has already provided enough protection
        -Wno-switch-enum                # it is not always possible to list all enums (especially for vulkan enums)
        -Wno-cast-function-type-strict  # TODO: remove later
        -Wno-unsafe-buffer-usage        # TODO: remove later
        -Wno-global-constructors        # TODO: remove later
        -Wno-exit-time-destructors      # TODO: remove later
        -Wno-unknown-pragmas            # TODO: remove later
        )
endif()

# android does not support LTO for now.
if (NOT (ANDROID))
    include(CheckIPOSupported)
    check_ipo_supported(RESULT supported OUTPUT error)

    if( supported )
        message(STATUS "IPO / LTO enabled for release builds")
        set_property(TARGET sparkle PROPERTY CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    else()
        message(STATUS "IPO / LTO not supported: <${error}>")
    endif()
endif()

if (ENABLE_ASAN)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(SANITIZER_FLAGS "/fsanitize=address")
    else()
        if (CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
            set(SANITIZER_FLAGS "")
        else()
            set(SANITIZER_FLAGS "-fsanitize=address,undefined")
        endif()
    endif()

    if("${SANITIZER_FLAGS}" STREQUAL "")
        message(WARNING "ASAN will not be enabled!")
    else()
        message(STATUS "ASAN enabled")

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
        set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${SANITIZER_FLAGS}")

        if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi /FS")
            set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} /Debug")
            
            get_filename_component(MSVC_BINARY_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
            message(STATUS "MSVC_BINARY_DIR = ${MSVC_BINARY_DIR}")
            add_custom_command(TARGET sparkle POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MSVC_BINARY_DIR}/clang_rt.asan_dynamic-x86_64.dll"
                $<TARGET_FILE_DIR:sparkle>
            )
        else()
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-omit-frame-pointer")
        endif()
    endif()
endif()

# ------------------ graphics ------------------

if ((ANDROID) OR (GLFW))
    assign_bool(ENABLE_VULKAN true)
else()
    error("No valid graphics API for current platform.")
endif()

if(ENABLE_VULKAN)
    target_compile_definitions(sparkle PRIVATE ENABLE_VULKAN)

    if(WIN32)
        target_compile_definitions(sparkle PRIVATE VK_USE_PLATFORM_WIN32_KHR NOMINMAX)
    elseif(ANDROID)
        target_compile_definitions(sparkle PRIVATE VK_USE_PLATFORM_ANDROID_KHR)
    endif()
    
    if (ANDROID)
        find_package(game-activity REQUIRED CONFIG)

        # android will use vulkan SDK provided by android toolchain
        set(LIBRARIES ${LIBRARIES}
            # vulkan
            game-activity::game-activity_static
            android
            log
            )

        set(CMAKE_SHARED_LINKER_FLAGS
            "${CMAKE_SHARED_LINKER_FLAGS} -u \
            Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")
    elseif(GLFW)
        find_package(Vulkan REQUIRED)

        set(LIBRARIES ${LIBRARIES} Vulkan::Vulkan)
    endif()

    set(INCLUDES
        ${INCLUDES}
        ${Vulkan_INCLUDE_DIRS}
    )
endif()

# ------------------ resources ------------------

add_custom_target(cook-resources)

if (ANDROID)
    set(PROJECT_RESOURCE_DIRECTORY "${CMAKE_SOURCE_DIR}/build_system/${READABLE_PLATFORM_NAME}/output/assets/packed")
else()
    set(PROJECT_RESOURCE_DIRECTORY "${PRODUCT_OUTPUT_DIRECTORY}/packed")
endif()

add_custom_target(copy-resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources/packed ${PROJECT_RESOURCE_DIRECTORY}
)

add_dependencies(copy-resources cook-resources)

add_subdirectory(shaders)

add_dependencies(sparkle copy-resources)

# ------------------ framework configuration ------------------

if(GLFW)
    target_compile_definitions(sparkle PRIVATE FRAMEWORK_GLFW=1)
    find_package(glfw3 CONFIG REQUIRED)
    set(LIBRARIES ${LIBRARIES} glfw)
    FILE(GLOB_RECURSE GLFW_SOURCES
        "frameworks/source/glfw/*.cpp"
    )
    SET(SOURCES ${SOURCES} ${GLFW_SOURCES})
elseif(ANDROID)
    target_compile_definitions(sparkle PRIVATE FRAMEWORK_ANDROID=1)
    FILE(GLOB_RECURSE ANDROID_SOURCES
        "frameworks/source/android/*.cpp"
    )
    SET(SOURCES ${SOURCES} ${ANDROID_SOURCES})
else()
    message(FATAL_ERROR "No valid platform is specified. Valid options: [GLFW, Android]")
endif()

if (ANDROID)
    target_compile_definitions(sparkle PRIVATE PLATFORM_ANDROID=1)
elseif (WIN32)
    target_compile_definitions(sparkle PRIVATE PLATFORM_WINDOWS=1)
endif()

# ------------------ third party libraries ------------------

add_subdirectory(thirdparty)

# ------------------ IDE config ------------------

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}
    FILES ${HEADERS}
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}
    FILES ${SOURCES}
)

if(ENABLE_TEST_CASES)
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${TEST_SOURCES}
    )
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    PREFIX "thirdparty"
    FILES ${THIRD_PARTY_SOURCES}
)

if (MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT sparkle)
endif()

# Set output directory for all generators (single-config and multi-config)
set_target_properties(sparkle PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PRODUCT_OUTPUT_DIRECTORY}
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PRODUCT_OUTPUT_DIRECTORY}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PRODUCT_OUTPUT_DIRECTORY}
)

# Set Visual Studio debugger working directory to where resources are located
if (MSVC)
    set_target_properties(sparkle PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY ${PRODUCT_OUTPUT_DIRECTORY}
    )
endif()

# ------------------ final output ------------------

target_include_directories(sparkle PRIVATE ${INCLUDES})

target_sources(sparkle PRIVATE ${HEADERS} ${SOURCES} ${RESOURCES})

target_link_libraries(sparkle PUBLIC ${LIBRARIES})
