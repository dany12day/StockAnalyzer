#include <domain/ToolchainProbe.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using domain::ProbeError;
using domain::ToolchainProbe;

TEST_CASE("create accepts valid input", "[domain][expected]")
{
    const auto probe = ToolchainProbe::create("VWCE");

    REQUIRE(probe.has_value());
    REQUIRE(probe->text() == "VWCE");
}

TEST_CASE("create reports why it rejected the input", "[domain][expected]")
{
    SECTION("empty")
    {
        const auto probe = ToolchainProbe::create("");

        REQUIRE_FALSE(probe.has_value());
        REQUIRE(probe.error() == ProbeError::Empty);
    }

    SECTION("too long")
    {
        const auto probe = ToolchainProbe::create(std::string(33, 'X'));

        REQUIRE_FALSE(probe.has_value());
        REQUIRE(probe.error() == ProbeError::TooLong);
    }
}

TEST_CASE("pairwiseSums truncates to the shorter sequence", "[domain][ranges]")
{
    const std::vector<int> lhs{1, 2, 3};
    const std::vector<int> rhs{10, 20};

    REQUIRE(ToolchainProbe::pairwiseSums(lhs, rhs) == std::vector<int>{11, 22});
}
