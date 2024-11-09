#ifndef MultiPushStream_H
#define MultiPushStream_H
#include <opencv2/opencv.hpp>
#include "ThreadSafeCircularQueue.h"
/*
    多线程推流模块
输入：循环消息队列消息队列、输入消息的结构
1. 多线程循环消息队列，存储输入的数据，防止推流处理慢时内存爆炸
2. 
*/

class MultiPushStream
{
public:
    struct InputData{
        cv::Mat img;
    };
public:
    MultiPushStream();
    ~MultiPushStream();
private:
    std::shared_ptr<ThreadSafeCircularQueue<InputData>> m_dataQueue;
    int m_dataCapacity=24 * 1;//消息队列的大小，默认允许存放24帧的数据
};

#endif MultiPushStream_H