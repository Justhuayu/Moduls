//
// Created by lkh on 24-11-8.
//

#ifndef FFmpegPushStream_H
#define FFmpegPushStream_H
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}
#include <opencv2/opencv.hpp>
class FFmpegPushStream {
public:
    FFmpegPushStream()
    {
        m_pixel_format_context = nullptr; //像素格式转换上下文
        m_output_data = nullptr;          //输出数据结构
        m_encoder_context = nullptr;      //编码器上下文
        m_format_wrapper = nullptr;       //rtmp flv 封装器
        m_codec = nullptr;                //视频编码器
        m_output_stream = nullptr;        //视频输出流
        // m_width = 0;
        // m_url = "";
        // m_height = 0;
        // m_fps = 0;
        std::cerr<<"[warning]: Currently, the use of the parameterless constructor may result in unknown issues. Please use the parameterized constructor instead."<<std::endl;
    }
    FFmpegPushStream(const uint32_t width,const uint32_t height,const uint8_t fps,const char* url):m_width(width),m_height(height),m_fps(fps),m_url(url)
    {
         m_pixel_format_context = nullptr; //像素格式转换上下文
         m_output_data = nullptr;          //输出数据结构
         m_encoder_context = nullptr;      //编码器上下文
         m_format_wrapper = nullptr;       //rtmp flv 封装器
         m_codec = nullptr;                //视频编码器
         m_output_stream = nullptr;        //视频输出流
    }

    ~FFmpegPushStream(){ReleaseResources();}
    void InitFFmpeg();//初始化FFmpeg
    bool PushStream(cv::Mat &frame,uint64_t  frameCount); //新版api推流
    bool PushStream(cv::Mat &frame,uint64_t  frameCount,int mode); //推流
    void ReleaseResources();//停止FFmpeg
private:
    bool InitRGB2YUV(); //图像格式转换
    bool InitOutputData(); //初始化输出格式
    bool InitEncodeContext(); //初始化编码上下文
    bool CreateFormatContext(); //创建封装器上下文
private:
    uint32_t m_width;
    uint32_t m_height;
    const char* m_url;
    uint8_t m_fps; //视频帧数
    SwsContext *m_pixel_format_context;   //像素格式转换上下文
    AVFrame *m_output_data;               //输出数据结构
    AVCodecContext *m_encoder_context;    //编码器上下文
    AVFormatContext *m_format_wrapper;    //rtmp flv 封装器
    AVCodec *m_codec;                     //视频编码器
    AVStream *m_output_stream;            //视频输出流
};



#endif //FFmpegPushStream_H
