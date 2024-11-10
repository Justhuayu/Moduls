#include "MultiPushStream.h"

MultiPushStream::MultiPushStream(const int stream_num,PushStreamParam *param)
    :m_dataCapacity(stream_num * m_dataCapacity_weight)
    ,m_dataQueue(m_dataCapacity)
{
    m_push_stream.resize(stream_num);
    Init(stream_num,param);
    m_stop_flag = false;
}

MultiPushStream::MultiPushStream(const int stream_num, std::vector<PushStreamParam> params)
    :m_dataCapacity(stream_num * m_dataCapacity_weight)
    ,m_dataQueue(m_dataCapacity)
{
    m_push_stream.resize(stream_num);
    Init(stream_num,params.data());
    m_stop_flag = false;
}

MultiPushStream::~MultiPushStream()
{
    //1. 停止所有子线程 和 工作线程
    {
        std::lock_guard<std::mutex> lock(mutex);
        m_stop_flag = true;
    }
    //2. 释放类资源
    m_thread_map.clear();
}


void MultiPushStream::StopThread() const
{
    //停止多线程，遍历所有子线程，停止子线程
    for(auto & it : m_thread_map)
    {
        it.second->stopThread = true;
    }
}

void MultiPushStream::ThreadPushStreamFunc(MultiPushStream::ThreadPushParam& param)
{
    //子线程推流函数，等待一定时间没数据后，自动停止
    // 线程添加 FFmpegPushStream 类，初始化 FFmpegPushStream
    thread_local FFmpegPushStream push_tool(param.streamParam.width,param.streamParam.height,param.streamParam.fps,param.streamParam.url.c_str());
    push_tool.InitFFmpeg();
    while(1)
    {
        //监听 param.dataQueue 是否有数据，有就推流
        std::unique_lock<std::mutex> lock(param.mutex);
        if(param.dataQueue.empty())
        {
            //TODO:定时器关闭长期没任务的线程
            param.cv.wait(lock,[&param]{return !param.dataQueue.empty() || param.stopThread;});
        }
        if(param.stopThread)
        {
            std::cout<<"push stream thread uuid: "<<param.streamParam.uuid<<" stop."<<std::endl;
            break;
        }
        //推流图片
        while (!param.dataQueue.empty())
        {
            InputData data = param.dataQueue.front();
            param.dataQueue.pop();
            if(data.uuid != param.streamParam.uuid)
            {
                std::cout<<"push stream thread uuid: " <<param.streamParam.uuid<<"is pushing data uuid: "<<data.uuid<<"!"<<std::endl;
                continue;
            }
            push_tool.PushStream(data.img,param.frame_count);
            param.frame_count++;
        }
    }
    push_tool.ReleaseResources();
}

void MultiPushStream::Init(const int stream_num,PushStreamParam *param)
{
    //为 stream_num 个流，开起线程，并存到线程映射表中
    //1. 开启线程 ，构建 uuid - ThreadPushParam
    for(int i = 0;i<stream_num;++i)
    {
        auto args = std::make_shared<ThreadPushParam>();
        args->streamParam = param[i];
        auto thread = std::thread(&MultiPushStream::ThreadPushStreamFunc,std::ref(*args));
        thread.detach();
        m_thread_map[param[i].uuid] = args;
    }
}

void MultiPushStream::PushData(const InputData& data)
{
    //向循环队列中投递推流数据
    std::lock_guard<std::mutex> lock(mutex);
    m_dataQueue.push(data);
    if(m_dataQueue.size() == m_dataCapacity)
    {
       std::cout<<"push stream too fast, data queue is full, data queue size is "<<m_dataQueue.size()<<std::endl;
    }
    cv.notify_one();
}

void MultiPushStream::Invoke()
{
    //监听m_dataQueue，有数据就分派给子线程
    //长时间没数据停止子线程，停止后能重启
    while(1)
    {
        //从 m_dataQueue 中读取数据 data
        //根据data 的uuid，从m_thread_map中找到对应的ThreadPushParam
        //通知子线程处理数据
        std::unique_lock<std::mutex> lock(mutex);
        if(m_dataQueue.empty())
        {
            cv.wait(lock,[this]{return !m_dataQueue.empty() || m_stop_flag;});
        }
        if(m_stop_flag)
        {
            this->StopThread();
            break;
        }
        //派发 m_dataQueue 的数据
        while(!m_dataQueue.empty())
        {
            InputData data = m_dataQueue.pop();
            auto thread_param = m_thread_map[data.uuid];
            thread_param->dataQueue.push(data);
            thread_param->cv.notify_one();
        }
    }
}


