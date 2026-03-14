cmake_minimum_required(VERSION 3.22)
project(nespp
        VERSION 0.3.4
        DESCRIPTION "A simple NES emulator"
        HOMEPAGE_URL "https://github.com/Noxikly/NES.cpp"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/forms)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

find_package(LuaJIT REQUIRED)

# Core sources
set(CORE_SOURCES
    src/core/cpu.cpp
    src/core/apu.cpp
    src/core/mem.cpp
    src/core/cartridge.cpp
    src/core/lua.cpp
    src/core/ppu.cpp
)

set(GUI_SOURCES
    src/main.cpp
    src/gui/w_main.cpp
    src/gui/nes_frame.cpp
    src/gui/nes_audio.cpp
    src/gui/forms/w_main.ui
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SOURCES
        ${CORE_SOURCES}
        tests/c6502.cpp
    )
else()
    find_package(Qt REQUIRED)

    set(SOURCES
        ${CORE_SOURCES}
        ${GUI_SOURCES}
    )
endif()

add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_options(${PROJECT_NAME} PRIVATE -O2 -Wall -Wextra -Wpedantic)

if(WIN32)
    target_link_options(${PROJECT_NAME} PRIVATE -Wl,--export-dynamic)
elseif(UNIX)
    target_link_options(${PROJECT_NAME} PRIVATE -rdynamic)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_package(GTest REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest LuaJIT::LuaJIT)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE ${QT_LIBRARIES} LuaJIT::LuaJIT)
endif()


target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
)
