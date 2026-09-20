#pragma once

#include <array>
#include <cstddef>
#include <expected>
#include <string_view>

namespace domain
{

/// Reasons Currency::create can reject its input.
///
/// An enum rather than a string (CLAUDE.md A6 Corollary 2): tests assert on
/// it, and the presentation layer turns it into text a person understands.
enum class CurrencyError
{
    InvalidLength,   ///< The text is not exactly Currency::length characters.
    InvalidCharacter ///< The text contains a character outside `A`-`Z`.
};

/// A validated ISO 4217 alpha-3 currency code, such as `USD`, `EUR` or `JPY`.
///
/// Holding a Currency guarantees its text is exactly #length characters, each
/// one in `A`-`Z`.
///
/// Two consequences are deliberate decisions, pinned by the `[decision]` tests:
///  - **Uppercase only.** `usd` is rejected, not normalised. This matches
///    Symbol; two adjacent value types with opposite normalisation rules
///    would be a trap.
///  - **Shape only, not membership.** `XYZ` validates. A hardcoded list of
///    real codes would go stale as currencies are added and redenominated,
///    and the data source dictates the codes in practice.
///
/// Currency is a separate type from Money rather than a member of it because
/// the cross-currency rate type (CLAUDE.md 6.4) pairs two currencies with an
/// as-of date and has no business depending on Money.
///
/// Follows A6 Corollary 1: the constructor is private and total, and all
/// validation happens once, in create.
class Currency
{
public:
    /// Number of characters in an ISO 4217 alpha-3 code. Fixed, not a bound.
    static constexpr std::size_t length = 3;

    /// Validates `currency` and returns a Currency, or the reason it was rejected.
    ///
    /// Checks run in this order, and the first failure is reported: length,
    /// then characters. The text is taken as-is: no trimming and no case
    /// conversion.
    ///
    /// \param currency Candidate code. Copied if accepted; need not outlive the call.
    /// \return The Currency, or CurrencyError::InvalidLength or
    ///         CurrencyError::InvalidCharacter.
    [[nodiscard]] static std::expected<Currency, CurrencyError> create(std::string_view currency);

    /// The validated code, as a view of exactly #length characters.
    ///
    /// \warning The storage holds no NUL terminator. The returned view carries
    ///          an explicit length, so use it as a view - passing its `data()`
    ///          to anything that infers length from a terminator (`std::string`'s
    ///          `const char *` constructor, `printf("%s")`, `strlen`) reads past
    ///          the end of the array.
    /// \warning The view points into this Currency and lives only as long as it
    ///          does. Binding it from a temporary dangles:
    ///          `const auto v = Currency::create("USD")->getCurrency();`
    [[nodiscard]] const std::string_view getCurrency() const noexcept
    {
        return std::string_view(m_currency.data(), m_currency.size());
    }

    /// Compares by code.
    ///
    /// Defaulted, which in C++20 also supplies `==` and `!=`. Equality is the
    /// operation this type exists to support: Money checks it on every
    /// arithmetic call. The ordering is lexicographic over the bytes and
    /// carries no financial meaning; it exists so a Currency can key a
    /// `std::map`, which the rate table will need.
    ///
    /// Defaulting is safe here precisely because there is a single member -
    /// unlike Money, where a defaulted ordering would compare the amount and
    /// silently ignore the currency.
    auto operator<=>(const Currency &) const = default;

private:
    /// Total: accepts only text that create has already validated.
    explicit Currency(std::array<char, 3> currency);

    std::array<char, 3> m_currency{};
};

} // namespace domain
