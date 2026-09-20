#include "domain/Currency.hpp"

#include <array>
#include <cstddef>
#include <expected>
#include <string_view>

namespace domain
{

Currency::Currency(std::array<char, 3> currency)
    : m_currency(currency)
{
}

std::expected<Currency, CurrencyError> Currency::create(std::string_view currency)
{
    if (currency.size() != Currency::length)
    {
        return std::unexpected(CurrencyError::InvalidLength);
    }

    // An explicit range check rather than std::isalpha, for two reasons.
    // std::isalpha has undefined behaviour for arguments that are not
    // representable as unsigned char, and a non-ASCII byte such as the first
    // byte of "Ä" arrives here as a negative char on x86-64. It is also
    // locale-dependent and accepts lowercase, which would contradict the
    // uppercase-only rule Symbol already enforces.
    for (char c : currency)
    {
        if (c < 'A' || c > 'Z')
        {
            return std::unexpected(CurrencyError::InvalidCharacter);
        }
    }

    return Currency(std::array<char, 3>{currency[0], currency[1], currency[2]});
}

} // namespace domain
