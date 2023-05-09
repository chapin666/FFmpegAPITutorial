//
// Created by chapin666 on 2023/5/8.
//
#include <iostream>
#include "muxer_core.h"

using namespace std;

static void usage(const char *program_name)
{
    cout << "usage: " << string(program_name) << " input_file output_video_file output_audio_file " << endl;
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        usage(argv[0]);
        return 1;
    }

    int32_t result = 0;
    do {
        result = init_muxer(argv[1], argv[2], argv[3]);
        if (result < 0)
        {
            break;
        }

        result = muxing();
        if (result < 0)
        {
            break;
        }
    } while (0);

    destroy_muxer();

    return 0;
}