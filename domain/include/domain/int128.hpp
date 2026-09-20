#pragma once

namespace domain
{

/// A signed 128-bit integer, used as the storage type for monetary amounts.
///
/// This alias exists so the non-standard type appears in exactly one place.
/// Everything downstream spells it `int128_t` and never writes `__int128`.
///
/// \warning The `__extension__` prefix is load-bearing. `__int128` is a GCC
///          and clang extension, and this project compiles with
///          `-Wpedantic -Werror` and `CMAKE_CXX_EXTENSIONS OFF`, so a bare
///          `__int128` is a hard error on GCC:
///          *"ISO C++ does not support '__int128'"*. It compiles cleanly on
///          the Android NDK's clang, which is exactly the trap described in
///          CLAUDE.md section 4.1 - the desktop and CI builds would fail
///          while the Android build passed. One `__extension__` at the
///          typedef launders every later use, including in `constexpr`
///          contexts and `static_assert`. Do not remove it.
///
/// Why 128 bits rather than 64 (CLAUDE.md D8): at scale 6 an `int64_t` caps
/// out around 9.2e12 currency units, which overflows on real statement
/// figures - Toyota's revenue is roughly JPY 45.1e12. At 128 bits overflow
/// stops being something the arithmetic has to reason about.
///
/// Note that `std::numeric_limits` *is* specialised for this type even under
/// strict `-std=c++23`, so range checks against it are sound. There is,
/// however, no `operator<<` for it in libstdc++; see the Catch2 StringMaker
/// in `domain/tests/StringMakers.hpp`.
__extension__ typedef __int128 int128_t;

} // namespace domain
