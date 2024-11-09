//
// Created by lkh on 24-11-8.
//

#include "FFmpegPushStream.h"
#include <iostream>
void FFmpegPushStream::InitFFmpeg()
{
    std::cout<<"FFmpegPushStream InitFFmpeg()!"<<std::endl;
    //1. 注册ffmpeg
    if(avformat_network_init() < 0)
    {
        std::cerr<<"avformat_network_init failed!"<<std::endl;
        return;
    }
    if(!InitRGB2YUV())
    {
        std::cerr<<"InitRGB2YUV failed!"<<std::endl;
        return;
    }
    if(!InitOutputData())
    {
        std::cerr<<"InitOutputData failed!"<<std::endl;
        return;
    }
    if(!InitEncodeContext())
    {
        std::cerr<<"InitEncodeContext failed!"<<std::endl;
        return;
    }if(!CreateFormatContext())
    {
        std::cerr<<"CreateFormatContext failed!"<<std::endl;
        return;
    }
}

bool FFmpegPushStream::PushStream(cv::Mat& frame, int frameCount)
{
    AVPacket *pkt = av_packet_alloc();
    if(!pkt)
    {
        std::cerr << "alloc packet failed!" << std::endl;
        return false;
    }
    //输入数据结构
    uint8_t *in_data[AV_NUM_DATA_POINTERS]={0};
    in_data[0] = frame.data;
    int in_size[AV_NUM_DATA_POINTERS] = {0};
    in_size[0] = frame.cols * frame.elemSize();    //一行（宽）数据的字节数
    //数据格式转换
    int ret = sws_scale(m_pixel_format_context,in_data,in_size,0,
        frame.rows,m_output_data->data,m_output_data->linesize);
    if (ret <= 0)
    {
        std::cerr<<"sws_scale error!"<<std::endl;
        av_packet_free(&pkt);
        return false;
    }
    m_output_data->pts = frameCount;
    int got_packet = 0;
    pkt->data = nullptr;
    pkt->size = 0;
    pkt->pts = AV_NOPTS_VALUE;
    pkt->dts = AV_NOPTS_VALUE;

    ret = avcodec_encode_video2(m_encoder_context,pkt,m_output_data,&got_packet);
    if(got_packet == 0 || ret != 0)
    {
        std::cerr << "avcodec_encode_video2 fail -------------"<<std::endl;
        av_packet_free(&pkt);
        return false;
    }
    if(m_output_stream->codec->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        pkt->pts = av_rescale_q(pkt->pts, m_encoder_context->time_base, m_output_stream->time_base);
        pkt->dts = av_rescale_q(pkt->dts, m_encoder_context->time_base, m_output_stream->time_base);
        pkt->duration = av_rescale_q(pkt->duration, m_encoder_context->time_base, m_output_stream->time_base);

        // 如果没有设置时间戳，确保设置默认值
        if (pkt->pts == AV_NOPTS_VALUE) {
            pkt->pts = pkt->dts = frameCount;
        }
        pkt->stream_index = m_output_stream->index;
        pkt->pos = -1;  // 重置位置
        std::cout << "Set packet timestamps successfully" << std::endl;
    }
    ret = av_interleaved_write_frame(m_format_wrapper,pkt);
    if (ret != 0)
    {
        char buf[1024] = { 0 };
        av_strerror(ret, buf, sizeof(buf) - 1);
        std::cout<<"push frame failed!,"<<buf<<std::endl;
        av_packet_free(&pkt);
        return false;
    }
    av_packet_free(&pkt);
    return true;
}

bool FFmpegPushStream::PushStream(cv::Mat& frame, int frameCount, int mode)
{
    static int count = 1;
    if(count>0)
    {
        std::cout<<"use ffmpeg new api!"<<std::endl;
        count--;
    }

    AVPacket *pkt = av_packet_alloc();
    if (!pkt)
    {
        std::cerr << "alloc packet failed!" << std::endl;
        return false;
    }

    // 输入数据结构
    uint8_t *in_data[AV_NUM_DATA_POINTERS] = {0};
    in_data[0] = frame.data;
    int in_size[AV_NUM_DATA_POINTERS] = {0};
    in_size[0] = frame.cols * frame.elemSize(); // 一行（宽）数据的字节数

    // 数据格式转换
    int ret = sws_scale(m_pixel_format_context, in_data, in_size, 0,
                        frame.rows, m_output_data->data, m_output_data->linesize);
    if (ret <= 0)
    {
        std::cerr << "sws_scale error!" << std::endl;
        av_packet_free(&pkt);
        return false;
    }

    // 设置 pts 值
    m_output_data->pts = frameCount;

    // 使用新版 API 发送帧
    ret = avcodec_send_frame(m_encoder_context, m_output_data);
    if (ret < 0)
    {
        char buf[1024] = {0};
        av_strerror(ret, buf, sizeof(buf) - 1);
        std::cerr << "Error sending frame for encoding: " << buf << std::endl;
        av_packet_free(&pkt);
        return false;
    }

    // 接收编码后的数据
    ret = avcodec_receive_packet(m_encoder_context, pkt);
    if (ret < 0)
    {
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_packet_free(&pkt);
            return true; // 当前帧未完成编码，继续处理下一帧
        }
        else {
            char buf[1024] = {0};
            av_strerror(ret, buf, sizeof(buf) - 1);
            std::cerr << "Error receiving packet from encoder: " << buf << std::endl;
            av_packet_free(&pkt);
            return false;
        }
    }

    // 设置时间戳和索引
    pkt->pts = av_rescale_q(pkt->pts, m_encoder_context->time_base, m_output_stream->time_base);
    pkt->dts = av_rescale_q(pkt->dts, m_encoder_context->time_base, m_output_stream->time_base);
    pkt->duration = av_rescale_q(pkt->duration, m_encoder_context->time_base, m_output_stream->time_base);
    pkt->stream_index = m_output_stream->index;

    // 写入数据
    ret = av_interleaved_write_frame(m_format_wrapper, pkt);
    if (ret < 0)
    {
        char buf[1024] = {0};
        av_strerror(ret, buf, sizeof(buf) - 1);
        std::cerr << "Push frame failed: " << buf << std::endl;
        av_packet_free(&pkt);
        return false;
    }

    av_packet_free(&pkt);
    return true;
}

void FFmpegPushStream::ReleaseResources()
{
    // 1. 释放像素格式转换上下文
    if (m_pixel_format_context) {
        sws_freeContext(m_pixel_format_context);
        m_pixel_format_context = nullptr;
    }

    // 2. 释放输出数据结构
    if (m_output_data) {
        av_frame_free(&m_output_data);
        m_output_data = nullptr;
    }

    // 3. 释放编码器上下文
    if (m_encoder_context) {
        avcodec_free_context(&m_encoder_context);
        m_encoder_context = nullptr;
    }

    // 4. 释放 RTMP FLV 封装器
    if (m_format_wrapper) {
        // 如果 AVIO 流已经打开，关闭它
        if (m_format_wrapper->pb) {
            avio_closep(&m_format_wrapper->pb);
        }
        avformat_free_context(m_format_wrapper);
        m_format_wrapper = nullptr;
    }

    // 5. 视频编码器无需释放，直接置空（可选）
    m_codec = nullptr;

    // 6. 视频输出流无需释放，直接置空（可选）
    m_output_stream = nullptr;
}

bool FFmpegPushStream::InitRGB2YUV()
{
    //初始化rgv -> yuv 的sws
    m_pixel_format_context = sws_getCachedContext(m_pixel_format_context,
        m_width,m_height,AV_PIX_FMT_BGR24,//源宽、高、像素格式
        m_width,m_height,AV_PIX_FMT_YUV420P,//目标宽、高、像素格式
        SWS_BICUBIC,//尺寸变换算法
        0,0,0);
    if(!m_pixel_format_context)
    {
        std::cerr<<"InitRGB2YUV() failed!"<<std::endl;
        return false;
    }
    return true;
}

bool FFmpegPushStream::InitOutputData()
{
    m_output_data = av_frame_alloc();
    m_output_data->format = AV_PIX_FMT_YUV420P;
    m_output_data->width = m_width;
    m_output_data->height = m_height;
    m_output_data->pts = 0;
    int ret = av_frame_get_buffer(m_output_data,32);
    if(ret != 0)
    {
        char buf[1024]={0};
        av_strerror(ret,buf,sizeof(buf)-1);
        std::cerr<<av_strerror<<std::endl;
        return false;
    }
    return true;
}

bool FFmpegPushStream::InitEncodeContext()
{
    m_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if(!m_codec)
    {
        std::cerr<<"Can`t find h264 encoder!"<<std::endl;
        return false;
    }
    m_encoder_context = avcodec_alloc_context3(m_codec);
    if(!m_encoder_context)
    {
        std::cerr<<"avcodec_alloc_context3 failed!"<<std::endl;
        return false;
    }
    //设置编码器参数
    m_encoder_context->codec_id = m_codec->id;
    m_encoder_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    m_encoder_context->thread_count = 8;

    m_encoder_context->bit_rate = 50*1024*8;
    m_encoder_context->width = m_width;
    m_encoder_context->height = m_height;

    m_encoder_context->time_base.num = 1;
    m_encoder_context->time_base.den = m_fps;
    m_encoder_context->framerate.num = 1;
    m_encoder_context->framerate.den = m_fps;

    m_encoder_context->qmin = 10;//调节清晰度和编码速度
    m_encoder_context->qmax = 50;//量化参数，值越低，图片越大，速度越慢，清晰度越好

    m_encoder_context->gop_size = 50; //编码一旦有gopsize很大的时候或者用了opencodec，有些播放器会等待I帧，无形中增加延迟。
    m_encoder_context->max_b_frames = 0; //编码时如果有B帧会再解码时缓存很多帧数据才能解B帧，因此只留下I帧和P帧。
    m_encoder_context->pix_fmt = AV_PIX_FMT_YUV420P;

    AVDictionary *param = 0;
    av_dict_set(&param,"preset","superfast",0);//编码形式修改
    av_dict_set(&param,"tune","zerolatency",0);//实时编码

    int ret = avcodec_open2(m_encoder_context,m_codec,&param);//打开编码器上下文
    if (ret != 0)
    {
        char buf[1024] = { 0 };
        av_strerror(ret, buf, sizeof(buf) - 1);
        std::cout << buf << std::endl;
        return false;
    }
    return true;
}

bool FFmpegPushStream::CreateFormatContext()
{
    int ret = avformat_alloc_output_context2(&m_format_wrapper,0,"flv",m_url);
    if(ret != 0)
    {
        char buf[1024] = { 0 };
        av_strerror(ret, buf, sizeof(buf) - 1);
        std::cerr<<"avformat_alloc_output_context2 error,"<<buf<<std::endl;
        return false;
    }
    //添加视频流
    m_output_stream = avformat_new_stream(m_format_wrapper,nullptr);
    if(!m_output_stream)
    {
        std::cerr<<"avformat_new_stream failed"<<std::endl;
        return false;
    }
    m_output_stream->codecpar->codec_tag = 0;

    //从编码器复制参数
    ret = avcodec_parameters_from_context(m_output_stream->codecpar, m_encoder_context);
    if (ret < 0) {
        std::cerr << "avcodec_parameters_from_context failed!" << std::endl;
        return false;
    }
    // 设置输出流的时间基准
    m_output_stream->time_base = m_encoder_context->time_base;
    av_dump_format(m_format_wrapper,0,m_url,1);

    //打开rtmp网络输出io
    ret = avio_open(&m_format_wrapper->pb,m_url,AVIO_FLAG_WRITE);
    if (ret != 0)
    {
        char buf[1024] = { 0 };
        av_strerror(ret, buf, sizeof(buf) - 1);
        std::cerr<<"avio_open failed,"<<buf<<std::endl;
        return false;
    }

    //写入封装头
    ret = avformat_write_header(m_format_wrapper,nullptr);
    if (ret != 0)
    {
        char buf[1024] = { 0 };
        av_strerror(ret, buf, sizeof(buf) - 1);
        std::cerr<<"avformat_write_header,"<<buf<<std::endl;
        return false;
    }
    return true;
}
