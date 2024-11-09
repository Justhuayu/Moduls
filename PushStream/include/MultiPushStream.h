#ifndef MultiPushStream_H
#define MultiPushStream_H
#include <opencv2/opencv.hpp>
#include "CircularQueue.h"
#include "FFmpegPushStream.h"
#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>
/*
    多线程推流模块
输入：循环消息队列消息队列、输入消息的结构
1. 多线程循环消息队列，存储输入的数据，防止推流处理慢时内存爆炸
使用：
1. 外部定义InputData，用于不断 PushData
2. 外部定义 stream_num 和 PushStreamParam[stream_num]，用于构造函数
3. 一个线程Invoke()，一个线程不断PushData(InputData)
*/

class MultiPushStream
{
public:
    struct InputData{
        //定义输入的结构
        cv::Mat img;
        std::string uuid;//流的id
    };
    struct PushStreamParam
    {
        //定义推流的属性
        std::string uuid;//流id
        uint32_t width;
        uint32_t height;
        uint32_t fps;
        char* url;
    };
    void PushData(const InputData& data);//向队列中投递推流数据
    void Invoke();//启动工作线程

public:
    MultiPushStream(const int stream_num,PushStreamParam *param);
    ~MultiPushStream();
private:
    void Init(const int stream_num,PushStreamParam *param);//一个流对应一个线程
    void StopThread() const;//停止所有子线程推流
    struct ThreadPushParam
    {
        //传入子线程的参数
        PushStreamParam streamParam;
        std::queue<InputData> dataQueue;
        bool stopThread=false;
        ssize_t frame_count = 0;
        std::condition_variable cv;
        std::mutex mutex;
    };
    static void ThreadPushStreamFunc(MultiPushStream::ThreadPushParam &param);//子线程推流
private:
    std::vector<std::shared_ptr<FFmpegPushStream>> m_push_stream;//用于推流
    int m_dataCapacity=24 * 1;//消息队列的大小，默认允许存放24帧的数据
    std::unordered_map<std::string,std::shared_ptr<ThreadPushParam>> m_thread_map;//流 uuid 和 子线程参数结构体的映射表
    CircularQueue<InputData> m_dataQueue;//存放输入的图片序列
    std::condition_variable cv;//m_dataQueue的条件变量
    std::mutex mutex;//m_dataQueue的锁
    bool m_stop_flag;//用于停止多线程推流
};
#endif //MultiPushStream_H