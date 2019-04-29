#include "VideoDecoder.h"
#include <stdexcept>


extern "C"{
    #include <libswscale/swscale.h>
    #include <libavcodec/avcodec.h>
}
#define INBUF_SIZE 4096

#include <condition_variable>
#include <mutex>
#include <queue>
#include <cstring>

struct VideoDecoder::Impl{
    
    Impl(int width, int height, std::string codec_name): _width(width), _height(height){
        
        _pkt = av_packet_alloc();
        if (!_pkt){
            
            throw std::runtime_error("Could not allocate packet\n");
        }
        
        memset(_inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);
        
        _codec = avcodec_find_decoder_by_name(codec_name.c_str());
        
        if (!_codec) {
            
            throw std::runtime_error("Codec not found\n");
        }
        
        _parser = av_parser_init(_codec->id);
        
        if (!_parser) {
            
            throw std::runtime_error("Parser not found\n");
        }
        
        _codec_context = avcodec_alloc_context3(_codec);
        
        if (!_codec_context) {
            
            throw std::runtime_error("Could not allocate video codec context\n");
        }
        
        //TODO supply width and height with  AVDictionary **options
        if (avcodec_open2(_codec_context, _codec, NULL) < 0) {
            
            throw std::runtime_error("Could not open codec\n");
        }
        
        _frame = av_frame_alloc();
        if (!_frame) {
            
            throw std::runtime_error("Could not allocate video frame\n");
        }
        
        _sws_context = sws_getContext(width, height, AV_PIX_FMT_YUV420P,
                                      width, height, AV_PIX_FMT_RGB24,
                                      0, NULL, NULL, NULL);
        if(!_sws_context){
            
            throw std::runtime_error("Could not allocate sws context\n");
        }
        
        
    }
    
    void decode(){
        
        int ret;
        ret = avcodec_send_packet(_codec_context, _pkt);
        if (ret < 0) {
            
            throw std::runtime_error("Error sending a packet for decoding\n");
        }
        while (ret >= 0) {
            
            ret = avcodec_receive_frame(_codec_context, _frame);
            
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                return;
            
            else if (ret < 0) {
                
                throw std::runtime_error("Error during decoding\n");
            }
            
            
            VideoFrame video_frame;
            video_frame.width = _width;
            video_frame.height = _height;
            video_frame.nc = 3;
            video_frame.data = (uint8_t *) std::malloc(_width * _height * 3);
            video_frame.pts = _codec_context->frame_number;
            
            int out_linesize[1] = { 3 *_width };
            sws_scale(_sws_context, _frame->data, _frame->linesize, 0, _height, &video_frame.data, out_linesize);
            
            // at this point the frame should be fully constructed;
            
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _queue.push(video_frame);
            }
            _inputCondition.notify_one();
        }
        
    }
    
    void supplyBytes(uint8_t * input_bytes, int size){
        
        uint8_t * input_bytes_alias = input_bytes;
        int num_available_bytes = size;
        
        while(num_available_bytes > 0){
            
            int num_bytes_to_copy = num_available_bytes >= INBUF_SIZE ? INBUF_SIZE : num_available_bytes;
            
            std::memcpy(_inbuf, input_bytes_alias, num_bytes_to_copy);
            
            num_available_bytes -= num_bytes_to_copy;
            input_bytes_alias += num_bytes_to_copy;
            
            int data_size = num_bytes_to_copy;
            
            uint8_t * data = _inbuf;
            
            while (data_size > 0) {
                
                int ret = av_parser_parse2(_parser, _codec_context, &_pkt->data, &_pkt->size,
                                    data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
                if (ret < 0) {
                    
                    throw std::runtime_error("Error while parsing\n");
                }
                
                data      += ret;
                data_size -= ret;
                if (_pkt->size)
                    decode();
            }
        }
        
    }
    
    VideoFrame getFrame(){
        
        std::unique_lock<std::mutex> lock(_mutex);
	
        _inputCondition.wait(lock, [&]{ return !_queue.empty(); });
		
	VideoFrame frame = _queue.front();
        _queue.pop();
        return frame;
    }
    
    ~Impl(){
        av_parser_close(_parser);
        avcodec_free_context(&_codec_context);
        av_frame_free(&_frame);
        av_packet_free(&_pkt);
        sws_freeContext(_sws_context);
    }
    
private:
    
    int _width;
    int _height;
    
    AVCodec              * _codec         = NULL;
    AVCodecParserContext * _parser        = NULL;
    AVCodecContext       * _codec_context = NULL;
    AVPacket             * _pkt           = NULL;
    AVFrame              * _frame         = NULL;
    struct SwsContext    * _sws_context   = NULL;
    
    uint8_t _inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    
    std::condition_variable _inputCondition;
    std::mutex              _mutex;
    std::queue<VideoFrame>  _queue;
    
};

VideoDecoder::VideoDecoder(int width, int height, std::string codec_name): _self( new Impl(width, height, codec_name)){}

VideoDecoder::~VideoDecoder(){
    
    delete _self;
}

void VideoDecoder::supplyBytes(uint8_t * data, int size){
    
    _self->supplyBytes(data, size);
}

VideoFrame VideoDecoder::getFrame(){
    
    return _self->getFrame();
}



