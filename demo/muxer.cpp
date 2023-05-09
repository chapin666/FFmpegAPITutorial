//
// Created by chapin666 on 2023/5/8.
//
#include <iostream>
#include "muxer_core.h"

using namespace std;

static void usage(const char *program_name)
{
    cout << "usage: " << string(program_name) << " input_video_file input_audio_file output_file " << endl;
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        usage(argv[0]);
        return 1;
    }

    int32_t result = init_muxer(argv[1], argv[2], argv[3]);
    if (result < 0)
    {
        cerr << "Error: failed to init muxer." << endl;
    }

    result = muxing();
    if (result < 0)
    {
        cerr << "Error: failed to muxing." << endl;
    }

    destroy_muxer();

    return 0;
}