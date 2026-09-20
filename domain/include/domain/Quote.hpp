#pragma once

#include <chrono>
#include <domain/Money.hpp>
#include <domain/Symbol.hpp>

namespace domain
{
class Quote
{
public:
    explicit Quote(Symbol symbol, Money price, std::chrono::sys_seconds regularMarketTime);

private:
    Symbol m_symbol;
    Money m_price;
    std::chrono::sys_seconds m_regularMarketTime;
};

} // namespace domain