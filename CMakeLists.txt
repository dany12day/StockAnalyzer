cmake_minimum_required(VERSION 3.16)

project(ValueApp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.11.2 EXACT REQUIRED COMPONENTS Quick Network)

qt_standard_project_setup(REQUIRES 6.11.2)

if (ANDROID)
    include(FetchContent)
    FetchContent_Declare(android_openssl
        DOWNLOAD_EXTRACT_TIMESTAMP true
        URL https://github.com/KDAB/android_openssl/archive/refs/heads/master.zip)
    FetchContent_MakeAvailable(android_openssl)
    include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)
endif()

qt_add_executable(appValueApp
    main.cpp
    app/StockDataAPI/StockData.cpp
)

if (ANDROID)
    add_android_openssl_libraries(appValueApp)
endif()

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
    PRIVATE Qt6::Network
)

target_compile_options(appValueApp PRIVATE -Wall -Wextra -Wpedantic -Werror)

include(GNUInstallDirs)
install(TARGETS appValueApp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

target_include_directories(appValueApp PRIVATE app/StockDataAPI)
