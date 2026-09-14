#include <domain/Quote.hpp>

#include <utility>

namespace domain
{

Quote::Quote(Symbol symbol, Money price, std::chrono::sys_seconds regularMarketTime)
    : m_symbol(std::move(symbol))
    , m_price(price)
    , m_regularMarketTime(regularMarketTime)
{
}

} // namespace domain
