//
// Created by chapin666 on 2023/5/6.
//
extern "C" {
#include <libavcodec/avcodec.h>
}
#include <iostream>
#include "audio_decoder_core.h"
#include "io_data.h"

#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

using namespace std;

const static AVCodec *codec = nullptr;
static AVCodecContext *codec_ctx = nullptr;
static AVCodecParserContext *parser_ctx = nullptr;

static AVPacket *pkt = nullptr;
static AVFrame *frame = nullptr;
static enum AVCodecID audio_codec_id;

static int get_format_from_sample_fmt(const char **fmt, enum AVSampleFormat sample_fmt)
{
    int i;
    struct sample_fmt_entry {
        enum AVSampleFormat sample_fmt;
        const char* fmt_be, *fmt_le;
    } sample_fmt_entries[] = {
            { AV_SAMPLE_FMT_U8, "u8", "u8" },
            { AV_SAMPLE_FMT_S16, "s16be", "s16le" },
            { AV_SAMPLE_FMT_S32, "s32be", "s32le" },
            { AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
            { AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
    };
    *fmt = NULL;

    for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++)
    {
        struct sample_fmt_entry *entry = &sample_fmt_entries[i];
        if (sample_fmt == entry->sample_fmt)
        {
            *fmt = AV_NE(entry->fmt_be, entry->fmt_le);
            return 0;
        }
    }

    cerr << "sample format %s is not supported as output format" << av_get_sample_fmt_name(sample_fmt) << endl;
    return -1;
}

int32_t get_audio_format(AVCodecContext *codec_ctx)
{
    int ret = 0;
    const char *fmt;
    enum AVSampleFormat sfmt = codec_ctx->sample_fmt;
    if (av_sample_fmt_is_planar(sfmt))
    {
        const char *packed = av_get_sample_fmt_name(sfmt);
        cout << "Warning: the sample format the decoder produced is planar "<< string(packed)
                << ", This example will output the first channel only." << endl;
        sfmt = av_get_packed_sample_fmt(sfmt);
    }

    int n_channels = codec_ctx->channels;
    if ((ret = get_format_from_sample_fmt(&fmt, sfmt)) < 0)
    {
        return -1;
    }

    cout << "Play command: ffplay -f " << string(fmt) << " -ac " << n_channels << " -ar " << codec_ctx->sample_rate << " output.pcm" << endl;
    return 0;
}

static int32_t decode_packet(bool flushing)
{
    int32_t result = 0;
    result = avcodec_send_packet(codec_ctx, flushing ? nullptr : pkt);
    if (result < 0)
    {
        cerr << "Error: avcodec_send_packet failed. " << endl;
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
            cerr << "Error: avcodec_receive_frame failed. " << endl;
            return -1;
        }
        if (flushing)
        {
            cout << "Flushing:";
        }
        write_samples_to_pcm(frame, codec_ctx);
        cout << "frame->nb_samples: " << frame->nb_samples << ", frame->channels: " << frame->channels << endl;
    }
    return result;
}

int32_t audio_decoding()
{
    uint8_t inbuf[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE] = {0};
    int32_t result = 0;
    uint8_t *data = nullptr;
    int32_t data_size = 0;

    while (!end_of_input_file())
    {
        result = read_data_to_buf(inbuf, AUDIO_INBUF_SIZE, data_size);
        if (result < 0)
        {
            cerr << "Error: read data to buf failed. " << endl;
            return -1;
        }
        data = inbuf;
        while (data_size > 0)
        {
            result = av_parser_parse2(parser_ctx, codec_ctx, &pkt->data, &pkt->size, data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (result < 0)
            {
                cerr << "Error: av_parser_parse2 failed. " << endl;
                return -1;
            }
            data += result;
            data_size -= result;

            if (pkt->size)
            {
                cout << "Got package with size: " << pkt->size << endl;
                decode_packet(false);
            }
        }
    }

    decode_packet(true);
    get_audio_format(codec_ctx);

    return 0;
}

int32_t init_audio_decoder(const char *codec_name)
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

    codec = avcodec_find_decoder(audio_codec_id);
    if (!codec)
    {
        cerr << "Error: codec not found" << endl;
        return -1;
    }
    parser_ctx = av_parser_init(codec->id);
    if (!parser_ctx)
    {
        cerr << "Error: parser not found" << endl;
        return -1;
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx)
    {
        cerr << "Error: could not allocate audio codec context" << endl;
        return -1;
    }

    int32_t result = avcodec_open2(codec_ctx, codec, nullptr);
    if (result < 0)
    {
        cerr << "Error: could not open codec" << endl;
        return -1;
    }

    frame = av_frame_alloc();
    if (!frame)
    {
        cerr << "Error: could not allocate audio frame" << endl;
        return -1;
    }

    pkt = av_packet_alloc();
    if (!pkt)
    {
        cerr << "Error: could not allocate packet" << endl;
        return -1;
    }

    return 0;
}

void destroy_audio_decoder()
{
    av_parser_close(parser_ctx);
    avcodec_free_context(&codec_ctx);
    av_frame_free(&frame);
    av_packet_free(&pkt);
}