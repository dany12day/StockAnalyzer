#include "domain/Money.hpp"

namespace domain
{

std::expected<std::strong_ordering, MoneyError> Money::compare(const Money &other) const noexcept
{
    // The currency check comes first and is absolute: there is deliberately no
    // rate parameter here. See the declaration for why a converting comparison
    // cannot be made coherent.
    if (m_currency != other.getCurrency())
    {
        return std::unexpected(MoneyError::CurrencyMismatch);
    }

    // int128_t is an integral type, so <=> yields std::strong_ordering
    // directly - verified on both desktop GCC and the NDK's clang.
    return m_scaledAmount <=> other.m_scaledAmount;
}

std::expected<Money, MoneyError> Money::add(const Money &other) const noexcept
{
    if (m_currency != other.getCurrency())
    {
        return std::unexpected(MoneyError::CurrencyMismatch);
    }

    return Money::fromScaled(m_scaledAmount + other.getScaledAmount(), m_currency);
}

std::expected<Money, MoneyError> Money::subtract(const Money &other) const noexcept
{
    if (m_currency != other.getCurrency())
    {
        return std::unexpected(MoneyError::CurrencyMismatch);
    }

    return Money::fromScaled(m_scaledAmount - other.getScaledAmount(), m_currency);
}

Money Money::negate() const noexcept
{
    return Money::fromScaled(-m_scaledAmount, m_currency);
}

std::expected<Money, MoneyError> Money::sum(std::span<const Money> others) const noexcept
{
    int128_t sum{m_scaledAmount};

    for (const auto &element : others)
    {
        if (m_currency != element.getCurrency())
        {
            return std::unexpected(MoneyError::CurrencyMismatch);
        }

        sum += element.getScaledAmount();
    }

    return Money::fromScaled(sum, m_currency);
}

std::expected<Money, MoneyError> Money::sum(std::initializer_list<Money> others) const noexcept
{
    return sum(std::span<const Money>(others.begin(), others.size()));
}

} // namespace domain
