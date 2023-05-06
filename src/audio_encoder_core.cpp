//
// Created by chapin666 on 2023/5/6.
//
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
}

#include <iostream>
#include <string>
#include "io_data.h"

using namespace std;

const static AVCodec *codec = nullptr;
static AVCodecContext *codec_ctx = nullptr;
static AVFrame *frame = nullptr;
static AVPacket *pkt = nullptr;
static enum AVCodecID audio_codec_id;

int32_t init_audio_encoder(const char *codec_name)
{
    if (strcasecmp(codec_name, "MP3") == 0)
    {
        audio_codec_id = AV_CODEC_ID_MP3;
        cout << "Select codec id: MP3 " << endl;
    }
    else if (strcasecmp(codec_name, "AAC") == 0)
    {
        audio_codec_id = AV_CODEC_ID_AAC;
        cout << "Select codec id: AAC " << endl;
    }
    else
    {
        cerr << "Unsupported codec: " << string(codec_name) << endl;
        return -1;
    }

    codec = avcodec_find_encoder(audio_codec_id);
    if (!codec)
    {
        cerr << "Error: codec not found" << endl;
        return -1;
    }
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx)
    {
        cerr << "Error: could not allocate audio codec context" << endl;
        return -1;
    }

    // 设置音频编码器参数
    const int sample_rate = 44100;
    const int num_channels = 2;
    const int bit_rate = 64000;
    codec_ctx->bit_rate = bit_rate;               // 输出码率为 64Kbps
    codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP; // 采样格式为 planar float
    codec_ctx->sample_rate = sample_rate;             // 采样率为 44100
    codec_ctx->channels = num_channels;
    codec_ctx->channel_layout = av_get_default_channel_layout(num_channels); // 双声道

    // 打开音频编码器
    int32_t result = avcodec_open2(codec_ctx, codec, nullptr);
    if (result < 0)
    {
        cerr << "Error: could not open codec" << endl;
        return -1;
    }

    // 分配音频帧
    frame = av_frame_alloc();
    if (!frame)
    {
        cerr << "Error: could not allocate audio frame" << endl;
        return -1;
    }

    frame->nb_samples = codec_ctx->frame_size;
    frame->format = codec_ctx->sample_fmt;
    frame->channel_layout = codec_ctx->channel_layout;

    result = av_frame_get_buffer(frame, 0);
    if (result < 0)
    {
        cerr << "Error: could not allocate audio data buffers" << endl;
        return -1;
    }

    // 分配音频包
    pkt = av_packet_alloc();
    if (!pkt)
    {
        cerr << "Error: could not allocate audio packet" << endl;
        return -1;
    }

    return 0;
}

static int32_t encode_frame(bool flushing)
{
    int32_t result = 0;
    result = avcodec_send_frame(codec_ctx, flushing ? nullptr : frame);
    if (result < 0)
    {
        cerr << "Error: could not send audio frame" << endl;
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
            cerr << "Error: could not receive audio packet" << endl;
            return result;
        }
        write_pkt_to_file(pkt);
    }
    return 0;
}

int32_t audio_encoding()
{
    int32_t result = 0;

    while (!end_of_input_file())
    {
        result = read_pcm_to_frame(frame, codec_ctx);
        if (result < 0)
        {
            cerr << "Error: could not read audio data" << endl;
            return -1;
        }
        result = encode_frame(false);
        if (result < 0)
        {
            cerr << "Error: could not encode audio frame" << endl;
            return result;
        }
    }

    result = encode_frame(true);
    if (result < 0)
    {
        cerr << "Error: could not encode audio frame" << endl;
        return result;
    }
    return 0;
}

void destroy_audio_encoder()
{
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&codec_ctx);
}