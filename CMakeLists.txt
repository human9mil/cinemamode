cmake_minimum_required(VERSION 3.16)
project(cinemamode VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick QuickControls2)
find_package(KF6Kirigami REQUIRED)
find_package(KF6GlobalAccel REQUIRED)
find_package(KF6StatusNotifierItem REQUIRED)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(cinemamode
    src/main.cpp
    src/monitorcontroller.cpp
    src/monitorcontroller.h
)

qt_add_qml_module(cinemamode
    URI CinemaMode
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

target_include_directories(cinemamode PRIVATE src)

target_link_libraries(cinemamode PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    KF6::GlobalAccel
    KF6::StatusNotifierItem
)

include(GNUInstallDirs)

install(TARGETS cinemamode
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES packaging/org.cinemamode.app.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

install(FILES packaging/org.cinemamode.app.metainfo.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
)

install(FILES packaging/icons/org.cinemamode.app.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)
