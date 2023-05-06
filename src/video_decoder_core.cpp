//
// Created by chapin666 on 2023/5/5.
//

extern "C" {
    #include <libavcodec/avcodec.h>
}
#include <iostream>
#include <string>
#include "video_decoder_core.h"
#include "io_data.h"

using namespace std;

const int INBUF_SIZE = 4096;
const static AVCodec *codec = nullptr;
static AVCodecParserContext *parser = nullptr;
static AVCodecContext *codec_ctx = nullptr;
static AVFrame *frame = nullptr;
static AVPacket *packet = nullptr;

static int32_t decode_packet(bool flusing)
{
    int32_t result = 0;
    result = avcodec_send_packet(codec_ctx, flusing ? nullptr : packet);
    if (result < 0)
    {
        cerr << "Error: avcodec_send_packet failed. result: " << result << endl;
        return result;
    }

    while (result >= 0)
    {
        result = avcodec_receive_frame(codec_ctx, frame);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF)
        {
            return 1;
        }
        else if (result < 0)
        {
            cerr << "Error: avcodec_receive_frame failed. result: " << result << endl;
            return -1;
        }
        if (flusing)
        {
            cout << "Flushing: ";
        }
        cout << "Write frame pic_num: " << frame->coded_picture_number << endl;
        write_frame_to_yuv(frame);
    }

    return 0;
}


int32_t decoding()
{
    uint8_t inbuf[INBUF_SIZE] = {0};
    int32_t result = 0;
    uint8_t *data = nullptr;
    int32_t data_size = 0;

    while (!end_of_input_file())
    {
        result = read_data_to_buf(inbuf, INBUF_SIZE, data_size);
        if (result < 0)
        {
            cerr << "Error: read_data_to_buf failed." << endl;
            return -1;
        }
        data = inbuf;
        while (data_size > 0)
        {
            result = av_parser_parse2(parser, codec_ctx, &packet->data, &packet->size, data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (result < 0)
            {
                cerr << "Error: av_parser_parse2 failed." << endl;
                return -1;
            }
            data += result;
            data_size -= result;

            if (packet->size)
            {
                cout << "Parsed packet size: " << packet->size << endl;
                decode_packet(false);
            }
        }
    }

    decode_packet(true);
    return 0;
}

int32_t init_video_decoder() 
{
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        cerr << "Error: could not find codec." << endl;
        return -1;
    }

    parser = av_parser_init(codec->id);
    if (!parser) {
        cerr << "Error: could not init parser." << endl;
        return -1;
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        cerr << "Error: could not alloc codec context." << endl;
        return -1;
    }

    int32_t result = avcodec_open2(codec_ctx, codec, nullptr);
    if (result < 0) {
        cerr << "Error: could not open codec." << endl;
        return -1;
    }

    frame = av_frame_alloc();
    if (!frame) {
        cerr << "Error: could not alloc frame." << endl;
        return -1;
    }

    packet = av_packet_alloc();
    if (!packet) {
        cerr << "Error: could not alloc packet." << endl;
        return -1;
    }

    return 0;
}

void destroy_video_decoder() 
{
    av_parser_close(parser);
    avcodec_free_context(&codec_ctx);
    av_frame_free(&frame);
    av_packet_free(&packet);
}



