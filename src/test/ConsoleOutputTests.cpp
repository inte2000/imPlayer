#include <catch2/catch_test_macros.hpp>
#include <utility>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "ConsoleOutput.h"
#include "ScopeGuard.h"

TEST_CASE("Console output initialization supports redirected output", "[console][encoding]")
{
#ifdef _WIN32
    const UINT oldOutputCp = GetConsoleOutputCP();
    CScopeGuard restore([=]() {
        if (oldOutputCp != 0) {
            SetConsoleOutputCP(oldOutputCp);
        }
    });
#endif
    REQUIRE(InitConsoleOutput());
    REQUIRE(InitConsoleOutput());
}

#ifdef _WIN32
TEST_CASE("Console output switches legacy code pages to UTF-8", "[console][encoding]")
{
    const HANDLE oldInput = GetStdHandle(STD_INPUT_HANDLE);
    const HANDLE oldOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    const HANDLE oldError = GetStdHandle(STD_ERROR_HANDLE);
    const bool ownsConsole = GetConsoleOutputCP() == 0;
    if (ownsConsole) {
        REQUIRE(AllocConsole() != FALSE);
    }
    const UINT oldOutputCp = GetConsoleOutputCP();
    const UINT oldInputCp = GetConsoleCP();
    const UINT oldAnsiCp = GetACP();
    CScopeGuard restore([=]() {
        SetConsoleOutputCP(oldOutputCp);
        if (ownsConsole) {
            FreeConsole();
            SetStdHandle(STD_INPUT_HANDLE, oldInput);
            SetStdHandle(STD_OUTPUT_HANDLE, oldOutput);
            SetStdHandle(STD_ERROR_HANDLE, oldError);
        }
    });

    for (UINT codePage : {936u, 437u}) {
        REQUIRE(SetConsoleOutputCP(codePage) != FALSE);
        REQUIRE(InitConsoleOutput());
        REQUIRE(GetConsoleOutputCP() == CP_UTF8);
        REQUIRE(GetConsoleCP() == oldInputCp);
        REQUIRE(GetACP() == oldAnsiCp);
    }
}
#endif
