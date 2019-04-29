# Python Video Streaming

This is a tool for video streaming in python. It has two core functions: video encoding and video decoding.

Essentially, this is a wrapper for FFmpeg codecs. It is based on these [two](https://ffmpeg.org/doxygen/trunk/encode_video_8c-example.html) [examples](https://ffmpeg.org/doxygen/trunk/decode_video_8c-example.html). 

## Usage
This module is supposed to be used in a setting with two peers: a receiver and a sender.

The sender uses a `PyVideoEncoder` object to encode video frames into messages, the receiver uses a `PyVideoDecoder` to decode messages into frames.

Crucially, there's no one-to-one correspondence between frames and messages. The receiver needs to supply messages into decoder in one thread and wait for decoded frames in another thread.


## Examples
Execute these two commands in separate terminals to run video streaming on your local machine:

```sh
video@streaming:~/ python receiver.py
video@streaming:~/ python sender.py
```

## Installation
Set your environment:
```sh
conda create -n video_streaming python=3.6
conda activate video_streaming
pip install numpy
pip install cython
pip install opencv-python

```

### Linux
Install `libvpx` and `libx264`.

Build FFmpeg from source:
```sh
git clone https://github.com/FFmpeg/FFmpeg.git
cd FFmpeg
./configure --enable-pic --enable-gpl --enable-shared --enable-libx264
make
sudo make install
```
Build `video_streaming`:
```sh
python setup.py build_ext --inplace
```

### Windows

Download latest FFmpeg builds from https://ffmpeg.zeranoe.com/builds/win64/ (you need dev and shared), unpack.

In file `setup.py` set paths `include_dir` and `lib_dir` to your newly downloaded resources.

Build `video_streaming`:
```sh
python setup.py build_ext --inplace
```

You may need to copy FFmpeg .dll's and binaries to your folder.

### MacOS

Contributions welcome.

## Acknowledgments

Thanks go to FFmpeg team and to Fabrice Bellard in particular for providing the examples. I also thank  Gael Varoquaux for providing an example of python bindings for c-computed arrays: https://gist.github.com/GaelVaroquaux/1249305