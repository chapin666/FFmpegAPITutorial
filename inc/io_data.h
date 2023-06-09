//
// Created by chapin666 on 2023/5/2.
//
#ifndef IO_DATA_H
#define IO_DATA_H
extern "C" {
#include <libavcodec/avcodec.h>
}
#include <stdint.h>

// video encoder
int32_t open_input_output_files(const char* input_name, const char* output_name);
void close_input_output_files();
int32_t read_yuv_to_frame(AVFrame *frame);
void write_pkt_to_file(AVPacket *pkt);

// video decoder
int32_t end_of_input_file();
int32_t read_data_to_buf(uint8_t *buf, int32_t size, int32_t &out_size);
int32_t write_frame_to_yuv(AVFrame *frame);


// audio encoder
int32_t read_pcm_to_frame(AVFrame *frame, AVCodecContext *codec_ctx);
int32_t write_samples_to_pcm(AVFrame *frame, AVCodecContext *codec_ctx);

#endif