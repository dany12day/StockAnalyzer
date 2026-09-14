#include <domain/Symbol.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <string>
#include <string_view>

using domain::Symbol;
using domain::SymbolError;

TEST_CASE("Symbol accepts real tickers and keeps the text unchanged", "[domain][symbol]")
{
    const auto text = GENERATE(as<std::string>{},
                               "F",       // Ford: one character
                               "GE",      // two characters
                               "AAPL",    // the common case
                               "BRK-B",   // share class
                               "VOW3.DE", // exchange suffix, digit in the root
                               "0700.HK"  // numeric root
    );
    CAPTURE(text);

    const auto symbol = Symbol::create(text);

    REQUIRE(symbol.has_value());
    REQUIRE(symbol->getSymbol() == text);
}

TEST_CASE("Symbol rejects empty text", "[domain][symbol]")
{
    const auto symbol = Symbol::create("");

    REQUIRE_FALSE(symbol.has_value());
    REQUIRE(symbol.error() == SymbolError::Empty);
}

TEST_CASE("Symbol length limit is inclusive", "[domain][symbol]")
{
    SECTION("exactly maxLength is accepted")
    {
        REQUIRE(Symbol::create(std::string(Symbol::maxLength, 'A')).has_value());
    }

    SECTION("one over maxLength is rejected")
    {
        const auto symbol = Symbol::create(std::string(Symbol::maxLength + 1, 'A'));

        REQUIRE_FALSE(symbol.has_value());
        REQUIRE(symbol.error() == SymbolError::TooLong);
    }
}

TEST_CASE("Symbol rejects characters outside the allowlist", "[domain][symbol]")
{
    SECTION("text that would change the request URL")
    {
        const auto text = GENERATE(as<std::string>{}, "AAPL?range=max", "AAPL&x", "AAPL/x", "AAPL#x", "AAPL%3F",
                                   "AAPL\\x", "AAPL:x", "AAPL;x", "AAPL@x");
        CAPTURE(text);

        const auto symbol = Symbol::create(text);

        REQUIRE_FALSE(symbol.has_value());
        REQUIRE(symbol.error() == SymbolError::InvalidCharacter);
    }

    SECTION("whitespace and control characters, typically from pasted text")
    {
        const auto text = GENERATE(as<std::string>{}, " AAPL", "AAPL ", "AA PL", "AAPL\n", "AAPL\t");
        CAPTURE(text);

        const auto symbol = Symbol::create(text);

        REQUIRE_FALSE(symbol.has_value());
        REQUIRE(symbol.error() == SymbolError::InvalidCharacter);
    }

    SECTION("embedded NUL")
    {
        // Built with an explicit length: a literal would stop at the NUL.
        const auto symbol = Symbol::create(std::string_view("AA\0PL", 5));

        REQUIRE_FALSE(symbol.has_value());
        REQUIRE(symbol.error() == SymbolError::InvalidCharacter);
    }

    SECTION("non-ASCII bytes, negative as signed char on x86-64")
    {
        // "ÄAPL" in UTF-8. The literal is split so \x84 does not absorb the "A".
        const auto symbol = Symbol::create("\xC3\x84"
                                           "APL");

        REQUIRE_FALSE(symbol.has_value());
        REQUIRE(symbol.error() == SymbolError::InvalidCharacter);
    }
}

// The next two pin decisions rather than defects. If either decision changes,
// change the test in the same commit.
TEST_CASE("Symbol is uppercase-only; callers normalise before create", "[domain][symbol][decision]")
{
    const auto text = GENERATE(as<std::string>{}, "aapl", "Aapl", "brk-b");
    CAPTURE(text);

    const auto symbol = Symbol::create(text);

    REQUIRE_FALSE(symbol.has_value());
    REQUIRE(symbol.error() == SymbolError::InvalidCharacter);
}

TEST_CASE("Symbol excludes indices, FX pairs and futures", "[domain][symbol][decision]")
{
    // Not businesses (CLAUDE.md section 1). A currency rate needed for 6.4 gets
    // its own type rather than travelling as a Symbol.
    const auto text = GENERATE(as<std::string>{}, "^GSPC", "EURUSD=X", "GC=F");
    CAPTURE(text);

    const auto symbol = Symbol::create(text);

    REQUIRE_FALSE(symbol.has_value());
    REQUIRE(symbol.error() == SymbolError::InvalidCharacter);
}

TEST_CASE("Symbol checks length before characters", "[domain][symbol]")
{
    const auto symbol = Symbol::create(std::string(Symbol::maxLength + 1, '?'));

    REQUIRE_FALSE(symbol.has_value());
    REQUIRE(symbol.error() == SymbolError::TooLong);
}
