#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <thread>

#include "NetStream.h"
#include "StreamMateSource.h"

TEST_CASE("CNetStream exposes MateSource and basic non-seek writable behavior", "[core][stream][net]")
{
    CNetStream netStream;

    auto* mateSource = netStream.QuerySource<MateSource>();
    REQUIRE(mateSource != nullptr);

    CHECK(netStream.Write("abc", 3) == 0);
    CHECK(netStream.Tell() == 0);
}

TEST_CASE("CNetStream rejects unsupported protocol and invalid stream type", "[core][stream][net]")
{
    CNetStream netStream;

    CHECK(netStream.Open(L"https://example.com/live", NetStreamType::Http));
    netStream.Close();
    CHECK_FALSE(netStream.Open(L"ftp://example.com/live", NetStreamType::Http));
    CHECK_FALSE(netStream.Open(L"http://example.com/live", static_cast<NetStreamType>(1024)));
}

TEST_CASE("CNetStream marks error state when curl perform fails before streaming", "[core][stream][net]")
{
    CNetStream netStream;

    REQUIRE(netStream.Open(L"http:///", NetStreamType::Http));

    bool hasError = false;
    for (int i = 0; i < 30; ++i) {
        if (netStream.HasError()) {
            hasError = true;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    netStream.Close();

    REQUIRE(hasError);
    CHECK_FALSE(netStream.GetLastErrorMessage().empty());
}
