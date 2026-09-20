#include <domain/Currency.hpp>

#include "StringMakers.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <map>
#include <string>
#include <string_view>

using domain::Currency;
using domain::CurrencyError;

TEST_CASE("Currency accepts ISO 4217 alpha-3 codes and keeps the text unchanged", "[domain][currency]")
{
    const auto text = GENERATE(as<std::string>{}, "USD", "EUR", "JPY", "GBP", "CHF", "KRW");
    CAPTURE(text);

    const auto currency = Currency::create(text);

    REQUIRE(currency.has_value());
    REQUIRE(currency->getCurrency() == text);
}

TEST_CASE("Currency requires exactly three characters", "[domain][currency]")
{
    // One error covers empty, short and long: unlike Symbol, the length is
    // fixed rather than bounded, so "too short" and "too long" are the same
    // mistake.
    const auto text = GENERATE(as<std::string>{}, "", "U", "US", "USDX", "USDOLLAR");
    CAPTURE(text);

    const auto currency = Currency::create(text);

    REQUIRE_FALSE(currency.has_value());
    REQUIRE(currency.error() == CurrencyError::InvalidLength);
}

TEST_CASE("Currency rejects characters outside A-Z", "[domain][currency]")
{
    SECTION("digits and punctuation")
    {
        const auto text = GENERATE(as<std::string>{}, "US1", "U$D", "U-D", "U.D", "US_");
        CAPTURE(text);

        const auto currency = Currency::create(text);

        REQUIRE_FALSE(currency.has_value());
        REQUIRE(currency.error() == CurrencyError::InvalidCharacter);
    }

    SECTION("whitespace, typically from pasted or padded text")
    {
        const auto text = GENERATE(as<std::string>{}, " SD", "US ", "U D", "US\n", "US\t");
        CAPTURE(text);

        const auto currency = Currency::create(text);

        REQUIRE_FALSE(currency.has_value());
        REQUIRE(currency.error() == CurrencyError::InvalidCharacter);
    }

    SECTION("embedded NUL")
    {
        // Built with an explicit length: a literal would stop at the NUL.
        const auto currency = Currency::create(std::string_view("U\0D", 3));

        REQUIRE_FALSE(currency.has_value());
        REQUIRE(currency.error() == CurrencyError::InvalidCharacter);
    }

    SECTION("non-ASCII bytes, negative as signed char on x86-64")
    {
        // "ÄD" in UTF-8 is three bytes, so it passes the length check and
        // reaches the character check with two negative values. This case is
        // why the check is an explicit 'A'-'Z' range rather than std::isalpha,
        // which has undefined behaviour for negative arguments.
        const auto currency = Currency::create("\xC3\x84"
                                               "D");

        REQUIRE_FALSE(currency.has_value());
        REQUIRE(currency.error() == CurrencyError::InvalidCharacter);
    }
}

TEST_CASE("Currency checks length before characters", "[domain][currency]")
{
    // "u$" is both the wrong length and the wrong characters. Length wins,
    // matching the order Symbol::create reports its failures in.
    const auto currency = Currency::create("u$");

    REQUIRE_FALSE(currency.has_value());
    REQUIRE(currency.error() == CurrencyError::InvalidLength);
}

// The next two pin decisions rather than defects. If either decision changes,
// change the test in the same commit.
TEST_CASE("Currency is uppercase-only; callers normalise before create", "[domain][currency][decision]")
{
    // Matches Symbol's rule deliberately. Two adjacent value types with
    // opposite normalisation behaviour would be a trap.
    const auto text = GENERATE(as<std::string>{}, "usd", "Usd", "usD");
    CAPTURE(text);

    const auto currency = Currency::create(text);

    REQUIRE_FALSE(currency.has_value());
    REQUIRE(currency.error() == CurrencyError::InvalidCharacter);
}

TEST_CASE("Currency validates shape only, not membership of ISO 4217", "[domain][currency][decision]")
{
    // A hardcoded list of real codes would go stale as currencies are added
    // and redenominated, and the data source dictates the codes in practice.
    // The accepted cost is that well-formed nonsense validates.
    const auto text = GENERATE(as<std::string>{}, "XYZ", "AAA", "ZZZ");
    CAPTURE(text);

    REQUIRE(Currency::create(text).has_value());
}

TEST_CASE("Currency compares by code", "[domain][currency]")
{
    const auto usd = Currency::create("USD");
    const auto alsoUsd = Currency::create("USD");
    const auto eur = Currency::create("EUR");

    REQUIRE(usd.has_value());
    REQUIRE(alsoUsd.has_value());
    REQUIRE(eur.has_value());

    SECTION("equality")
    {
        REQUIRE(*usd == *alsoUsd);
        REQUIRE(*usd != *eur);
    }

    SECTION("ordering makes Currency usable as a map key")
    {
        // The ordering is lexicographic over the bytes and carries no
        // financial meaning. It exists so the rate table can key on Currency.
        std::map<Currency, int> byCurrency;
        byCurrency[*usd] = 1;
        byCurrency[*eur] = 2;
        byCurrency[*alsoUsd] = 3;

        REQUIRE(byCurrency.size() == 2);
        REQUIRE(byCurrency.at(*usd) == 3);
    }
}

TEST_CASE("Currency exposes exactly three characters with no terminator", "[domain][currency]")
{
    const auto currency = Currency::create("USD");
    REQUIRE(currency.has_value());

    const auto code = currency->getCurrency();

    // The view is built from an explicit length. The underlying array holds no
    // NUL, so anything that infers the length from the data would read past
    // the end.
    REQUIRE(code.size() == Currency::length);
    REQUIRE(code == "USD");
}
