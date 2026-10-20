#pragma once

// 在首次输出前调用。cout/cerr/clog 和重定向输出均使用 UTF-8。
// Windows 设置控制台输出代码页；没有控制台时和 Linux 上无需设置。
// 不修改输入代码页、系统 ANSI 代码页或 C/C++ locale。
bool InitConsoleOutput() noexcept;
