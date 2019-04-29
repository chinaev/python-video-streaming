#include <string>
#include <stdint.h>
#include <vector>

struct VideoFrame{
    int width = 0;
    int height = 0;
    int nc = 0;
    int pts;
    uint8_t * data = NULL;
};

class VideoDecoder{
    
    struct Impl;
    Impl * _self;
    
public:
    
    VideoDecoder(int width, int height, std::string codec_name);
    ~VideoDecoder();
    
    void supplyBytes(uint8_t * data, int size);
    
    VideoFrame getFrame();
    
    //void terminate();
    
};