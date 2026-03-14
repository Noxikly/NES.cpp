find_package(Qt6 QUIET COMPONENTS Core Gui Widgets Multimedia)

if(Qt6_FOUND)
    set(QT_LIBRARIES
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Multimedia
    )
    set(QT_VERSION_MAJOR 6)
else()
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Multimedia)
    set(QT_LIBRARIES
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Multimedia
    )
    set(QT_VERSION_MAJOR 5)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Qt
    REQUIRED_VARS QT_LIBRARIES QT_VERSION_MAJOR
)
