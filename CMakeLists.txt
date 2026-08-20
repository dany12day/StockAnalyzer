cmake_minimum_required(VERSION 3.16)

project(ValueApp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.11.1 EXACT REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.11.1)

qt_add_executable(appValueApp
    main.cpp
)

qt_add_qml_module(appValueApp
    URI ValueApp
    VERSION 1.0
    QML_FILES
        Main.qml
)

set_target_properties(appValueApp PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appValueApp
    PRIVATE Qt6::Quick
)

include(GNUInstallDirs)
install(TARGETS appValueApp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
