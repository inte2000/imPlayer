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

	for (size_t i = 0; i < len; i++) {
		priv->data[priv->pos++] = soxSampleToFloat(ibuf[i], effp->clips);
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



