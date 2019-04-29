#include <string>
#include <stdint.h>
#include <vector>

class VideoEncoder{
    
    struct Impl;
    Impl * _self;
    
public:
    
    VideoEncoder(int width, int height, int bitrate, int pixel_fmt, std::string codec_name, int fps, int gop_size, int max_b_frames);
    ~VideoEncoder();
    
    std::vector<uint8_t> encodeFrame(uint8_t * data, int stride1, int stride2, int stride3);
    
    //void terminate();
    
};