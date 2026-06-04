/*
20250612 AI 生成（Web 问答，手工粘贴代码）
大模型：Deepseek V1
*/
#include <string_view>
#include "ScopeGuard.h"
#include "SoxOutputEffect.h"

// sox_sample_t -> PCM float
inline float soxSampleToFloat(sox_sample_t d, sox_uint64_t& clips)
{
    // 溢出保护：接近上限直接当作 1.0
    if (d > SOX_SAMPLE_MAX - 64) {
        ++clips;
        return 1.0f;
    }

    // 四舍五入到 128 的倍数
    sox_sample_t rounded = (d + 64) & ~127;

    // 归一化到 [-1.0, 1.0)
    return (float)rounded * (1.0f / (SOX_SAMPLE_MAX + 1.0f));
}

int buf_output_start(sox_effect_t* effp) {
    BufferOutputPrivT* priv = (BufferOutputPrivT*)effp->priv;
    priv->pos = 0;
    return SOX_SUCCESS;
}

int buf_output_flow(sox_effect_t* effp, const sox_sample_t* ibuf, sox_sample_t* obuf, size_t* isamp, size_t* osamp) {
    BufferOutputPrivT* priv = (BufferOutputPrivT*)effp->priv;

    std::size_t len = *isamp;

    if (effp->out_encoding->encoding == SOX_ENCODING_SIGN2) {
        if (effp->out_encoding->bits_per_sample == 8) {
            SOX_SAMPLE_LOCALS;
            for (size_t i = 0; i < len; i++) {
                priv->data[priv->pos++] = SOX_SAMPLE_TO_SIGNED_8BIT(ibuf[i], effp->clips);
            }
        }
        else if (effp->out_encoding->bits_per_sample == 16) {
            SOX_SAMPLE_LOCALS;
            int16_t* pDPtr = (int16_t*)priv->data;
            for (size_t i = 0; i < len; i++) {
                pDPtr[priv->pos++] = SOX_SAMPLE_TO_SIGNED_16BIT(ibuf[i], effp->clips);
            }
        }
        else if (effp->out_encoding->bits_per_sample == 24) {
            //SOX_SAMPLE_LOCALS;
            int8_t* pDPtr = (int8_t*)(priv->data + priv->pos * 3);
            uint32_t thipos = 0;
            for (size_t i = 0; i < len; i++) {
                int32_t v = SOX_SAMPLE_TO_SIGNED_32BIT(ibuf[i], effp->clips);
                pDPtr[thipos++] = (v >> 8) & 0xFF;
                pDPtr[thipos++] = (v >> 16) & 0xFF;
                pDPtr[thipos++] = (v >> 24) & 0xFF;
                priv->pos++;
            }
        }
        else if (effp->out_encoding->bits_per_sample == 32) {
            int32_t* pDPtr = (int32_t*)priv->data;
            for (size_t i = 0; i < len; i++) {
                pDPtr[priv->pos++] = SOX_SAMPLE_TO_SIGNED_32BIT(ibuf[i], effp->clips);
            }
        }
        else {
            *isamp = 0;
            return SOX_EFMT;
        }
    }
    else if (effp->out_encoding->encoding == SOX_ENCODING_FLOAT) {
        /*
        for (size_t i = 0; i < len; i++) {
            priv->data[priv->pos++] = soxSampleToFloat(ibuf[i], effp->clips);
        }
        */
        if (effp->out_encoding->bits_per_sample == 32) {
            SOX_SAMPLE_LOCALS;
            float* pDptr = (float*)priv->data;
            for (size_t i = 0; i < len; i++) {
                pDptr[priv->pos++] = (float)SOX_SAMPLE_TO_FLOAT_32BIT(ibuf[i], effp->clips);
            }
        }
        else {
            //SOX_SAMPLE_LOCALS;
            double* pDptr = (double*)priv->data;
            for (size_t i = 0; i < len; i++) {
                pDptr[priv->pos++] = SOX_SAMPLE_TO_FLOAT_64BIT(ibuf[i], effp->clips);
            }
        }
    }
    else if (effp->out_encoding->encoding == SOX_ENCODING_UNSIGNED) {
        if (effp->out_encoding->bits_per_sample == 8) {
            SOX_SAMPLE_LOCALS;
            for (size_t i = 0; i < len; i++) {
                priv->data[priv->pos++] = SOX_SAMPLE_TO_UNSIGNED_8BIT(ibuf[i], effp->clips);
            }
        }
        else if (effp->out_encoding->bits_per_sample == 16) {
            SOX_SAMPLE_LOCALS;
            uint16_t* pDPtr = (uint16_t*)priv->data;
            for (size_t i = 0; i < len; i++) {
                pDPtr[priv->pos++] = SOX_SAMPLE_TO_UNSIGNED_16BIT(ibuf[i], effp->clips);
            }
        }
        else if (effp->out_encoding->bits_per_sample == 24) {
            uint8_t* pDPtr = (uint8_t*)(priv->data + priv->pos * 3);
            uint32_t thipos = 0;
            for (size_t i = 0; i < len; i++) {
                uint32_t v = SOX_SAMPLE_TO_UNSIGNED_32BIT(ibuf[i], effp->clips);
                pDPtr[thipos++] = (v >> 8) & 0xFF;
                pDPtr[thipos++] = (v >> 16) & 0xFF;
                pDPtr[thipos++] = (v >> 24) & 0xFF;
                priv->pos++;
            }
        }
        else if (effp->out_encoding->bits_per_sample == 32) {
            uint32_t* pDPtr = (uint32_t*)priv->data;
            for (size_t i = 0; i < len; i++) {
                pDPtr[priv->pos++] = SOX_SAMPLE_TO_UNSIGNED_32BIT(ibuf[i], effp->clips);
            }
        }
        else {
            *isamp = 0;
            return SOX_EFMT;
        }
    }
    else {
        *isamp = 0;
        return SOX_EFMT;
    }

    *isamp = len;
    *osamp = 0;
    return SOX_SUCCESS;
}

int buf_output_options(sox_effect_t* effp, int argc, char* argv[]) {
    BufferOutputPrivT* priv = (BufferOutputPrivT*)effp->priv;

    priv->pos = 0;
    if (argc != 3) {
        return SOX_EOF;
    }

    priv->data = (uint8_t *)argv[1];
    priv->size = atol(argv[2]);

    return SOX_SUCCESS;
}

int buf_output_drain(sox_effect_t* effp, sox_sample_t* obuf, size_t* osamp) {
    BufferOutputPrivT* priv = (BufferOutputPrivT*)effp->priv;
    *osamp = 0;
    return SOX_SUCCESS;
}

int buf_output_stop(sox_effect_t* effp) {
    return SOX_SUCCESS;
}

int buf_output_kill(sox_effect_t* effp) {
    return SOX_SUCCESS;
}

// 自定义输出 effect handler
sox_effect_handler_t const* get_buf_output_handler() {
    static sox_effect_handler_t handler = {
        "buf_output", nullptr, SOX_EFF_MCHAN,
        buf_output_options, nullptr,
        buf_output_flow, nullptr, nullptr, nullptr,
        sizeof(BufferOutputPrivT)
    };
    return &handler;
}



