#include <algorithm>
#include <domain/Symbol.hpp>
#include <utility>

namespace domain
{

Symbol::Symbol(std::string symbol)
    : m_symbol(std::move(symbol))
{
}

std::expected<Symbol, SymbolError> Symbol::create(std::string_view text)
{
    if (text.empty())
    {
        return std::unexpected(SymbolError::Empty);
    }

    if (text.length() > Symbol::maxLength)
    {
        return std::unexpected(SymbolError::TooLong);
    }

    const bool validTicker = std::ranges::all_of(
        text, [](char c) { return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '-'; });

    if (!validTicker)
    {
        return std::unexpected(SymbolError::InvalidCharacter);
    }

    return Symbol(std::string(text));
}

} // namespace domain
