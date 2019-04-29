#include "VideoEncoder.h"
#include <stdexcept>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctime>
#include <iostream>

extern "C"{
    #include <libswscale/swscale.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/opt.h>
    #include <libavutil/imgutils.h>
}

struct VideoEncoder::Impl{
    
    Impl(int width, int height, int bitrate, int pixel_fmt, std::string codec_name, int fps, int gop_size, int max_b_frames): _width(width), _height(height), _pixel_fmt(pixel_fmt){
        
        //TODO release resources before every throw
        
        _codec = avcodec_find_encoder_by_name(codec_name.c_str());
        
        if (!_codec) {
            
            throw std::runtime_error(("Codec " + codec_name + "not found\n").c_str());
        }
        
        _codec_context = avcodec_alloc_context3(_codec);
        
        if (!_codec_context){
            
            throw std::runtime_error("Could not allocate video codec context\n");
        }
        
        _pkt = av_packet_alloc();
        if (!_pkt){
            
            throw std::runtime_error("Could not allocate packet\n");
        }
        
        _codec_context->bit_rate = bitrate;
        
        _codec_context->width = _width;
        _codec_context->height = _height;
        
        _codec_context->time_base = (AVRational){1, fps};
        _codec_context->framerate = (AVRational){fps, 1};
        
        _codec_context->gop_size = gop_size;
        _codec_context->max_b_frames = max_b_frames;
        
        //TODO assign actual pixel format
        _codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
        
        if (_codec->id == AV_CODEC_ID_H264)
            av_opt_set(_codec_context->priv_data, "preset", "slow", 0);
        
        int ret = avcodec_open2(_codec_context, _codec, NULL);
        
        if (ret < 0) {
            
            throw std::runtime_error("Could not open codec\n");
        }
        
        _frame = av_frame_alloc();
        
        if (!_frame) {
            
            throw std::runtime_error("Could not allocate video frame\n");
        }

        _frame->format = _codec_context->pix_fmt;
        _frame->width  = _codec_context->width;
        _frame->height = _codec_context->height;
        
        ret = av_frame_get_buffer(_frame, 32);
        
        if (ret < 0) {
            
            throw std::runtime_error("Could not allocate the video frame data\n");
        }
        
        _sws_context = sws_getContext(width, height, AV_PIX_FMT_RGB24,
                                      width, height, AV_PIX_FMT_YUV420P,
                                      0, NULL, NULL, NULL);
        if(!_sws_context){
            
            throw std::runtime_error("Could not allocate sws context\n");
        }
        
    }
    
    ~Impl(){
        
        avcodec_free_context(&_codec_context);
        av_frame_free(&_frame);
        av_packet_free(&_pkt);
    }
    
    void encode(std::vector<uint8_t> & bytes) {
        
        int ret;
        
        ret = avcodec_send_frame(_codec_context, _frame);
        
        if (ret < 0) {
            
            throw std::runtime_error("Error sending a frame for encoding\n");
        }
        
        while (ret >= 0) {
        
            ret = avcodec_receive_packet(_codec_context, _pkt);
            
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                return;
            
            else if (ret < 0) {
                
                throw std::runtime_error("Error during encoding\n");
            }
            
            //TODO possibly use memcpy
            for(int i = 0; i < _pkt->size; ++i){
                
                bytes.push_back(_pkt->data[i]);
            }
            
            av_packet_unref(_pkt);
        }
    }
    
    std::vector<uint8_t> encodeFrame(uint8_t * data, int stride1, int stride2, int stride3){
        
        uint8_t * inData[1] = { data }; // RGB24 have one plane
        int inLinesize[1] = { 3*_width };
        
        sws_scale(_sws_context, inData, inLinesize, 0, _height, _frame->data, _frame->linesize);
        
        _frame->pts = _pts++;
        
        std::vector<uint8_t> bytes;
        encode(bytes);
        
        return bytes;
    }
    
    
private:
    
    AVCodec* _codec = NULL;
    AVCodecContext* _codec_context = NULL;
    AVFrame* _frame = NULL;
    AVPacket* _pkt = NULL;
    struct SwsContext    * _sws_context   = NULL;
    
    int _pts = 0;
    
    int _width;
    int _height;
    int _bitrate;
    int _pixel_fmt;
    
    
};


VideoEncoder::VideoEncoder(int width, int height, int bitrate, int pixel_fmt, std::string codec_name, int fps, int gop_size, int max_b_frames):
    _self(new Impl(width, height, bitrate, pixel_fmt, codec_name, fps, gop_size, max_b_frames)){}
    
VideoEncoder::~VideoEncoder(){
    
    delete _self;
}

std::vector<uint8_t> VideoEncoder::encodeFrame(uint8_t * data, int stride1, int stride2, int stride3){
    
    return _self->encodeFrame(data, stride1, stride2, stride3);
}