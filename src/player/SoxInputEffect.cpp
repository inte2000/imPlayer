/*
20250612 AI 生成（Web 问答，手工粘贴代码）
大模型：Deepseek V1
*/
#include <fstream>
#include <string_view>
#include <format>
#include "ScopeGuard.h"
#include "SoxInputEffect.h"

inline sox_sample_t floatToSoxSample(float v, sox_uint64_t& clips) {
    if (v <= -1.0f) {
        ++clips;
        return SOX_SAMPLE_MIN;
    }
    else if (v >= 1.0f) {
        ++clips;
        return SOX_SAMPLE_MAX;
    }
    else {
        return (sox_sample_t)(v * SOX_SAMPLE_MAX);
    }
}


int buf_input_effect_options(sox_effect_t* effp, int argc, char* argv[]) {
    BufferInputPrivT* priv = (BufferInputPrivT*)effp->priv;

    // 设置默认参数
    priv->pos = 0;
    if (argc != 3) {
        return SOX_EOF;
    }
    // 处理命令行参数
    priv->data = (const uint8_t*)argv[1];
    priv->size = atol(argv[2]);

    return SOX_SUCCESS;
}

int buf_input_effect_start(sox_effect_t* effp) {
    BufferInputPrivT* priv = (BufferInputPrivT*)effp->priv;
    return SOX_SUCCESS;
}

int buf_input_effect_flow(sox_effect_t* effp, const sox_sample_t* ibuf, sox_sample_t* obuf, size_t* isamp, size_t* osamp) {
    BufferInputPrivT* priv = (BufferInputPrivT*)effp->priv;

    return SOX_SUCCESS;
}

// 效果器停止
int buf_input_effect_stop(sox_effect_t* effp) {
    BufferInputPrivT* priv = (BufferInputPrivT*)effp->priv;
    return SOX_SUCCESS;
}

int buf_input_effect_drain(sox_effect_t* effp, sox_sample_t* obuf, size_t* osamp) {
    BufferInputPrivT* priv = (BufferInputPrivT*)effp->priv;
    size_t remaining = priv->size - priv->pos;
    if (remaining == 0) {
        *osamp = 0;
        return SOX_EOF; // 数据结束
    }

    size_t samples_to_read = (*osamp < remaining) ? *osamp : remaining;

    // 从缓冲区复制数据
	for (size_t i = 0; i < samples_to_read; i++) {
		obuf[i] = floatToSoxSample(priv->data[priv->pos++], effp->clips);
	}

    *osamp = samples_to_read;
     return SOX_SUCCESS;
}

// 效果器删除
int buf_input_effect_delete(sox_effect_t* effp) {
    BufferInputPrivT* priv = (BufferInputPrivT*)effp->priv;
    //free(priv);
    return SOX_SUCCESS;
}

// 自定义输入 effect handler
sox_effect_handler_t const* get_buf_input_handler() {
    static sox_effect_handler_t handler = {
        "buf_input",              // effect 名字
        nullptr,                 // usage
        SOX_EFF_MCHAN,           // flags
        buf_input_effect_options, buf_input_effect_start, // getopts, start
        nullptr, buf_input_effect_drain, // flow, drain
        nullptr, nullptr,        // stop, kill
        sizeof(BufferInputPrivT)     // priv size
    };
    return &handler;
}



