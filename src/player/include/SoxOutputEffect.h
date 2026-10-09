/*
20250612 AI 生成（Web 问答，手工粘贴代码）
大模型：Deepseek V1
*/
#ifndef SOX_BUFFER_OUTPUT_EFFECT_H
#define SOX_BUFFER_OUTPUT_EFFECT_H

#include <string>
extern "C" {
#include <sox.h>
}

typedef struct {
    uint8_t* data;
    size_t size; 
    size_t pos; 
} BufferOutputPrivT;

sox_effect_handler_t const* get_buf_output_handler();

#endif //SOX_BUFFER_OUTPUT_EFFECT_H
