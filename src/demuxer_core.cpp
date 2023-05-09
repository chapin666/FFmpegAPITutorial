//
// Created by chapin666 on 2023/5/7.
//

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
}

#include <iostream>
#include "io_data.h"

using namespace std;

static AVFormatContext *format_ctx = nullptr;
static AVCodecContext *video_dec_ctx = nullptr, *audio_dec_ctx = nullptr;
static AVStream *video_stream = nullptr, *audio_stream = nullptr;
static FILE *output_video_file = nullptr, *output_audio_file = nullptr;
static AVFrame *frame = nullptr;
static AVPacket pkt = {0};
static int32_t video_stream_idx = -1, audio_stream_idx = -1;

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


static int32_t decode_packet(AVCodecContext *dec, const AVPacket *pkt)
{
    int32_t result = 0;
    result = avcodec_send_packet(dec, pkt);
    if (result < 0)
    {
        cerr << "Error: avcodec_send_packet failed. " << endl;
        return result;
    }

    while (result >= 0)
    {
        result = avcodec_receive_frame(dec, frame);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF)
        {
            return 1;
        }
        else if (result < 0)
        {
            cerr << "Error: avcodec_receive_frame failed. " << endl;
            return result;
        }

        if (dec->codec->type == AVMEDIA_TYPE_VIDEO)
        {
            write_frame_to_yuv(frame);
        }
        else
        {
            write_samples_to_pcm(frame, audio_dec_ctx);
        }

        av_frame_unref(frame);
    }

    return result;
}


int32_t demuxing(char *video_output_name, char *audio_output_name)
{
    int32_t result = 0;
    while (av_read_frame(format_ctx, &pkt) >= 0)
    {
        cout << "Read packet, pts: " << pkt.pts << " ,stream: " << pkt.stream_index << " ,size: " << pkt.size << endl;
        if (pkt.stream_index == video_stream_idx)
        {
            result = decode_packet(video_dec_ctx, &pkt);
        }
        else if (pkt.stream_index == audio_stream_idx)
        {
            result = decode_packet(audio_dec_ctx, &pkt);
        }
        else
        {
            cout << "Ignore packet, stream: " << pkt.stream_index << endl;
        }
        if (result < 0)
        {
            cerr << "Error: decode_packet failed. " << endl;
            return result;
        }
        av_packet_unref(&pkt);
    }

    if (video_dec_ctx)
    {
        decode_packet(video_dec_ctx, nullptr);
    }
    if (audio_dec_ctx)
    {
        decode_packet(audio_dec_ctx, nullptr);
    }

    cout << "Demuxing succeeded." << endl;

    if (video_dec_ctx)
    {
        cout << "Play the output video file with the command:" << endl << "  ffplay -f rawvideo -pix_fmt "
            << string(av_get_pix_fmt_name(video_dec_ctx->pix_fmt)) << " -video_size " << video_dec_ctx->width << "x"
            << video_dec_ctx->height << " " << string(video_output_name) << endl;
    }

    if (audio_dec_ctx)
    {
        enum AVSampleFormat sfmt = audio_dec_ctx->sample_fmt;
        int n_channels = audio_dec_ctx->channels;
        const char *fmt;

        if (av_sample_fmt_is_planar(sfmt))
        {
            const char *packed = av_get_sample_fmt_name(sfmt);
            sfmt = av_get_packed_sample_fmt(sfmt);
            n_channels = 1;
        }
        result = get_format_from_sample_fmt(&fmt, sfmt);
        if (result < 0)
        {
            return result;
        }
        cout << "Play the output video file with the command: " <<
            endl << "  ffplay -f " << string(fmt) << " -ac " << n_channels << " -ar " << audio_dec_ctx->sample_rate << " "
            << string(audio_output_name) << endl;
    }

    return 0;
}

static int open_codec_context(int32_t *stream_idx, AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret, stream_index;
    AVStream *st;
    const AVCodec *dec = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0)
    {
        cerr << "Error: Could not find " << string(av_get_media_type_string(type)) << " stream in input file." << endl;
        return ret;
    }
    else
    {
        stream_index = ret;
        st = fmt_ctx->streams[stream_index];

        dec = avcodec_find_decoder(st->codecpar->codec_id);
        if (!dec)
        {
            cerr << "Error: Failed to find codec: " << string(av_get_media_type_string(type)) << " codec." << endl;
            return -1;
        }

        *dec_ctx = avcodec_alloc_context3(dec);
        if (!*dec_ctx)
        {
            cerr << "Error: Failed to allocate the " << string(av_get_media_type_string(type)) << " codec context." << endl;
            return -1;
        }

        if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0 )
        {
            cerr << "Error: Failed to copy " << string(av_get_media_type_string(type)) << " codec parameters to decoder context." << endl;
            return ret;
        }

        if ((ret = avcodec_open2(*dec_ctx, dec, NULL)) < 0)
        {
            cerr << "Error: Failed to open " << string(av_get_media_type_string(type)) << " codec." << endl;
            return ret;
        }

        *stream_idx = stream_index;
    }
    return 0;
}

int32_t init_demuxer(char *input_name, char *video_output_name, char *audio_output_name)
{
    if (strlen(input_name) == 0)
    {
        cerr << "Error: Input file name is empty." << endl;
        return -1;
    }

    int32_t result = avformat_open_input(&format_ctx, input_name, NULL, NULL);
    if (result < 0)
    {
        cerr << "Error: Could not open input file: " << string(input_name) << endl;
        return -1;
    }

    result = avformat_find_stream_info(format_ctx, NULL);
    if (result < 0)
    {
        cerr << "Error: Could not find stream information." << endl;
        return -1;
    }

    result = open_codec_context(&video_stream_idx, &video_dec_ctx, format_ctx, AVMEDIA_TYPE_VIDEO);
    if (result >= 0)
    {
        video_stream = format_ctx->streams[video_stream_idx];
        output_video_file = fopen(video_output_name, "wb");
        if (!output_video_file)
        {
            cerr << "Error: Could not open output file: " << string(video_output_name) << endl;
            return -1;
        }
    }

    result = open_codec_context(&audio_stream_idx, &audio_dec_ctx, format_ctx, AVMEDIA_TYPE_AUDIO);
    if (result >= 0)
    {
        audio_stream = format_ctx->streams[audio_stream_idx];
        output_audio_file = fopen(audio_output_name, "wb");
        if (!output_audio_file)
        {
            cerr << "Error: Could not open output file: " << string(audio_output_name) << endl;
            return -1;
        }
    }

    av_dump_format(format_ctx, 0, input_name, 0);

    if (!audio_stream && !video_stream)
    {
        cerr << "Error: Could not find audio or video stream in the input, aborting." << endl;
        return -1;
    }

    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    frame = av_frame_alloc();
    if (!frame)
    {
        cerr << "Error: Could not allocate frame." << endl;
        return -1;
    }

    if (video_stream)
    {
        cout<< "Demuxer: Video codec: " << avcodec_get_name(video_dec_ctx->codec_id) << endl;
    }

    if (audio_stream)
    {
        cout << "Demuxer: Audio codec: " << avcodec_get_name(audio_dec_ctx->codec_id) << endl;
    }

    return 0;
}

void destroy_demuxer()
{
    avcodec_free_context(&video_dec_ctx);
    avcodec_free_context(&audio_dec_ctx);
    avformat_close_input(&format_ctx);

    if (output_video_file != nullptr)
    {
        fclose(output_video_file);
        output_video_file = NULL;
    }

    if (output_audio_file != nullptr)
    {
        fclose(output_audio_file);
        output_audio_file = NULL;
    }
}

