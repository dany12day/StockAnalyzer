# The domain layer (CLAUDE.md A1, A2).
#
# This target must NEVER link a Qt module, and must never be given Qt's
# include directories. That is not a style preference: it is the mechanism
# that enforces A2. Qt's headers reach a target only through the INTERFACE
# usage requirements of a Qt6:: target, and usage requirements propagate
# from a linked library to its consumer, never backwards. So as long as
# nothing here links Qt, `#include <QString>` in domain code fails to
# compile, and the architectural rule is checked by the compiler instead of
# by memory.
#
# If you ever find yourself adding Qt6::Core here to fix a build error, the
# error is the point. Move the code that needs Qt into infrastructure.

add_library(domain STATIC
    src/ToolchainProbe.cpp
    src/Currency.cpp
    src/Quote.cpp
    src/Dcf.cpp
    src/Symbol.cpp
)

# PUBLIC: consumers (the app, the tests) need these headers too.
target_include_directories(domain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# PUBLIC so that anything linking domain also gets C++23.
target_compile_features(domain PUBLIC cxx_std_23)

target_compile_options(domain PRIVATE -Wall -Wextra -Wpedantic -Werror)

# qt_standard_project_setup() switches CMAKE_AUTOMOC/AUTOUIC/AUTORCC on for
# the whole project. Those are Qt's code generators and they have no business
# scanning a layer that does not know Qt exists (A2). Turning them off here
# is not just tidiness: it stops moc walking these sources on every build.
set_target_properties(domain PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
)
