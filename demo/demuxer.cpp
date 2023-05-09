//
// Created by chapin666 on 2023/5/7.
//
#include <iostream>
#include "demuxer_core.h"

using namespace std;

static void usage(const char *program_name)
{
    cout << "usage: " << string(program_name) << " input_file output_video_file output_audio_file " << endl;
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        usage(argv[0]);
        return 1;
    }

    int32_t result = init_demuxer(argv[1], argv[2], argv[3]);
    if (result < 0) {
        cerr << "Error: failed to init demuxer." << endl;
    }

    result = demuxing(argv[2], argv[3]);
    if (result < 0) {
        cerr << "Error: failed to demuxing." << endl;
    }

    destroy_demuxer();

    return 0;
}