include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(PC_LUAJIT QUIET luajit)
endif()

find_path(LUAJIT_INCLUDE_DIR
    NAMES luajit.h
    HINTS ${PC_LUAJIT_INCLUDE_DIRS}
    PATH_SUFFIXES luajit-2.1 luajit-2.0 luajit
)

find_library(LUAJIT_LIBRARY
    NAMES luajit-5.1 luajit lua51
    HINTS ${PC_LUAJIT_LIBRARY_DIRS}
)

set(LUAJIT_VERSION ${PC_LUAJIT_VERSION})

find_package_handle_standard_args(LuaJIT
    REQUIRED_VARS LUAJIT_LIBRARY LUAJIT_INCLUDE_DIR
    VERSION_VAR LUAJIT_VERSION
)

if(LuaJIT_FOUND AND NOT TARGET LuaJIT::LuaJIT)
    add_library(LuaJIT::LuaJIT UNKNOWN IMPORTED)
    set_target_properties(LuaJIT::LuaJIT PROPERTIES
        IMPORTED_LOCATION "${LUAJIT_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LUAJIT_INCLUDE_DIR}"
    )
endif()

set(LUAJIT_LIBRARIES ${LUAJIT_LIBRARY})
set(LUAJIT_INCLUDE_DIRS ${LUAJIT_INCLUDE_DIR})

mark_as_advanced(LUAJIT_INCLUDE_DIR LUAJIT_LIBRARY)
