#include "domain/Money.hpp"

namespace domain
{

std::expected<std::strong_ordering, MoneyError> Money::compare(const Money &other) const noexcept
{
    // The currency check comes first and is absolute: there is deliberately no
    // rate parameter here. See the declaration for why a converting comparison
    // cannot be made coherent.
    if (m_currency != other.m_currency)
    {
        return std::unexpected(MoneyError::CurrencyMismatch);
    }

    // int128_t is an integral type, so <=> yields std::strong_ordering
    // directly - verified on both desktop GCC and the NDK's clang.
    return m_scaledAmount <=> other.m_scaledAmount;
}

} // namespace domain
