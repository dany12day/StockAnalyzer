#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace domain
{

/// Errors that ToolchainProbe::create can report.
///
/// An enum rather than a string, per CLAUDE.md A6 Corollary 2: a string
/// cannot be asserted on meaningfully in a test, nor localized in the UI.
enum class ProbeError
{
    Empty,
    TooLong
};

/// M0 toolchain probe. Deliberately throwaway.
///
/// Two jobs, both of which disappear once M1 lands real domain types:
///
///  1. Prove that the C++23 library features this project depends on are
///     actually present in the Android NDK's libc++, not just in desktop
///     GCC. Because appValueApp links this target and appValueApp builds
///     for Android, every Android build recompiles this file. A future NDK
///     that drops support surfaces as a build failure rather than as a
///     surprise halfway through a milestone.
///
///     Verified present on NDK 27.2.12479018 (clang 18, aarch64, API 28):
///     std::expected, std::views::zip, std::ranges::to.
///
///     NOT present there, despite compiling fine on desktop GCC:
///     std::views::enumerate, std::flat_map, std::generator,
///     std::stacktrace. Do not use them. CLAUDE.md 4.1 names
///     views::enumerate as desirable; the toolchain floor disagrees.
///
///  2. Establish the validation idiom every later domain type follows
///     (A6 Corollary 1): the constructor is private and total, validation
///     lives in a static factory returning std::expected, so an invalid
///     instance is unrepresentable rather than merely unlikely.
class ToolchainProbe
{
public:
    /// Validates `text` and returns a probe, or the reason it was rejected.
    [[nodiscard]] static std::expected<ToolchainProbe, ProbeError> create(std::string_view text);

    [[nodiscard]] const std::string &text() const noexcept { return m_text; }

    /// Element-wise sum of two sequences, truncated to the shorter one.
    /// Exists only to exercise std::views::zip and std::ranges::to.
    [[nodiscard]] static std::vector<int> pairwiseSums(const std::vector<int> &lhs, const std::vector<int> &rhs);

private:
    explicit ToolchainProbe(std::string text);

    std::string m_text;
};

} // namespace domain
