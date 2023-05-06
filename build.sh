#!/bin/zsh

rm -rf output
mkdir output
cd output

cmake ..
make

# ./video_encoder
./video_decoder