#pragma once

// Catch2 stringifiers for the domain value types.
//
// Without these, a failed assertion prints "{?}" instead of the value, because
// Catch2 falls back to a placeholder for any type it cannot stream. That
// matters most for domain::int128_t: libstdc++ provides no operator<< for
// __int128 at all (streaming one is ambiguous, not merely unsupported), so
// every money assertion would lose its numbers exactly when you need them.

#include <domain/Currency.hpp>
#include <domain/Money.hpp>
#include <domain/int128.hpp>

#include <catch2/catch_tostring.hpp>

#include <string>

namespace Catch
{

template <> struct StringMaker<domain::int128_t>
{
    static std::string convert(domain::int128_t value)
    {
        if (value == 0)
        {
            return "0";
        }

        // Digits are taken from the value in place rather than from its
        // absolute value: negating the most negative int128 would overflow,
        // so the sign is applied only to each extracted digit.
        const bool negative = value < 0;
        std::string reversed;
        while (value != 0)
        {
            const int digit = static_cast<int>(value % 10);
            reversed.push_back(static_cast<char>('0' + (digit < 0 ? -digit : digit)));
            value /= 10;
        }

        if (negative)
        {
            reversed.push_back('-');
        }

        return std::string(reversed.rbegin(), reversed.rend());
    }
};

template <> struct StringMaker<domain::Currency>
{
    static std::string convert(const domain::Currency &currency)
    {
        const auto code = currency.getCurrency();
        return std::string(code.begin(), code.end());
    }
};

template <> struct StringMaker<domain::Money>
{
    /// Rendered as the raw scaled amount plus the code, e.g. `12500000 USD`.
    /// Deliberately not formatted as a decimal: a test failure should show
    /// exactly what is stored, not a prettied version of it.
    static std::string convert(const domain::Money &money)
    {
        return StringMaker<domain::int128_t>::convert(money.getScaledAmount()) + " " +
               StringMaker<domain::Currency>::convert(money.getCurrency());
    }
};

} // namespace Catch
