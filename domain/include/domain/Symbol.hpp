#pragma once

#include <cstddef>
#include <expected>
#include <string>
#include <string_view>

namespace domain
{

/// Reasons Symbol::create can reject its input.
///
/// An enum rather than a string (CLAUDE.md A6 Corollary 2): tests assert on
/// it, and the presentation layer turns it into text a person understands.
enum class SymbolError
{
    Empty,           ///< The text has no characters.
    TooLong,         ///< The text is longer than Symbol::maxLength.
    InvalidCharacter ///< The text contains a character outside the allowlist.
};

/// A validated exchange ticker, such as `AAPL`, `BRK-B` or `VOW3.DE`.
///
/// Holding a Symbol guarantees its text is:
///  - non-empty,
///  - at most #maxLength characters,
///  - made only of `A`-`Z`, `0`-`9`, `.` and `-`.
///
/// That allowlist makes the text safe to place in a request URL without
/// escaping.
///
/// Two consequences are deliberate decisions, pinned by the `[decision]` tests:
///  - **Uppercase only.** `aapl` is rejected, not normalised. Callers
///    uppercase user input before calling create.
///  - **Businesses only.** `^` and `=` are excluded, so indices (`^GSPC`),
///    FX pairs (`EURUSD=X`) and futures (`GC=F`) are not Symbols
///    (CLAUDE.md section 1). A currency rate gets its own type.
///
/// Follows A6 Corollary 1: the constructor is private and total, and all
/// validation happens once, in create.
class Symbol
{
public:
    /// Longest accepted symbol, inclusive. Leaves room for exchange suffixes.
    static constexpr std::size_t maxLength = 20;

    /// Validates `text` and returns a Symbol, or the reason it was rejected.
    ///
    /// Checks run in this order, and the first failure is reported:
    /// empty, then length, then characters. The text is taken as-is: no
    /// trimming and no case conversion.
    ///
    /// \param text  Candidate ticker. Copied if accepted; need not outlive the call.
    /// \return The Symbol, or SymbolError::Empty, SymbolError::TooLong or
    ///         SymbolError::InvalidCharacter.
    [[nodiscard]] static std::expected<Symbol, SymbolError> create(std::string_view text);

    /// The validated ticker text.
    ///
    /// \warning The reference lives only as long as this Symbol. Binding it
    ///          from a temporary dangles:
    ///          `const auto &t = Symbol::create("AAPL")->getSymbol();`
    [[nodiscard]] const std::string &getSymbol() const noexcept { return m_symbol; }

private:
    /// Total: accepts only text that create has already validated.
    explicit Symbol(std::string symbol);

    std::string m_symbol;
};

} // namespace domain
