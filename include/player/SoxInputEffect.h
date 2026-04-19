#ifndef SOX_BUFFER_INPUT_EFFECT_H
#define SOX_BUFFER_INPUT_EFFECT_H

#include <string>
extern "C" {
#include <sox.h>
}

typedef struct {
    const uint8_t* data; 
    size_t size; 
    size_t pos; 
} BufferInputPrivT;

sox_effect_handler_t const* get_buf_input_handler();

#endif //SOX_BUFFER_INPUT_EFFECT_H
