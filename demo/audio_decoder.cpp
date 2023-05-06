//
// Created by chapin666 on 2023/5/6.
//

#include <iostream>
#include <string>
#include "io_data.h"
#include "audio_decoder_core.h"

using namespace std;

static void usage(const char *program_name)
{
    cout << "usage: " << string(program_name) << " input_file output_file codec_name " << endl;
}

int main(int argc, char** argv)
{
    if (argc < 4)
    {
        usage(argv[0]);
        return -1;
    }

    char *input_file_name = argv[1];
    char *output_file_name = argv[2];
    char *codec_name = argv[3];

    cout << "input_file_name: " << string(input_file_name) << endl;
    cout << "output_file_name: " << string(output_file_name) << endl;
    cout << "codec: " << string(codec_name) << endl;

    int result = open_input_output_files(input_file_name, output_file_name);
    if (result < 0)
    {
        return result;
    }

    result = init_audio_decoder(codec_name);
    if (result < 0)
    {
        return result;
    }

    result = audio_decoding();
    if (result < 0)
    {
        return result;
    }

    destroy_audio_decoder();
    close_input_output_files();
    return 0;
}