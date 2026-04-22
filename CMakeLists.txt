cmake_minimum_required(VERSION 3.22)
project(sparkle)

# ------------------ c++ configuration ------------------

set(CMAKE_CXX_STANDARD 20)

# required by clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------ platform validation ------------------

if(NOT ANDROID)
    message(FATAL_ERROR "This project targets Android only.")
endif()

option(ENABLE_PROFILER "Build with profiler support" OFF)
option(ENABLE_TEST_CASES "Build with test case support" ON)

set(READABLE_PLATFORM_NAME android)
add_library(sparkle SHARED "")

if(CMAKE_BUILD_TYPE)
    set(BUILD_CONFIG ${CMAKE_BUILD_TYPE})
else()
    set(BUILD_CONFIG Multiple)
endif()

message("Build configuration: ${BUILD_CONFIG}")
message("Framework: ${READABLE_PLATFORM_NAME}")
message("Compiler: ${CMAKE_CXX_COMPILER_ID}")

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

FILE(GLOB_RECURSE ANDROID_SOURCES
    "frameworks/source/android/*.cpp"
)
set(SOURCES ${SOURCES} ${ANDROID_SOURCES})

if(ENABLE_TEST_CASES)
    target_compile_definitions(sparkle PRIVATE ENABLE_TEST_CASES=1)
    FILE(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")
    target_sources(sparkle PRIVATE ${TEST_SOURCES})
    target_include_directories(sparkle PRIVATE "tests/")
else()
    target_compile_definitions(sparkle PRIVATE ENABLE_TEST_CASES=0)
endif()

# ------------------ compiler configuration ------------------

# global macro header included implicitly in every source file
target_compile_options(sparkle PRIVATE -include core/GlobalMacro.h)

target_compile_options(sparkle PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-c99-extensions
    -Wno-c++20-compat
    -Wno-extra-semi-stmt
    -Wno-unused-macros
    -Wno-covered-switch-default
    -Wno-switch-enum
    -Wno-cast-function-type-strict
    -Wno-unsafe-buffer-usage
    -Wno-global-constructors
    -Wno-exit-time-destructors
    -Wno-unknown-pragmas
    )

if (ENABLE_ASAN)
    set(SANITIZER_FLAGS "-fsanitize=address,undefined")
    message(STATUS "ASAN enabled")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
endif()

# ------------------ graphics (Vulkan) ------------------

target_compile_definitions(sparkle PRIVATE
    ENABLE_VULKAN
    VK_USE_PLATFORM_ANDROID_KHR
)

find_package(game-activity REQUIRED CONFIG)

set(LIBRARIES ${LIBRARIES}
    game-activity::game-activity_static
    android
    log
)

set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u \
    Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")

# ------------------ resources ------------------

add_custom_target(cook-resources)

set(PROJECT_RESOURCE_DIRECTORY "${CMAKE_SOURCE_DIR}/build_system/${READABLE_PLATFORM_NAME}/output/assets/packed")

add_custom_target(copy-resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources/packed ${PROJECT_RESOURCE_DIRECTORY}
)

add_dependencies(copy-resources cook-resources)

add_subdirectory(shaders)

add_dependencies(sparkle copy-resources)

# ------------------ platform configuration ------------------

target_compile_definitions(sparkle PRIVATE
    FRAMEWORK_ANDROID=1
    PLATFORM_ANDROID=1
)

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

# ------------------ final output ------------------

target_include_directories(sparkle PRIVATE ${INCLUDES})

target_sources(sparkle PRIVATE ${HEADERS} ${SOURCES})

target_link_libraries(sparkle PUBLIC ${LIBRARIES})
