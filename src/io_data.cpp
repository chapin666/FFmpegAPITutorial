#include <iostream>
#include "io_data.h"

static FILE *input_file = nullptr;
static FILE *output_file = nullptr;

int32_t open_input_output_files(const char *input_name, const char *output_name)
{
    if (strlen(input_name) == 0 || strlen(output_name) == 0)
    {
        std::cerr << "Error: empty input or output file name." << std::endl;
        return -1;
    }
    close_input_output_files();

    input_file = fopen(input_name, "rb");
    if (input_file == nullptr)
    {
        std::cerr << "Error: can't open input file." << std::endl;
        return -1;
    }

    output_file = fopen(output_name, "wb");
    if (output_file == nullptr)
    {
        std::cerr << "Error: can't open output file." << std::endl;
        return -1;
    }
    return 0;
}

void close_input_output_files()
{
    if (input_file != nullptr)
    {
        fclose(input_file);
        input_file = nullptr;
    }
    if (output_file != nullptr)
    {
        fclose(output_file);
        output_file = nullptr;
    }
}

int32_t read_yuv_to_frame(AVFrame *frame)
{
    int32_t frame_width = frame->width;
    int32_t frame_height = frame->height;
    int32_t luma_stride = frame->linesize[0];
    int32_t chroma_stride = frame->linesize[1];
    int32_t frame_size = frame_width * frame_height * 3 / 2;
    int32_t read_size = 0;

    // 如果 width 等于 stride，则说明 frame 中不存在 padding 字节，可整体读取
    if (frame_width == luma_stride)
    {
        read_size += fread(frame->data[0], 1, frame_width * frame_height, input_file);
        read_size += fread(frame->data[1], 1, frame_height * frame_width / 4, input_file);
        read_size += fread(frame->data[2], 1, frame_height * frame_width / 4, input_file);
    }
    else
    {
        // 如果 width 不等于 stride，则说明 frame 中存在 padding 字节，对应三个分量应当逐行读取
        for (size_t i = 0; i < frame_height; i++)
        {
            read_size += fread(frame->data[0]+i*luma_stride, 1, frame_width, input_file);
        }
        for (size_t uv = 1; uv < 2; uv++)
        {
            for (size_t i = 0; i < frame_height / 2; i++)
            {
                read_size += fread(frame->data[uv] + i * chroma_stride, 1, frame_width / 2, input_file);
            }
        }
    }

    if (read_size != frame_size)
    {
        std::cerr << "Error: Read data error, frame_size: " << frame_size << std::endl;
        return -1;
    }
    return 0;
}

void write_pkt_to_file(AVPacket *pkt)
{
    fwrite(pkt->data, 1, pkt->size, output_file);
}