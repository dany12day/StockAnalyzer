#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <domain/Symbol.hpp>

namespace domain
{

struct Currency
{
    std::array<char, 3> currency{};
};

struct Money
{
    std::int64_t amount;
    Currency currency;
    static constexpr std::uint8_t scale = 6; // 6 decimal places
};

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