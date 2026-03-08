cmake_minimum_required(VERSION 3.22)
project(nespp
        VERSION 0.3.0
        DESCRIPTION "A simple NES emulator"
        HOMEPAGE_URL "https://github.com/Noxikly/NES.cpp"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-O2 -Wall -Wextra -Wpedantic")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/forms)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LUAJIT REQUIRED luajit)

# Core sources
set(CORE_SOURCES
    src/core/cpu.cpp
    src/core/apu.cpp
    src/core/mem.cpp
    src/core/cartridge.cpp
    src/core/lua.cpp
    src/core/ppu.cpp
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SOURCES
        ${CORE_SOURCES}
        tests/c6502.cpp
    )
else()
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia)

    set(SOURCES
        ${CORE_SOURCES}
        src/main.cpp
        src/gui/w_main.cpp
        src/gui/nes_frame.cpp
        src/gui/nes_audio.cpp
        src/gui/forms/w_main.ui
    )
endif()

add_executable(${PROJECT_NAME} ${SOURCES})

if(WIN32)
    target_link_options(${PROJECT_NAME} PRIVATE -Wl,--export-dynamic)
elseif(UNIX)
    target_link_options(${PROJECT_NAME} PRIVATE -rdynamic)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${LUAJIT_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${LUAJIT_LIBRARY_DIRS})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_package(GTest REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest ${LUAJIT_LIBRARIES})
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Multimedia ${LUAJIT_LIBRARIES})
endif()


target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/android
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
)
