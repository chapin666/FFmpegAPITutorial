//
// Created by chapin666 on 2023/5/6.
//

#ifndef AUDIO_DECODER_CORE_H
#define AUDIO_DECODER_CORE_H

#include <stdint.h>

int32_t init_audio_decoder(const char* codec_name);
void destroy_audio_decoder();

int32_t audio_decoding();

#endif //AUDIO_DECODER_CORE_H
