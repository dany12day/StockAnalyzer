#include <domain/ToolchainProbe.hpp>

#include <ranges>
#include <tuple>
#include <utility>

namespace domain
{
namespace
{
constexpr std::string_view::size_type kMaxLength = 32;
}

std::expected<ToolchainProbe, ProbeError> ToolchainProbe::create(std::string_view text)
{
    if (text.empty())
    {
        return std::unexpected(ProbeError::Empty);
    }

    if (text.size() > kMaxLength)
    {
        return std::unexpected(ProbeError::TooLong);
    }

    return ToolchainProbe(std::string(text));
}

std::vector<int> ToolchainProbe::pairwiseSums(const std::vector<int> &lhs, const std::vector<int> &rhs)
{
    return std::views::zip(lhs, rhs) |
           std::views::transform([](const auto &pair) { return std::get<0>(pair) + std::get<1>(pair); }) |
           std::ranges::to<std::vector<int>>();
}

ToolchainProbe::ToolchainProbe(std::string text)
    : m_text(std::move(text))
{
}

} // namespace domain
