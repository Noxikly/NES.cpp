cmake_minimum_required(VERSION 3.23)
project(nespp
        VERSION 0.1.1
        DESCRIPTION "A simple NES emulator"
        HOMEPAGE_URL "https://github.com/Noxikly/NES.cpp"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -Wall -Wextra -Wpedantic")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/forms
)


# Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# LuaJIT
find_package(PkgConfig REQUIRED)
pkg_check_modules(LUAJIT REQUIRED luajit)

# Sources
set(SOURCES
    src/main.cpp
    src/core/cpu.cpp
    src/core/memory.cpp
    src/core/ppu.cpp
    src/core/lua.cpp
    src/gui/nes_frame.cpp
    src/gui/w_main.cpp
)

# UI
set(UI_FILES
    src/gui/forms/w_main.ui
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES} ${UI_FILES})
if(WIN32)
    target_link_options(${PROJECT_NAME} PRIVATE
        -Wl,--export-all-symbols
        -static
        -static-libgcc
        -static-libstdc++
    )
elseif(UNIX)
    target_link_options(${PROJECT_NAME} PRIVATE -rdynamic)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/forms"
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
    ${LUAJIT_INCLUDE_DIRS}
)


target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    ${LUAJIT_LIBRARIES}
)