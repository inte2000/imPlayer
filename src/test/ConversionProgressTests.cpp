#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <limits>
#include <sstream>

#include "ConversionProgress.h"

TEST_CASE("Conversion progress tracks streams and completes only after finalization", "[convert][progress]")
{
    std::ostringstream output;
    CConversionProgress progress(output, 2, std::chrono::milliseconds(0));
    progress.Update(0, 0, 100);
    REQUIRE(output.str().find("  0% (stream 1/2)") != std::string::npos);
    progress.Update(0, 100, 100);
    REQUIRE(output.str().find(" 50% (stream 1/2)") != std::string::npos);
    progress.Update(1, 100, 100);
    REQUIRE(output.str().find(" 99% (stream 2/2)") != std::string::npos);
    REQUIRE(output.str().find("100%") == std::string::npos);
    progress.Finish();
    REQUIRE(output.str().find("100% (complete)") != std::string::npos);
    REQUIRE(output.str().back() == '\n');
    const auto completed = output.str();
    progress.Finish();
    progress.Update(1, 0, 100);
    REQUIRE(output.str() == completed);
}

TEST_CASE("Conversion progress handles unknown lengths and interrupted conversions", "[convert][progress]")
{
    std::ostringstream output;
    {
        CConversionProgress progress(output, 1, std::chrono::milliseconds(0));
        progress.Update(0, 0, 0);
        progress.Update(0, 32768, (std::numeric_limits<std::size_t>::max)());
        REQUIRE(output.str().find("32768 frames written") != std::string::npos);
    }
    REQUIRE(output.str().find("100%") == std::string::npos);
    REQUIRE(output.str().back() == '\n');
}

TEST_CASE("Conversion progress throttles updates without hiding completion", "[convert][progress]")
{
    std::ostringstream output;
    CConversionProgress progress(output, 1, std::chrono::hours(1));
    progress.Update(0, 0, 100);
    const auto initial = output.str();
    progress.Update(0, 50, 100);
    REQUIRE(output.str() == initial);
    progress.Finish();
    REQUIRE(output.str().find("100% (complete)") != std::string::npos);
}
