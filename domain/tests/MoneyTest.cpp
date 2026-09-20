#include <domain/Money.hpp>

#include "StringMakers.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <limits>
#include <string_view>

using domain::Currency;
using domain::Money;
using domain::MoneyError;

namespace
{

/// Builds a Currency or fails the test. Keeps the money assertions readable.
Currency makeCurrency(std::string_view code)
{
    const auto currency = Currency::create(code);
    REQUIRE(currency.has_value());
    return *currency;
}

} // namespace

TEST_CASE("Money's scale is six decimal places", "[domain][money]")
{
    STATIC_REQUIRE(Money::scale == 6);
    STATIC_REQUIRE(Money::scaledFactor == 1000000);
}

TEST_CASE("fromUnits multiplies by the scale factor", "[domain][money]")
{
    const auto usd = makeCurrency("USD");

    REQUIRE(Money::fromUnits(0, usd).getScaledAmount() == 0);
    REQUIRE(Money::fromUnits(1, usd).getScaledAmount() == 1000000);
    REQUIRE(Money::fromUnits(100, usd).getScaledAmount() == 100000000);
}

TEST_CASE("fromScaled stores the amount verbatim", "[domain][money]")
{
    const auto usd = makeCurrency("USD");

    REQUIRE(Money::fromScaled(12500000, usd).getScaledAmount() == 12500000);
    REQUIRE(Money::fromScaled(1, usd).getScaledAmount() == 1);
}

TEST_CASE("Money carries its currency", "[domain][money]")
{
    const auto eur = makeCurrency("EUR");

    REQUIRE(Money::fromUnits(5, eur).getCurrency() == eur);
}

TEST_CASE("Money represents negative amounts unchanged", "[domain][money]")
{
    // Negative owner earnings is a real and meaningful result (CLAUDE.md 6.4).
    // It must never be clamped, and the sign must survive every conversion.
    const auto usd = makeCurrency("USD");

    REQUIRE(Money::fromUnits(-5, usd).getScaledAmount() == -5000000);
    REQUIRE(Money::fromScaled(-1, usd).getScaledAmount() == -1);

    const auto fromNegativeDouble = Money::fromDouble(-12.5, usd);
    REQUIRE(fromNegativeDouble.has_value());
    REQUIRE(fromNegativeDouble->getScaledAmount() == -12500000);
}

TEST_CASE("Money holds magnitudes that would overflow a 64-bit integer", "[domain][money]")
{
    // Toyota's revenue is roughly JPY 45.1e12. Scaled by 1e6 that is 4.51e19,
    // which exceeds int64's 9.22e18 ceiling. This is the case that decided the
    // 128-bit representation (CLAUDE.md D8).
    const auto jpy = makeCurrency("JPY");

    const auto revenue = Money::fromUnits(45100000000000LL, jpy);

    REQUIRE(revenue.getScaledAmount() > static_cast<domain::int128_t>(std::numeric_limits<std::int64_t>::max()));
    REQUIRE(revenue.getScaledAmount() == static_cast<domain::int128_t>(45100000000000LL) * Money::scaledFactor);
}

TEST_CASE("fromDouble converts finite values to the nearest scaled amount", "[domain][money]")
{
    const auto usd = makeCurrency("USD");

    SECTION("values representable at scale 6")
    {
        const auto money = Money::fromDouble(12.5, usd);

        REQUIRE(money.has_value());
        REQUIRE(money->getScaledAmount() == 12500000);
    }

    SECTION("rounds to nearest rather than truncating")
    {
        // Truncation toward zero would give 1000000 for both of these, and
        // would round the negative pair the opposite way from the positive.
        REQUIRE(Money::fromDouble(1.0000004, usd)->getScaledAmount() == 1000000);
        REQUIRE(Money::fromDouble(1.0000006, usd)->getScaledAmount() == 1000001);
        REQUIRE(Money::fromDouble(-1.0000004, usd)->getScaledAmount() == -1000000);
        REQUIRE(Money::fromDouble(-1.0000006, usd)->getScaledAmount() == -1000001);
    }

    SECTION("zero")
    {
        const auto money = Money::fromDouble(0.0, usd);

        REQUIRE(money.has_value());
        REQUIRE(money->getScaledAmount() == 0);
    }
}

TEST_CASE("fromDouble rejects values with no representation", "[domain][money]")
{
    const auto usd = makeCurrency("USD");

    SECTION("not a number")
    {
        const auto money = Money::fromDouble(std::numeric_limits<double>::quiet_NaN(), usd);

        REQUIRE_FALSE(money.has_value());
        REQUIRE(money.error() == MoneyError::NotANumber);
    }

    SECTION("infinity")
    {
        const auto positive = Money::fromDouble(std::numeric_limits<double>::infinity(), usd);
        const auto negative = Money::fromDouble(-std::numeric_limits<double>::infinity(), usd);

        REQUIRE_FALSE(positive.has_value());
        REQUIRE(positive.error() == MoneyError::Overflow);
        REQUIRE_FALSE(negative.has_value());
        REQUIRE(negative.error() == MoneyError::Overflow);
    }

    SECTION("finite but far beyond the 128-bit range")
    {
        // 1e300 is finite, so an isfinite check alone would let it through and
        // the cast that follows would be undefined behaviour.
        const auto money = Money::fromDouble(1e300, usd);

        REQUIRE_FALSE(money.has_value());
        REQUIRE(money.error() == MoneyError::Overflow);
    }
}

TEST_CASE("toDouble round-trips values within double's exact integer range", "[domain][money]")
{
    // double holds integers exactly only to 2^53, which at scale 6 is about
    // 9e9 currency units. Beyond that toDouble is lossy by design; it exists
    // for display and charting, never for arithmetic.
    const auto usd = makeCurrency("USD");
    const auto value = GENERATE(0.0, 1.0, 12.5, -12.5, 0.000001, -0.000001, 1234.567891);
    CAPTURE(value);

    const auto money = Money::fromDouble(value, usd);

    REQUIRE(money.has_value());
    REQUIRE_THAT(money->toDouble(), Catch::Matchers::WithinAbs(value, 1e-9));
}

TEST_CASE("Money equality compares amount and currency together", "[domain][money]")
{
    const auto usd = makeCurrency("USD");
    const auto eur = makeCurrency("EUR");

    REQUIRE(Money::fromUnits(5, usd) == Money::fromUnits(5, usd));
    REQUIRE(Money::fromUnits(5, usd) != Money::fromUnits(10, usd));

    // The currency is part of the value. Equal amounts in different currencies
    // are not equal, and no exchange rate is consulted to decide that.
    REQUIRE(Money::fromUnits(5, usd) != Money::fromUnits(5, eur));
}

TEST_CASE("compare orders amounts within a single currency", "[domain][money]")
{
    const auto usd = makeCurrency("USD");

    const auto five = Money::fromUnits(5, usd);
    const auto ten = Money::fromUnits(10, usd);
    const auto alsoFive = Money::fromUnits(5, usd);

    REQUIRE(five.compare(ten) == std::strong_ordering::less);
    REQUIRE(ten.compare(five) == std::strong_ordering::greater);
    REQUIRE(five.compare(alsoFive) == std::strong_ordering::equal);
}

TEST_CASE("compare orders negative amounts correctly", "[domain][money]")
{
    const auto usd = makeCurrency("USD");

    const auto negative = Money::fromUnits(-5, usd);
    const auto zero = Money::fromUnits(0, usd);

    REQUIRE(negative.compare(zero) == std::strong_ordering::less);
    REQUIRE(zero.compare(negative) == std::strong_ordering::greater);
}

TEST_CASE("compare refuses to rank across currencies", "[domain][money][decision]")
{
    // Ordering across currencies is not merely imprecise, it is incoherent:
    // converting into the left operand's currency makes the answer depend on
    // which side the call is made from, because only one side gets rounded.
    // Callers convert to a single reporting currency first (CLAUDE.md 6.4).
    const auto usd = makeCurrency("USD");
    const auto eur = makeCurrency("EUR");

    const auto result = Money::fromUnits(5, usd).compare(Money::fromUnits(5, eur));

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == MoneyError::CurrencyMismatch);
}

TEST_CASE("compare is antisymmetric and agrees with equality", "[domain][money]")
{
    // Exhaustive over a small grid. These are the laws any ordering must obey,
    // and the laws a rate-converting comparison would break.
    const auto usd = makeCurrency("USD");

    for (int left = -5; left <= 5; ++left)
    {
        for (int right = -5; right <= 5; ++right)
        {
            CAPTURE(left, right);

            const auto lhs = Money::fromUnits(left, usd);
            const auto rhs = Money::fromUnits(right, usd);

            const auto forward = lhs.compare(rhs);
            const auto backward = rhs.compare(lhs);
            REQUIRE(forward.has_value());
            REQUIRE(backward.has_value());

            REQUIRE((*forward < 0) == (*backward > 0));
            REQUIRE((*forward == 0) == (*backward == 0));
            REQUIRE((*forward == 0) == (lhs == rhs));
        }
    }
}
