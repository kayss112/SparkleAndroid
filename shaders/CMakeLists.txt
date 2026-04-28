option(SHADER_DEBUG "Allow shader debug" OFF)

if(SHADER_DEBUG)
    message("Shader debug enabled")
    set(SLANGC_FLAGS ${SLANGC_FLAGS} -g -O0)
endif()

message("Vulkan SDK from environment " $ENV{VULKAN_SDK})

find_program(SPIRV_CROSS spirv-cross HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)
if(DEFINED ENV{SLANGC})
    set(SLANGC $ENV{SLANGC})
else()
    find_program(SLANGC slangc HINTS /usr/bin /usr/local/bin)
endif()

set(OUTPUT_SHADER_DIRECTORY ${PROJECT_RESOURCE_DIRECTORY}/shaders)
file(MAKE_DIRECTORY "${OUTPUT_SHADER_DIRECTORY}/standard")
file(MAKE_DIRECTORY "${OUTPUT_SHADER_DIRECTORY}/screen")
file(MAKE_DIRECTORY "${OUTPUT_SHADER_DIRECTORY}/ray_trace")
file(MAKE_DIRECTORY "${OUTPUT_SHADER_DIRECTORY}/utilities")

file(GLOB_RECURSE SLANG_SOURCE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ps.slang"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.vs.slang"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cs.slang"
)

file(GLOB_RECURSE SLANG_INCLUDE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h.slang"
)

set(ALL_SPIRV_OUTPUTS)

# stage 1: slang->spirv
foreach(SLANG ${SLANG_SOURCE_FILES})
    cmake_path(GET SLANG FILENAME SHADER_NAME)
    cmake_path(RELATIVE_PATH SLANG BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE FILE_PATH_RELATIVE)
    
    set(SPIRV "${OUTPUT_SHADER_DIRECTORY}/${FILE_PATH_RELATIVE}.spv")
    string(REPLACE "." ";" PARTS ${SHADER_NAME})
    list(GET PARTS 1 SHADER_TYPE)

    if (SHADER_TYPE STREQUAL "vs")
        set(STAGE "vertex")
    elseif (SHADER_TYPE STREQUAL "ps")
        set(STAGE "fragment")
    elseif (SHADER_TYPE STREQUAL "cs")
        set(STAGE "compute")
    else()
        message(FATAL_ERROR "Invalid shader file name, please provide a valid shader type (vs, ps, cs): " ${SLANG})
    endif()

    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${SLANGC} ${SLANG} -target spirv -o ${SPIRV} -entry main ${SLANGC_FLAGS} ${SHADER_DEFINES} -I ${CMAKE_CURRENT_SOURCE_DIR}/include -profile spirv_1_3
        COMMENT "Compiling ${SHADER_NAME} to SPIRV: ${SLANGC} ${SLANG} -target spirv -o ${SPIRV} -entry main ${SLANGC_FLAGS} ${SHADER_DEFINES} -I ${CMAKE_CURRENT_SOURCE_DIR}/include -profile spirv_1_3"
        DEPENDS ${SLANG} ${SLANG_INCLUDE_FILES}
    )
    list(APPEND ALL_SPIRV_OUTPUTS ${SPIRV})
endforeach(SLANG)

add_custom_target(
    compile-shaders ALL
    DEPENDS ${ALL_SPIRV_OUTPUTS}
)

add_dependencies(cook-resources compile-shaders)
