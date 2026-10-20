#ifdef _WIN32
#include <Windows.h>
#endif

#include "ConsoleOutput.h"

bool InitConsoleOutput() noexcept
{
#ifdef _WIN32
    // 无控制台的文件/管道输出保留 UTF-8 字节，不做本地编码转换。
    if (GetConsoleOutputCP() != 0) {
        return SetConsoleOutputCP(CP_UTF8) != FALSE;
    }
#endif
    // Linux 的 UTF-8 终端直接接收窄字符流，无需 Windows 代码页设置。
    return true;
}
