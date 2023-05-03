//
// Created by chapin666 on 2023/5/2.
//

#ifndef VIDEO_ENCODER_CORE_H
#define VIDEO_ENCODER_CORE_H
#include <stdint.h>

// 初始化视频编码器
int32_t init_video_encoder(const char *codec_name);

// 销毁视频编码器
void destroy_video_encoder();

// 编码一帧数据
int32_t encoding(int32_t frame_cnt);

#endif //VIDEO_ENCODER_CORE_H
