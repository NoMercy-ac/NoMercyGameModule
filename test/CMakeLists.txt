cmake_minimum_required(VERSION 3.8)
project(GameModuleTest CXX)

file(GLOB GameModuleTest_SOURCES
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

add_executable(${PROJECT_NAME} ${GameModuleTest_SOURCES})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ${PROJECT_NAME} PROPERTY CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++) 
endif()

target_link_libraries(${PROJECT_NAME} ${BIN_EXTRA_LIBS})
