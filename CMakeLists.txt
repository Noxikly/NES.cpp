cmake_minimum_required(VERSION 3.22)
project(nespp
        VERSION 0.4.1
        DESCRIPTION "A simple NES emulator"
        HOMEPAGE_URL "https://github.com/Noxikly/NES.cpp"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/forms
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/forms/settings
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")
find_package(Qt REQUIRED)
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

# Gui sources
set(GUI_SOURCES
    src/main.cpp
    src/gui/update.cpp
    src/gui/modules/frame.cpp
    src/gui/modules/audio.cpp
    src/gui/w_settings.cpp
    src/gui/w_main.cpp
)

set(SOURCES
    ${CORE_SOURCES}
    ${GUI_SOURCES}
)

add_executable(${PROJECT_NAME} ${SOURCES})


target_sources(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:src/gui/w_logs.cpp>
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        /permissive-
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:RelWithDebInfo>:/O2>
        $<$<CONFIG:MinSizeRel>:/O1>
    )

    set_property(TARGET ${PROJECT_NAME} PROPERTY
        MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")

    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:/INCREMENTAL:NO>
        $<$<CONFIG:Debug>:/DEBUG:FASTLINK>
        $<$<CONFIG:Debug>:/ignore:4099>
    )

    set_property(TARGET ${PROJECT_NAME} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        $<$<CONFIG:Debug>:-Og>
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:RelWithDebInfo>:-O2>
        $<$<CONFIG:MinSizeRel>:-Os>
    )
endif()

if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(${PROJECT_NAME} PRIVATE -static -static-libgcc -static-libstdc++)
elseif(UNIX)
    target_link_options(${PROJECT_NAME} PRIVATE -rdynamic)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${QT_LIBRARIES} LuaJIT::LuaJIT)


target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
)
