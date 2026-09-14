# Domain test suite (CLAUDE.md 4.1, 8).
#
# These tests link `domain` and nothing else. They must never link Qt --
# that is the whole point of A2: the calculations are provable without a
# GUI toolkit in the picture. Anything touching QObject gets tested with
# Qt Test in a separate target, later.

include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.16.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)

add_executable(domain_tests
    ToolchainProbeTest.cpp
)

# Catch2::Catch2WithMain supplies main(). Catch2 v3 is a compiled library,
# not a header -- if you find a tutorial using CATCH_CONFIG_MAIN, it is
# written for v2 and does not apply.
target_link_libraries(domain_tests PRIVATE domain Catch2::Catch2WithMain)

target_compile_options(domain_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)

# Catch.cmake ships in Catch2's extras/ and provides catch_discover_tests,
# which runs the built binary to enumerate its TEST_CASEs so each becomes a
# separate CTest entry. Without it you get one pass/fail blob in CI.
# Same reasoning as domain/: no Qt here, so no Qt code generators.
set_target_properties(domain_tests PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(domain_tests)
