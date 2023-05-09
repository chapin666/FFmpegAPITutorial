//
// Created by chapin666 on 2023/5/8.
//

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
}

#include <iostream>
#include "muxer_core.h"

using namespace std;

const int STREAM_FRAME_RATE = 25;
static AVFormatContext *video_fmt_ctx = nullptr, *audio_fmt_ctx = nullptr, *output_fmt_ctx = nullptr;
static AVPacket pkt;
static int32_t in_video_st_idx = -1, in_audio_st_idx = -1;
static int32_t out_video_st_idx = -1, out_audio_st_idx = -1;


static int32_t init_input_video_file(char *video_input_file, const char *video_format)
{
    int32_t result = 0;
    const AVInputFormat *video_input_format = av_find_input_format(video_format);
    if (video_input_format == nullptr)
    {
        cerr << "Error: failed to find proper AVInputFormat for format:  " << string(video_format) << endl;
        return -1;
    }

    result = avformat_open_input(&video_fmt_ctx, video_input_file, video_input_format, nullptr);
    if (result < 0)
    {
        cerr << "Error: failed to open input file: " << string(video_input_file) << endl;
        return -1;
    }

    result = avformat_find_stream_info(video_fmt_ctx, nullptr);
    if (result < 0)
    {
        cerr << "Error: failed to find stream info for input file: " << string(video_input_file) << endl;
        return -1;
    }

    return result;
}

static int32_t init_input_audio_file(char *audio_input_file, const char *audio_format)
{
    int32_t result = 0;
    const AVInputFormat *audio_input_format = av_find_input_format(audio_format);
    if (!audio_input_format)
    {
        cerr << "Error: failed to find proper AVInputFormat for format:  " << string(audio_format) << endl;
        return -1;
    }

    result = avformat_open_input(&audio_fmt_ctx, audio_input_file, audio_input_format, nullptr);
    if (result < 0)
    {
        cerr << "Error: failed to open input file: " << string(audio_input_file) << endl;
        return -1;
    }

    result = avformat_find_stream_info(audio_fmt_ctx, nullptr);
    if (result < 0)
    {
        cerr << "Error: failed to find stream info for input file: " << string(audio_input_file) << endl;
        return -1;
    }

    return 0;
}

static int32_t init_output_file(char *output_file)
{
    int32_t result = 0;
    avformat_alloc_output_context2(&output_fmt_ctx, nullptr, nullptr, output_file);
    if (!output_fmt_ctx)
    {
        cerr << "Error: failed to create output context for output file: " << string(output_file) << endl;
        return -1;
    }

    const AVOutputFormat *fmt = output_fmt_ctx->oformat;
    cout << "Default video codec id: " << fmt->video_codec << " , audio codec id: " << fmt->audio_codec << endl;

    AVStream *video_stream = avformat_new_stream(output_fmt_ctx, nullptr);
    if (!video_stream)
    {
        cerr << "Error: failed to create video stream for output file: " << string(output_file) << endl;
        return -1;
    }
    out_video_st_idx = video_stream->index;
    in_video_st_idx = av_find_best_stream(video_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (in_video_st_idx < 0)
    {
        cerr << "Error: failed to find best video stream for input file: " << string(output_file) << endl;
        return -1;
    }

    result = avcodec_parameters_copy(video_stream->codecpar, video_fmt_ctx->streams[in_video_st_idx]->codecpar);
    if (result < 0)
    {
        cerr << "Error: failed to copy video codec parameters for output file: " << string(output_file) << endl;
        return -1;
    }

    video_stream->id = output_fmt_ctx->nb_streams - 1;
    video_stream->time_base = (AVRational){1, STREAM_FRAME_RATE};

    AVStream *audio_stream = avformat_new_stream(output_fmt_ctx, nullptr);
    if (!audio_stream)
    {
        cerr << "Error: failed to create audio stream for output file: " << string(output_file) << endl;
        return -1;
    }
    out_audio_st_idx = audio_stream->index;
    in_audio_st_idx = av_find_best_stream(audio_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (in_audio_st_idx < 0)
    {
        cerr << "Error: failed to find best audio stream for input file: " << string(output_file) << endl;
        return -1;
    }
    result = avcodec_parameters_copy(audio_stream->codecpar, audio_fmt_ctx->streams[in_audio_st_idx]->codecpar);
    if (result < 0)
    {
        cerr << "Error: failed to copy audio codec parameters for output file: " << string(output_file) << endl;
        return -1;
    }
    audio_stream->id = output_fmt_ctx->nb_streams - 1;
    audio_stream->time_base = (AVRational){1, audio_stream->codecpar->sample_rate};

    av_dump_format(output_fmt_ctx, 0, output_file, 1);
    cout << "Output video idx: " << out_video_st_idx << " , audio idx: " << out_audio_st_idx << endl;

    if (!(fmt->flags & AVFMT_NOFILE))
    {
        result = avio_open(&output_fmt_ctx->pb, output_file, AVIO_FLAG_WRITE);
        if (result < 0)
        {
            cerr << "Error: failed to open output file: " << string(output_file) << endl;
            return -1;
        }
    }

    return result;
}

int32_t init_muxer(char *video_input_file, char *audio_input_file, char* output_file)
{

    int32_t result = init_input_video_file(video_input_file, "h264");
    if (result < 0)
    {
        return result;
    }

    result = init_input_audio_file(audio_input_file, "mp3");
    if (result < 0)
    {
        return result;
    }

    result = init_output_file(output_file);
    if (result < 0)
    {
        return result;
    }

    return 0;
}

int32_t muxing()
{
    int32_t result = 0;
    int64_t prev_video_dts = 0;
    int64_t cur_video_pts = 0, cur_audio_pts = 0;

    AVStream *in_video_st = video_fmt_ctx->streams[in_video_st_idx];
    AVStream *in_audio_st = audio_fmt_ctx->streams[in_audio_st_idx];
    AVStream *output_stream = nullptr, *input_stream = nullptr;

    int32_t video_frame_idx = 0;

    result = avformat_write_header(output_fmt_ctx, nullptr);
    if (result < 0)
    {
        return -1;
    }

    av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;

    cout << "Video r_frame_rate: " << in_video_st->r_frame_rate.num << "/" << in_video_st->r_frame_rate.den << endl;
    cout << "Video time_base:" << in_video_st->time_base.num << "/" << in_video_st->time_base.den << endl;

    while (1)
    {
        if (av_compare_ts(cur_video_pts, in_video_st->time_base, cur_audio_pts, in_audio_st->time_base) <= 0)
        {
            input_stream = in_video_st;
            result = av_read_frame(video_fmt_ctx, &pkt);
            if (result < 0)
            {
                av_packet_unref(&pkt);
                break;
            }
            if (pkt.pts == AV_NOPTS_VALUE)
            {
                int64_t frame_duration = (double)AV_TIME_BASE / av_q2d(in_video_st->r_frame_rate);
                pkt.duration = (double)frame_duration / (double)av_q2d(in_video_st->time_base) * AV_TIME_BASE;
                pkt.pts = (double)(video_frame_idx * pkt.duration) / (double)(av_q2d(in_video_st->time_base) * AV_TIME_BASE);
                pkt.dts = pkt.dts;
                cout << "frame_duration:" << frame_duration << " ,pkt.duration" << pkt.duration << " ,pkt.pts:" << pkt.pts << endl;
            }

            video_frame_idx++;
            cur_video_pts = pkt.pts;
            pkt.stream_index = out_video_st_idx;
            output_stream = output_fmt_ctx->streams[out_video_st_idx];
        }
        else
        {
            input_stream = in_audio_st;
            result = av_read_frame(audio_fmt_ctx, &pkt);
            if (result < 0)
            {
                av_packet_unref(&pkt);
                break;
            }
            cur_video_pts = pkt.pts;
            pkt.stream_index = out_video_st_idx;
            output_stream = output_fmt_ctx->streams[out_audio_st_idx];
        }

        pkt.pts = av_rescale_q_rnd(pkt.pts, input_stream->time_base, output_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, input_stream->time_base, output_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, input_stream->time_base, output_stream->time_base);
        cout << "Final pts: " << pkt.pts << ", duration:" << pkt.duration << ", output_stream->time_base" << output_stream->time_base.num << "/" << output_stream->time_base.den << endl;

        if (av_interleaved_write_frame(output_fmt_ctx, &pkt) < 0)
        {
            cerr << "Error: failed to mux packet" << endl;
            break;
        }
        av_packet_unref(&pkt);
    }

    result = av_write_trailer(output_fmt_ctx);
    if (result < 0)
    {
        cerr << "Error: failed to write trailer" << endl;
        return -1;
    }

    return result;
}

void destroy_muxer()
{
    avformat_free_context(video_fmt_ctx);
    avformat_free_context(audio_fmt_ctx);

    if (!output_fmt_ctx->oformat->flags & AVFMT_NOFILE)
    {
        avio_closep(&output_fmt_ctx->pb);
    }

    avformat_free_context(output_fmt_ctx);
}