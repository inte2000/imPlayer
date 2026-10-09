#ifndef __WIN_SOCK_ENV_INIT_H__
#define __WIN_SOCK_ENV_INIT_H__

#pragma once

#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")

class WinSockEnv final
{
public:
    WinSockEnv(int majorVer = 2, int minorVer = 2)
    {
        WSADATA wsaData = { 0 };
        int err = ::WSAStartup(MAKEWORD(minorVer, majorVer), &wsaData);
        m_status = (err == 0);
    }
    ~WinSockEnv()
    {
        Release();
    }
    void Release()
    {
        if (m_status)
        {
            ::WSACleanup();
            m_status = false;
        }
    }
    operator bool() { return m_status; }

    WinSockEnv(const WinSockEnv&) = delete;
    WinSockEnv(WinSockEnv&&) = delete;
    WinSockEnv& operator = (const WinSockEnv&) = delete;
    WinSockEnv& operator = (WinSockEnv&&) = delete;

private:
    bool m_status;
};

#if 0
int main()
{
    WinSockInit sockinit;
    if (!sockinit)
    {
        //¥ÌŒÛ¥¶¿Ì
    }
}
#endif 

#endif //__WIN_SOCK_ENV_INIT_H__

