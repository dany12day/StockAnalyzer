#include <domain/Money.hpp>

#include "StringMakers.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <array>
#include <cmath>
#include <limits>
#include <span>
#include <string_view>
#include <vector>

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

TEST_CASE("add totals two amounts in the same currency", "[domain][money]")
{
    const auto usd = makeCurrency("USD");

    const auto total = Money::fromUnits(2, usd).add(Money::fromUnits(3, usd));

    REQUIRE(total.has_value());
    REQUIRE(*total == Money::fromUnits(5, usd));
}

TEST_CASE("subtract takes one amount from another in the same currency", "[domain][money]")
{
    const auto usd = makeCurrency("USD");

    const auto difference = Money::fromUnits(3, usd).subtract(Money::fromUnits(2, usd));

    REQUIRE(difference.has_value());
    REQUIRE(*difference == Money::fromUnits(1, usd));
}

TEST_CASE("arithmetic preserves the scale", "[domain][money][regression]")
{
    // Guards a factor-of-a-million slip. Both operands are already scaled, so
    // building the result with fromUnits rather than fromScaled multiplies by
    // #scaledFactor a second time and every answer comes out 1e6 too large.
    // The assertions are deliberately written in storage units, because that
    // is where such an error is visible; comparing two Money values built the
    // same wrong way would agree with itself and prove nothing.
    const auto usd = makeCurrency("USD");

    const auto two = Money::fromUnits(2, usd);
    const auto three = Money::fromUnits(3, usd);

    REQUIRE(two.add(three)->getScaledAmount() == 5000000);
    REQUIRE(two.subtract(three)->getScaledAmount() == -1000000);
    REQUIRE(two.negate().getScaledAmount() == -2000000);
    REQUIRE(two.sum({three, three})->getScaledAmount() == 8000000);
}

TEST_CASE("arithmetic carries sub-unit amounts", "[domain][money]")
{
    // Whole units alone would not catch a scale error that happens to cancel.
    const auto usd = makeCurrency("USD");

    const auto price = Money::fromDouble(12.50, usd);
    const auto fee = Money::fromDouble(0.25, usd);
    REQUIRE(price.has_value());
    REQUIRE(fee.has_value());

    REQUIRE(price->add(*fee)->getScaledAmount() == 12750000);
    REQUIRE(price->subtract(*fee)->getScaledAmount() == 12250000);
}

TEST_CASE("add and subtract return negative results unchanged", "[domain][money]")
{
    // Negative owner earnings is a real result and is never clamped
    // (CLAUDE.md 6.4).
    const auto usd = makeCurrency("USD");

    const auto deficit = Money::fromUnits(2, usd).subtract(Money::fromUnits(5, usd));

    REQUIRE(deficit.has_value());
    REQUIRE(*deficit == Money::fromUnits(-3, usd));
}

TEST_CASE("add and subtract hold statement-scale magnitudes", "[domain][money]")
{
    // Two Toyota-sized revenues added together stay exact. At int64 this would
    // have wrapped; the 128-bit storage is what makes overflow a non-concern
    // for addition (CLAUDE.md D8), which is why neither operation reports it.
    const auto jpy = makeCurrency("JPY");

    const auto revenue = Money::fromUnits(45100000000000LL, jpy);
    const auto total = revenue.add(revenue);

    REQUIRE(total.has_value());
    REQUIRE(total->getScaledAmount() == static_cast<domain::int128_t>(90200000000000LL) * Money::scaledFactor);
}

TEST_CASE("add and subtract refuse to combine different currencies", "[domain][money][decision]")
{
    // Currency is a runtime member, so mixing compiles and is caught as a
    // value rather than by the type system (CLAUDE.md D7). This test is the
    // check that replaces the compile error.
    const auto usd = makeCurrency("USD");
    const auto eur = makeCurrency("EUR");

    const auto sum = Money::fromUnits(5, usd).add(Money::fromUnits(5, eur));
    const auto difference = Money::fromUnits(5, usd).subtract(Money::fromUnits(5, eur));

    REQUIRE_FALSE(sum.has_value());
    REQUIRE(sum.error() == MoneyError::CurrencyMismatch);
    REQUIRE_FALSE(difference.has_value());
    REQUIRE(difference.error() == MoneyError::CurrencyMismatch);
}

TEST_CASE("negate flips the sign and keeps the currency", "[domain][money]")
{
    const auto eur = makeCurrency("EUR");

    REQUIRE(Money::fromUnits(5, eur).negate() == Money::fromUnits(-5, eur));
    REQUIRE(Money::fromUnits(-5, eur).negate() == Money::fromUnits(5, eur));
    REQUIRE(Money::fromUnits(5, eur).negate().getCurrency() == eur);
}

TEST_CASE("negate leaves zero alone", "[domain][money]")
{
    // There is no negative zero here: the amount is an integer, not a float.
    const auto usd = makeCurrency("USD");

    REQUIRE(Money::fromUnits(0, usd).negate() == Money::fromUnits(0, usd));
    REQUIRE(Money::fromUnits(0, usd).negate().getScaledAmount() == 0);
}

TEST_CASE("negate is its own inverse", "[domain][money]")
{
    const auto usd = makeCurrency("USD");
    const auto units = GENERATE(-1000, -7, -1, 0, 1, 7, 1000);
    CAPTURE(units);

    const auto money = Money::fromUnits(units, usd);

    REQUIRE(money.negate().negate() == money);
}

TEST_CASE("sum totals a braced list onto the receiver", "[domain][money]")
{
    // The receiver is the first term, not merely the caller.
    const auto usd = makeCurrency("USD");

    const auto total = Money::fromUnits(1, usd).sum({Money::fromUnits(2, usd), Money::fromUnits(3, usd)});

    REQUIRE(total.has_value());
    REQUIRE(*total == Money::fromUnits(6, usd));
}

TEST_CASE("sum of an empty list is the receiver", "[domain][money]")
{
    // Being a member is what gives this an answer: the receiver supplies both
    // the currency and the identity, so no separate reporting currency is
    // needed and the empty case is not an error. An empty portfolio totals to
    // zero, not to a failure.
    const auto usd = makeCurrency("USD");

    const auto five = Money::fromUnits(5, usd);
    const auto zero = Money::fromUnits(0, usd);

    REQUIRE(five.sum({}).has_value());
    REQUIRE(*five.sum({}) == five);
    REQUIRE(*zero.sum({}) == zero);
    REQUIRE(zero.sum({})->getCurrency() == usd);
}

TEST_CASE("sum reads any contiguous sequence", "[domain][money]")
{
    // The span overload is the general one: it binds a vector, an array or a
    // subspan without copying. This is the form the portfolio needs, where the
    // terms are a runtime-sized collection (CLAUDE.md 6.3).
    const auto usd = makeCurrency("USD");

    const auto one = Money::fromUnits(1, usd);
    const auto two = Money::fromUnits(2, usd);
    const auto three = Money::fromUnits(3, usd);

    const std::vector<Money> asVector{two, three};
    const std::array<Money, 2> asArray{two, three};

    REQUIRE(*one.sum(asVector) == Money::fromUnits(6, usd));
    REQUIRE(*one.sum(asArray) == Money::fromUnits(6, usd));
    REQUIRE(*one.sum(std::span<const Money>(asVector).subspan(1)) == Money::fromUnits(4, usd));
}

TEST_CASE("sum agrees with repeated add", "[domain][money]")
{
    // sum exists for readability, not for different arithmetic. If these ever
    // disagree, one of them is wrong.
    const auto usd = makeCurrency("USD");

    const auto first = Money::fromUnits(11, usd);
    const auto second = Money::fromUnits(-4, usd);
    const auto third = Money::fromUnits(7, usd);

    const auto chained = first.add(second).and_then([&third](const Money &partial) { return partial.add(third); });

    REQUIRE(chained.has_value());
    REQUIRE(*first.sum({second, third}) == *chained);
}

TEST_CASE("sum rejects a list holding any foreign currency", "[domain][money]")
{
    // Checked at every position, so a mismatch cannot hide behind valid
    // neighbours at either end of the range.
    const auto usd = makeCurrency("USD");
    const auto eur = makeCurrency("EUR");

    const auto receiver = Money::fromUnits(1, usd);
    const auto native = Money::fromUnits(2, usd);
    const auto foreign = Money::fromUnits(2, eur);

    const auto atFirst = receiver.sum({foreign, native, native});
    const auto inMiddle = receiver.sum({native, foreign, native});
    const auto atLast = receiver.sum({native, native, foreign});

    REQUIRE_FALSE(atFirst.has_value());
    REQUIRE(atFirst.error() == MoneyError::CurrencyMismatch);
    REQUIRE_FALSE(inMiddle.has_value());
    REQUIRE(inMiddle.error() == MoneyError::CurrencyMismatch);
    REQUIRE_FALSE(atLast.has_value());
    REQUIRE(atLast.error() == MoneyError::CurrencyMismatch);
}

TEST_CASE("sum and negate express a mixed additive expression", "[domain][money]")
{
    // The shape owner earnings has: additive terms plus subtractive ones, in a
    // single flat call rather than a chain (CLAUDE.md 6.1). negate returning a
    // plain Money rather than an expected is what lets the subtractive terms
    // sit in the braced list at all.
    //
    // The figures here are arbitrary. A worked example from a named filing
    // waits on the maintenance-capex sign convention, which is still open.
    const auto usd = makeCurrency("USD");

    const auto netIncome = Money::fromUnits(1000, usd);
    const auto depreciation = Money::fromUnits(200, usd);
    const auto maintenanceCapex = Money::fromUnits(150, usd);
    const auto workingCapital = Money::fromUnits(50, usd);

    const auto ownerEarnings = netIncome.sum({depreciation, maintenanceCapex.negate(), workingCapital.negate()});

    REQUIRE(ownerEarnings.has_value());
    REQUIRE(*ownerEarnings == Money::fromUnits(1000, usd));
}

TEST_CASE("add is commutative and subtract is add of negate", "[domain][money]")
{
    // Exhaustive over a small grid, in the style of the compare laws above.
    // The second law is the one that ties negate to subtract: if they ever
    // disagree, negate is not the operation its name claims.
    const auto usd = makeCurrency("USD");

    for (int left = -5; left <= 5; ++left)
    {
        for (int right = -5; right <= 5; ++right)
        {
            CAPTURE(left, right);

            const auto lhs = Money::fromUnits(left, usd);
            const auto rhs = Money::fromUnits(right, usd);

            const auto forward = lhs.add(rhs);
            const auto backward = rhs.add(lhs);
            REQUIRE(forward.has_value());
            REQUIRE(backward.has_value());
            REQUIRE(*forward == *backward);

            const auto difference = lhs.subtract(rhs);
            const auto viaNegate = lhs.add(rhs.negate());
            REQUIRE(difference.has_value());
            REQUIRE(viaNegate.has_value());
            REQUIRE(*difference == *viaNegate);
        }
    }
}
