//
// Created by chapin666 on 2023/5/2.
//
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}
#include <iostream>
#include <string>
#include "video_encoder_core.h"
#include "io_data.h"

using namespace std;

const static AVCodec *codec = nullptr;
static AVCodecContext *codec_ctx = nullptr;
static AVFrame *frame = nullptr;
static AVPacket *pkt = nullptr;


static int32_t encode_frame(bool flushing)
{
    int32_t result = 0;
    if (!flushing)
    {
        std::cout << "Send frame to encoder with pts: "<< frame->pts << std::endl;
    }

    result = avcodec_send_frame(codec_ctx, flushing ? nullptr : frame);
    if (result < 0)
    {
        cerr << "Error: avcodec_send_frame failed. " << endl;
        return result;
    }

    while (result >= 0)
    {
        result = avcodec_receive_packet(codec_ctx, pkt);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF)
        {
            return 1;
        }
        else if (result < 0)
        {
            cerr << "Error: avcodec_receive_packet failed. " << endl;
            return result;
        }
        if (flushing)
        {
            cout << "Flushing: ";
        }
        cout << "Got encoded package with dts: " << pkt->dts << ", pts: " << pkt->pts << ", size: " << pkt->size << endl;
        write_pkt_to_file(pkt);
    }
    return 0;
}

int32_t init_video_encoder(const char *codec_name) {
    // 验证输入编码器名称是否为空
    if (strlen(codec_name) == 0) {
        cerr << "Error: codec_name is nullptr" << endl;
        return -1;
    }

    // 查询编码器
    codec = avcodec_find_encoder_by_name(codec_name);
    if (!codec) {
        cerr << "Error: could not find codec with codec name: " << string(codec_name) << endl;
        return -1;
    }

    // 创建编码器上下文结构
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx)
    {
        cerr << "Error: avcodec_alloc_context3 failed" << endl;
        return -1;
    }

    // 配置编码参数
    codec_ctx->profile = FF_PROFILE_H264_HIGH;
    codec_ctx->bit_rate = 2000000;
    codec_ctx->width = 1920;
    codec_ctx->height = 1080;
    codec_ctx->gop_size = 10;
    codec_ctx->time_base = (AVRational){1, 25};
    codec_ctx->framerate = (AVRational){25, 1};
    codec_ctx->max_b_frames = 3;
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    if (codec->id == AV_CODEC_ID_H264)
    {
        av_opt_set(codec_ctx->priv_data, "preset", "slow", 0);
    }

    // 使用指定的 codec 初始化编码器上下文结构
    int32_t result = avcodec_open2(codec_ctx, codec, nullptr);
    if (result < 0)
    {
        cerr << "Error: could not open codec: " << string(av_err2str(result)) << endl;
        return -1;
    }
    pkt = av_packet_alloc();
    if (!pkt)
    {
        cerr << "Error: could not alloc AVPacket" << endl;
        return -1;
    }
    frame = av_frame_alloc();
    if (!frame)
    {
        cerr << "Error: could not alloc AVFrame" << endl;
        return -1;
    }
    frame->width = codec_ctx->width;
    frame->height = codec_ctx->height;
    frame->format = codec_ctx->pix_fmt;

    result = av_frame_get_buffer(frame, 0);
    if (result < 0)
    {
        cerr << "Error: could not get AVFrame buffer." << endl;
        return -1;
    }

    return 0;
}

void destroy_video_encoder()
{
    avcodec_free_context(&codec_ctx);
    av_frame_free(&frame);
    av_packet_free(&pkt);
}

int32_t encoding(int32_t frame_cnt) {
    int result = 0;
    for (size_t i = 0; i < frame_cnt; i++) {
        result = av_frame_make_writable(frame);
        if (result < 0) {
            cerr << "Error: could not av_frame_make_writeable. " << endl;
            return result;
        }
        result = read_yuv_to_frame(frame);
        if (result < 0) {
            cerr << "Error: read_yuv_to_frame failed. " << endl;
            return result;
        }
        frame->pts = i;

        result = encode_frame(false);
        if (result < 0) {
            cerr << "Error: encode_frame failed. " << endl;
            return result;
        }
    }
    result = encode_frame(true);
    if (result < 0)
    {
        cerr << "Error: encode_frame failed. " << endl;
        return result;
    }
    return 0;
}
