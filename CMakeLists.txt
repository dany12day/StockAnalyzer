# 3.21 is the floor required by CMakePresets.json schema version 3, which is
# the documented way to configure this project. Qt 6.11 itself declares
# 3.16...3.21, so this sits at the top of Qt's own supported range.
cmake_minimum_required(VERSION 3.21)

project(ValueApp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.11.2 EXACT REQUIRED COMPONENTS Quick Network)

qt_standard_project_setup(REQUIRES 6.11.2)

# The domain layer. Declared before the app target that links it.
# It links no Qt at all -- see domain/CMakeLists.txt for why that is the
# enforcement mechanism for A2 rather than a convention.
add_subdirectory(domain)

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
    infrastructure/StockData.cpp
    # The header is listed deliberately. AUTOMOC discovers Q_OBJECT headers by
    # looking for a same-named header beside each .cpp; now that headers live
    # under infrastructure/include/ and sources under infrastructure/src/, that
    # heuristic no longer finds it, and the moc-generated signal bodies silently
    # vanish -- surfacing as "undefined reference to StockData::resultTextChanged()"
    # at link time rather than as a clear error.
    infrastructure/StockData.hpp
)

if (ANDROID)
    add_android_openssl_libraries(appValueApp)
endif()

qt_add_qml_module(appValueApp
    URI ValueApp
    VERSION 1.0
    QML_FILES
        ui/Main.qml
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
    PRIVATE domain
)

target_compile_options(appValueApp PRIVATE -Wall -Wextra -Wpedantic -Werror)

include(GNUInstallDirs)
install(TARGETS appValueApp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ${CMAKE_CURRENT_SOURCE_DIR} lets application code write the self-documenting
# form, #include <infrastructure/StockData.hpp>.
#
# infrastructure/ must ALSO be on the path, and for a non-obvious reason:
# qmltyperegistrar generates the QML registration unit from the basename moc
# records in the metatypes JSON, and guards it as
#     #if __has_include(<StockData.hpp>)
# If that basename does not resolve, the include is skipped with no diagnostic
# and the build fails later with "'StockData' was not declared in this scope".
# Removing this line reproduces that.
target_include_directories(appValueApp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/infrastructure
)

# Tests (CLAUDE.md 4.1). Catch2 for the domain layer, which links zero Qt.
#
# enable_testing() must be called at top level or ctest finds nothing.
#
# Never built for Android: cross-compiling Catch2 for arm64 is slow and the
# resulting binary cannot be run by the desktop test harness anyway. CI
# runs the suite in the Linux job.
option(BUILD_TESTING "Build the domain test suite" ON)

if (BUILD_TESTING AND NOT ANDROID)
    enable_testing()
    add_subdirectory(tests)
endif()
