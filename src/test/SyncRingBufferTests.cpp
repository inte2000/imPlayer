#include <catch2/catch_test_macros.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <thread>

#include "SyncRingBuffer.h"

TEST_CASE("CSyncRingBuffer constructor and capacity/reset work", "[core][ringbuffer]")
{
    CSyncRingBuffer buffer;
    CHECK(buffer.Capacity() == 0);

    buffer.Reset(8);
    CHECK(buffer.Capacity() == 8);
}

TEST_CASE("CSyncRingBuffer write/read supports wrap-around order", "[core][ringbuffer]")
{
    CSyncRingBuffer buffer(5);
    std::array<uint8_t, 5> in1 = {1, 2, 3, 4, 5};
    std::array<uint8_t, 3> out1{};
    std::array<uint8_t, 3> in2 = {6, 7, 8};
    std::array<uint8_t, 5> out2{};

    REQUIRE(buffer.Write(in1.data(), in1.size()) == in1.size());
    REQUIRE(buffer.Read(out1.data(), out1.size(), 1) == out1.size());
    CHECK(out1 == std::array<uint8_t, 3>{1, 2, 3});

    REQUIRE(buffer.Write(in2.data(), in2.size()) == in2.size());
    REQUIRE(buffer.Read(out2.data(), out2.size(), 1) == out2.size());
    CHECK(out2 == std::array<uint8_t, 5>{4, 5, 6, 7, 8});
}

TEST_CASE("CSyncRingBuffer clear empties pending bytes", "[core][ringbuffer]")
{
    CSyncRingBuffer buffer(8);
    std::array<uint8_t, 4> input = {10, 11, 12, 13};
    std::array<uint8_t, 4> output{};

    REQUIRE(buffer.Write(input.data(), input.size()) == input.size());
    buffer.Clear();
    CHECK(buffer.Read(output.data(), output.size(), 1) == 0);
}

TEST_CASE("CSyncRingBuffer read times out when no data available", "[core][ringbuffer]")
{
    CSyncRingBuffer buffer(16);
    std::array<uint8_t, 8> output{};

    const auto start = std::chrono::steady_clock::now();
    CHECK(buffer.Read(output.data(), output.size(), 1) == 0);
    const auto elapsed = std::chrono::steady_clock::now() - start;
    CHECK(elapsed >= std::chrono::milliseconds(900));
}

TEST_CASE("CSyncRingBuffer SetProducerFinished unblocks waiting read", "[core][ringbuffer]")
{
    CSyncRingBuffer buffer(8);
    std::array<uint8_t, 4> output{};
    std::atomic<std::size_t> readSize = 999;

    std::thread reader([&]() {
        readSize.store(buffer.Read(output.data(), output.size(), 0));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    buffer.SetProducerFinished();
    reader.join();

    CHECK(readSize.load() == 0);
}

TEST_CASE("CSyncRingBuffer SetProducerFinished returns partial readable bytes", "[core][ringbuffer]")
{
    CSyncRingBuffer buffer(8);
    std::array<uint8_t, 2> input = {21, 22};
    std::array<uint8_t, 4> output{};

    REQUIRE(buffer.Write(input.data(), input.size()) == input.size());
    buffer.SetProducerFinished();

    CHECK(buffer.Read(output.data(), output.size(), 0) == input.size());
    CHECK(output[0] == 21);
    CHECK(output[1] == 22);
}

TEST_CASE("CSyncRingBuffer Close unblocks read and then disables read/write", "[core][ringbuffer]")
{
    CSyncRingBuffer buffer(8);
    std::array<uint8_t, 4> output{};
    std::array<uint8_t, 4> input = {31, 32, 33, 34};
    std::atomic<std::size_t> blockedReadSize = 999;

    std::thread reader([&]() {
        blockedReadSize.store(buffer.Read(output.data(), output.size(), 0));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    buffer.Close();
    reader.join();

    CHECK(blockedReadSize.load() == 0);
    CHECK(buffer.Read(output.data(), output.size(), 1) == 0);
    CHECK(buffer.Write(input.data(), input.size()) == 0);
}

TEST_CASE("CSyncRingBuffer Close unblocks blocked write", "[core][ringbuffer]")
{
    CSyncRingBuffer buffer(4);
    std::array<uint8_t, 4> fill = {41, 42, 43, 44};
    std::array<uint8_t, 2> blockedInput = {45, 46};
    std::atomic<std::size_t> blockedWriteSize = 999;

    REQUIRE(buffer.Write(fill.data(), fill.size()) == fill.size());

    std::thread writer([&]() {
        blockedWriteSize.store(buffer.Write(blockedInput.data(), blockedInput.size()));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    buffer.Close();
    writer.join();

    CHECK(blockedWriteSize.load() == 0);
}
