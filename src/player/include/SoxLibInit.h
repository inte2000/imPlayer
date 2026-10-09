/*
20250612 AI 生成（Web 问答，手工粘贴代码）
大模型：Deepseek V1
*/
#ifndef SOX_LIB_INIT_H
#define SOX_LIB_INIT_H

#include <string>

extern "C" {
#include <sox.h>
}


class CSoxLibInit final
{
public:
    CSoxLibInit() {
        m_bInit = false;
        if (sox_init() == SOX_SUCCESS) {
            sox_format_init();
            m_bInit = true;
        }
    }
    bool IsValid() const {
        return m_bInit;
    }
    ~CSoxLibInit() {
        if(m_bInit) {
            sox_format_quit();
            sox_quit();
        }
    }
private:
    bool m_bInit;
};

#endif //SOX_LIB_INIT_H
