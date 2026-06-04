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
    if (effp->in_encoding->encoding == SOX_ENCODING_SIGN2) {
        if (effp->in_encoding->bits_per_sample == 8) {
            const int8_t* pDPtr = (const int8_t*)priv->data;
            for (size_t i = 0; i < samples_to_read; i++) {
                obuf[i] = SOX_SIGNED_8BIT_TO_SAMPLE(pDPtr[priv->pos++], effp->clips);
            }
        }
        else if (effp->in_encoding->bits_per_sample == 16) {
            const int16_t* pDPtr = (const int16_t*)priv->data;
            for (size_t i = 0; i < samples_to_read; i++) {
                obuf[i] = SOX_SIGNED_16BIT_TO_SAMPLE(pDPtr[priv->pos++], effp->clips);
            }
        }
        else if (effp->in_encoding->bits_per_sample == 24) {
            const uint8_t* p = (const uint8_t*)(priv->data + priv->pos * 3);
            for (size_t i = 0; i < samples_to_read; i++) {
                int32_t v = (p[0] << 8) | (p[1] << 16) | (p[2] << 24);
                obuf[i] = (sox_sample_t)v;
                p += 3;
                priv->pos++;
            }
        }
        else if (effp->in_encoding->bits_per_sample == 32) {
            const int32_t* pDPtr = (const int32_t*)priv->data;
            for (size_t i = 0; i < samples_to_read; i++) {
                obuf[i] = (sox_sample_t)pDPtr[priv->pos++];
            }
        }
        else {
            *osamp = 0;
            return SOX_EFMT;
        }
    }
    else if (effp->in_encoding->encoding == SOX_ENCODING_FLOAT) {
/*
        for (size_t i = 0; i < samples_to_read; i++) {
            obuf[i] = floatToSoxSample(priv->data[priv->pos++], effp->clips);
        }
*/
        SOX_SAMPLE_LOCALS;
        if (effp->in_encoding->bits_per_sample == 32) {
            const float* pDptr = (const float*)priv->data;
            for (size_t i = 0; i < samples_to_read; i++) {
                obuf[i] = SOX_FLOAT_32BIT_TO_SAMPLE(pDptr[priv->pos++], effp->clips);
            }
        }
        else {
            const double* pDptr = (const double*)priv->data;
            for (size_t i = 0; i < samples_to_read; i++) {
                obuf[i] = SOX_FLOAT_64BIT_TO_SAMPLE(pDptr[priv->pos++], effp->clips);
            }
        }
    }
    else if (effp->in_encoding->encoding == SOX_ENCODING_UNSIGNED) {
        if (effp->in_encoding->bits_per_sample == 8) {
            for (size_t i = 0; i < samples_to_read; i++) {
                obuf[i] = SOX_UNSIGNED_8BIT_TO_SAMPLE(priv->data[priv->pos++], effp->clips);
            }
        }
        else if (effp->in_encoding->bits_per_sample == 16) {
            const uint16_t* pDPtr = (const uint16_t*)priv->data;
            for (size_t i = 0; i < samples_to_read; i++) {
                obuf[i] = SOX_UNSIGNED_16BIT_TO_SAMPLE(pDPtr[priv->pos++], effp->clips);
            }
        }
        else if (effp->in_encoding->bits_per_sample == 24) {
            const uint8_t* p = (const uint8_t*)(priv->data + priv->pos * 3);
            for (size_t i = 0; i < samples_to_read; i++) {
                uint32_t v = (p[0] << 8) | (p[1] << 16) | (p[2] << 24);
                obuf[i] = (sox_sample_t)v;
                p += 3;
                priv->pos++;
            }
        }
        else if (effp->in_encoding->bits_per_sample == 32) {
            const uint32_t* pDPtr = (const uint32_t*)priv->data;
            for (size_t i = 0; i < samples_to_read; i++) {
                obuf[i] = (sox_sample_t)pDPtr[priv->pos++];
            }
        }
        else {
            *osamp = 0;
            return SOX_EFMT;
        }
    }
    else {
        *osamp = 0;
        return SOX_EFMT;
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



