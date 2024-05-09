cmake_minimum_required(VERSION 3.2)
project(GameModule CXX)
set(OUTPUT_NAME "nomercy_game_module_x${TARGET_ARCH_STR}_${CMAKE_CXX_COMPILER_ID}")

file(GLOB GameModule_HEADERS
    "${PROJECT_SOURCE_DIR}/include/*.h"
    "${PROJECT_SOURCE_DIR}/include/*.hpp"
)
file(GLOB GameModule_SOURCES
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

add_library(${OUTPUT_NAME} SHARED ${GameModule_HEADERS} ${GameModule_SOURCES})

set_property(TARGET ${OUTPUT_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${OUTPUT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ${OUTPUT_NAME} PROPERTY CMAKE_CXX_EXTENSIONS OFF)

# if (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
#     target_link_options(${OUTPUT_NAME} PRIVATE -static-libgcc -static-libstdc++)
# endif()

target_link_libraries(${OUTPUT_NAME} ${BIN_EXTRA_LIBS})
