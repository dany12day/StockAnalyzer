#pragma once

#include "domain/Currency.hpp"
#include "domain/int128.hpp"
#include <cmath>
#include <cstdint>
#include <expected>
#include <initializer_list>
#include <limits>
#include <span>

namespace domain
{

/// Computes `base` raised to `exp` at compile time, using integers only.
///
/// `std::pow` cannot be used here. It is not `constexpr` until C++26; GCC
/// folds it anyway as an extension, but the Android NDK's clang rejects it
/// outright (*"constexpr variable must be initialized by a constant
/// expression"*). Since CI is Linux-only until M2, that failure would pass
/// every check and surface only on an Android build - CLAUDE.md section 4.1
/// again. This loop is accepted by both toolchains, and keeps floating point
/// out of the scale constant entirely.
[[nodiscard]] constexpr int128_t constexprPow(int128_t base, std::uint8_t exp) noexcept
{
    int128_t result = 1;
    for (std::uint8_t i = 0; i < exp; ++i)
    {
        result *= base;
    }

    return result;
}

/// Reasons a Money operation can fail.
///
/// An enum rather than a string (CLAUDE.md A6 Corollary 2): tests assert on
/// it, and the presentation layer turns it into text a person understands.
enum class MoneyError
{
    NotANumber,      ///< A double input was NaN.
    Overflow,        ///< A double input was infinite, or too large to scale into int128_t.
    CurrencyMismatch ///< Two operands carry different currencies. Both are valid; they differ.
};

/// A monetary amount: a fixed-point quantity paired with its currency.
///
/// The amount is stored as an integer count of fractional units, #scaledFactor
/// of them to one currency unit - so USD 12.50 is held as 12500000, not as
/// 12.5. Floating point is not
/// used for storage or arithmetic, and the reasons are concrete rather than
/// conventional:
///  - decimal fractions have no exact binary representation, so `0.1 + 0.2`
///    is not `0.3`;
///  - error accumulates, and `double` addition is **not associative**, so a
///    portfolio total would change when the rows are re-sorted;
///  - precision collapses at statement magnitudes - at JPY 4.5e13 the gap
///    between adjacent doubles is about 0.008, so scale-6 precision is
///    unrepresentable there.
///
/// `double` remains appropriate for derived ratios (where the scale cancels)
/// and for display; see #toDouble.
///
/// Currency is a runtime member rather than a template parameter
/// (CLAUDE.md D7). Mixing currencies therefore compiles and is caught as a
/// value - see #compare - rather than being rejected by the type system.
///
/// Follows A6 Corollary 1: the constructor is private. Note that it is
/// *total* rather than validating, and deliberately so - a valid Currency
/// combined with any integer is always a valid Money, and negative amounts
/// are meaningful (CLAUDE.md 6.4). Only the fallible conversion, #fromDouble,
/// returns `std::expected`; per A7 the others say they cannot fail by not
/// returning one.
class Money
{
public:
    /// Number of decimal places kept. Six is finer than any reported filing.
    static constexpr std::uint8_t scale = 6; // 6 decimal places

    /// The multiplier between currency units and the stored amount, 10^#scale.
    static constexpr int128_t scaledFactor = constexprPow(10, scale);

    /// Builds a Money from whole currency units, scaling by #scaledFactor.
    ///
    /// `fromUnits(100, usd)` is USD 100.00.
    [[nodiscard]] static Money fromUnits(int128_t amount, Currency currency)
    {
        return Money(amount * scaledFactor, currency);
    }

    /// Builds a Money from an already-scaled amount, stored verbatim.
    ///
    /// `fromScaled(100, usd)` is USD 0.000100. Prefer #fromUnits unless the
    /// caller genuinely holds a scaled figure.
    [[nodiscard]] static Money fromScaled(int128_t scaledAmount, Currency currency)
    {
        return Money(scaledAmount, currency);
    }

    /// Converts a `double` into a Money, rounding to the nearest scaled amount.
    ///
    /// This is the boundary adapter for data that arrives as a JSON number,
    /// which is the only form the price endpoint offers. It is the one
    /// fallible factory, because some doubles have no representation here:
    /// NaN, either infinity, and finite values too large to scale.
    ///
    /// Note that a finite check alone would be insufficient - `1e300` is
    /// finite, and casting it after scaling would be undefined behaviour -
    /// so the magnitude is range-checked before the cast.
    ///
    /// \param amount   Value in whole currency units.
    /// \param currency Currency of the result.
    /// \return The Money, or MoneyError::NotANumber or MoneyError::Overflow.
    [[nodiscard]] static std::expected<Money, MoneyError> fromDouble(double amount, Currency currency)
    {
        if (std::isnan(amount))
        {
            return std::unexpected(MoneyError::NotANumber);
        }

        double scaledAmount = amount * scaledFactor;
        if (scaledAmount > static_cast<double>(std::numeric_limits<int128_t>::max()) ||
            scaledAmount < static_cast<double>(std::numeric_limits<int128_t>::min()))
        {
            return std::unexpected(MoneyError::Overflow);
        }

        return Money(static_cast<int128_t>(std::round(scaledAmount)), currency);
    }

    /// The amount in whole currency units, as a `double`.
    ///
    /// \warning Lossy above roughly 9e9 currency units: a `double` holds
    ///          integers exactly only to 2^53, which at #scale is about
    ///          9.0e9. Intended for display and charting. Never route
    ///          arithmetic through this - see the class notes for why.
    [[nodiscard]] double toDouble() const noexcept
    {
        return static_cast<double>(m_scaledAmount) / static_cast<double>(scaledFactor);
    }

    /// The currency this amount is denominated in.
    [[nodiscard]] Currency getCurrency() const noexcept { return m_currency; }

    /// The raw stored amount: #scaledFactor of these make one currency unit.
    ///
    /// Named for what it is: this is 10^#scale times the currency amount, and
    /// reading it as units is wrong by a factor of a million.
    [[nodiscard]] int128_t getScaledAmount() const noexcept { return m_scaledAmount; }

    /// Equality over both the amount and the currency.
    ///
    /// Defaulted, which compares every member, so USD 5 and EUR 5 are *not*
    /// equal and no exchange rate is consulted to decide that. Only `==` is
    /// defaulted: a defaulted `<=>` would compare #getScaledAmount first and
    /// silently provide a meaningless cross-currency ordering. Ordering goes
    /// through #compare instead.
    [[nodiscard]] bool operator==(const Money &) const = default;

    /// Orders this amount against `other`, which must share its currency.
    ///
    /// Cross-currency ordering is not offered at any price, and the reason is
    /// stronger than imprecision. Converting into one operand's currency
    /// rounds only that side, so the answer depends on which operand the call
    /// is made from - `a.compare(b)` can report *equal* while `b.compare(a)`
    /// reports *less*. That breaks antisymmetry, and a relation that breaks it
    /// is undefined behaviour if it ever reaches `std::sort`. Callers convert
    /// everything into one reporting currency first (CLAUDE.md 6.4).
    ///
    /// Note the deliberate asymmetry with `operator==`: across currencies,
    /// equality answers `false` while this answers an error. "Are these the
    /// same value?" has an answer; "how do these rank?" does not. Neither can
    /// be implemented in terms of the other.
    ///
    /// \return The ordering, or MoneyError::CurrencyMismatch.
    [[nodiscard]] std::expected<std::strong_ordering, MoneyError> compare(const Money &other) const noexcept;

    /// Adds `other` to this amount, which must share its currency.
    ///
    /// The currency check runs first and is absolute. There is deliberately no
    /// rate parameter: a converting addition would round one operand and not
    /// the other, so the result would depend on which side the call was made
    /// from. Callers convert into a single reporting currency first, with an
    /// explicit rate and as-of date (CLAUDE.md 6.4).
    ///
    /// Overflow is not reported, and cannot meaningfully occur. At #scale an
    /// int128_t reaches roughly 1.7e32 currency units - some eighteen orders
    /// of magnitude beyond world GDP - so the sum of two representable amounts
    /// is itself representable in every case this application can encounter.
    /// That is the whole point of the 128-bit storage decision (CLAUDE.md D8),
    /// and it is why MoneyError::Overflow belongs to #fromDouble alone.
    ///
    /// \param other Amount to add. Must carry the same Currency as this one.
    /// \return The sum, or MoneyError::CurrencyMismatch.
    [[nodiscard]] std::expected<Money, MoneyError> add(const Money &other) const noexcept;

    /// Subtracts `other` from this amount, which must share its currency.
    ///
    /// The currency rule and the absence of an overflow error are exactly as
    /// described on #add, for the same reasons.
    ///
    /// A negative result is returned unchanged and is never clamped to zero.
    /// Negative owner earnings is a real, meaningful figure about a real
    /// business, and hiding it would defeat the purpose of computing it
    /// (CLAUDE.md 6.4).
    ///
    /// \param other Amount to subtract. Must carry the same Currency as this one.
    /// \return The difference, or MoneyError::CurrencyMismatch.
    [[nodiscard]] std::expected<Money, MoneyError> subtract(const Money &other) const noexcept;

    /// The same amount with its sign flipped, in the same currency.
    ///
    /// Returns a plain Money rather than `std::expected`. There is no second
    /// operand here, so there is no currency to disagree with and nothing that
    /// can fail; per A7 a function that cannot fail says so by not returning
    /// `expected`. Obtaining negation from #subtract instead would hand every
    /// caller an error that is unreachable by construction.
    ///
    /// That return type is also what lets negation compose with #sum. An
    /// `expected` cannot sit in a braced list, so `sum({income, capex.negate()})`
    /// compiles only because this yields a Money. Subtractive terms of an
    /// owner-earnings expression enter the sum this way (CLAUDE.md 6.1).
    ///
    /// \warning Negating the smallest representable amount is undefined. The
    ///          two's-complement range of int128_t is asymmetric, so that
    ///          value has no positive counterpart. It sits near -1.7e32
    ///          currency units and is unreachable from any real figure, but it
    ///          is the one input this function cannot honour.
    /// \return This amount negated.
    [[nodiscard]] Money negate() const noexcept;

    /// Adds every element of `others` to this amount.
    ///
    /// This amount is the first term *and* the currency authority: the result
    /// carries this Currency, every element must match it, and summing an
    /// empty range returns `*this` rather than an error. Being a member is
    /// what makes that well-defined - a free function would need a separate
    /// reporting currency to say what the sum of nothing is.
    ///
    /// Exists because owner earnings is a five-term expression (CLAUDE.md 6.1)
    /// and chaining #add through `std::expected` nests it four deep. A single
    /// call reads the way the definition is written. Subtractive terms enter
    /// through #negate.
    ///
    /// The first mismatching element ends the traversal and later elements are
    /// not examined. This is why the implementation is a loop rather than a
    /// fold: neither `std::accumulate` nor `std::ranges::fold_left` can
    /// short-circuit, so either would have to traverse the whole range or make
    /// two passes over it.
    ///
    /// \param others Contiguous sequence of amounts, all in this Currency. A
    ///               std::vector, std::array, C array or subspan binds without
    ///               copying or allocating. For a braced list, see the
    ///               overload taking `std::initializer_list`.
    /// \return The total, or MoneyError::CurrencyMismatch.
    [[nodiscard]] std::expected<Money, MoneyError> sum(std::span<const Money> others) const noexcept;

    /// Adds every element of a braced list to this amount.
    ///
    /// A convenience over the `std::span` overload, which holds the logic and
    /// documents the semantics. It exists because a `std::span` cannot be
    /// constructed from a braced list - verified to fail on this toolchain
    /// under both `-std=c++23` and `-std=c++26` - so `sum({a, b, c})` needs an
    /// overload of its own. The two accept disjoint argument forms, so no call
    /// between them is ambiguous, including `sum({})`.
    ///
    /// The elements live in an array the compiler materialises with automatic
    /// storage duration, alive for the whole call, so this allocates nothing
    /// and copies nothing.
    ///
    /// \param others Amounts to add, all in this Currency.
    /// \return The total, or MoneyError::CurrencyMismatch.
    [[nodiscard]] std::expected<Money, MoneyError> sum(std::initializer_list<Money> others) const noexcept;

private:
    /// Total: any integer paired with a valid Currency is a valid Money.
    Money(int128_t amount, Currency currency)
        : m_scaledAmount(amount)
        , m_currency(currency)
    {
    }

    int128_t m_scaledAmount;
    Currency m_currency;
};

} // namespace domain
