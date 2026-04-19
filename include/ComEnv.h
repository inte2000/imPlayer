#ifndef __COM_ENV_INIT_H__
#define __COM_ENV_INIT_H__

#define NOMINMAX
#include <windows.h>

/*
CoInitialize/CoInitializeEx 只支持基础的 COM 服务，要使用拖拽和剪贴板之类的
高级 OLE 功能，需要使用 OleInitialize
*/
class ComEnv final
{
public:
    ComEnv()
    {
        m_isInit = !FAILED(::CoInitialize(NULL));
    }
    ~ComEnv()
    {
        Release();
    }
    void Release()
    {
        if (m_isInit)
        {
            ::CoUninitialize();
            m_isInit = false;
        }
    }
    ComEnv (const ComEnv &) = delete;
    ComEnv(ComEnv &&) = delete;
    ComEnv & operator = (const ComEnv &) = delete;
    ComEnv& operator = (ComEnv &&) = delete;
private:
    bool m_isInit;
};

class ComEnvEx final
{
public:
    ComEnvEx(DWORD apartment)
    {
        m_isInit = (::CoInitializeEx(NULL, apartment) == S_OK) ? true : false;
    }
    ~ComEnvEx()
    {
        Release();
    }
    void Release()
    {
        if (m_isInit)
        {
            ::CoUninitialize();
            m_isInit = false;
        }
    }

    ComEnvEx (const ComEnvEx &) = delete;
    ComEnvEx(ComEnvEx &&) = delete;
    ComEnvEx & operator = (const ComEnvEx &) = delete;
    ComEnvEx& operator = (ComEnvEx &&) = delete;
private:
    bool m_isInit;
};

class OleEnv final
{
public:
    OleEnv()
    {
        m_isInit = !FAILED(::OleInitialize(NULL));
    }

    ~OleEnv()
    {
        Release();
    }
    void Release()
    {
        if (m_isInit)
        {
            ::OleUninitialize();
            m_isInit = false;
        }
    }
    OleEnv(const OleEnv&) = delete;
    OleEnv(OleEnv&&) = delete;
    OleEnv& operator = (const OleEnv&) = delete;
    OleEnv& operator = (OleEnv&&) = delete;
private:
    bool m_isInit;
};

#endif //ifndef __COM_ENV_INIT_H__
