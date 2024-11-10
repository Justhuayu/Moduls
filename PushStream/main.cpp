#include <iostream>
#include <opencv2/opencv.hpp>
#include "FFmpegPushStream.h"
#include "MultiPushStream.h"
#include "uuid/uuid.h"

int TestOnePushStream();//测试单线程推流
int TestMulPushStream(int stream_num);//测试多线程推流
void GenerateUUID(std::string& m_id);//生成uuid
int main() {
    //TestOnePushStream();
    TestMulPushStream(4);
    return 0;
}
int TestMulPushStream(int stream_num)
{
    //1. 读取本地图片
    std::vector<cv::Mat> src_imgs(stream_num);
    for(auto& img:src_imgs)
    {
        img = cv::imread(PROJECT_ROOT_DIR "/../_sources/src1.JPEG");
        if (img.empty()) {
            std::cerr << "Failed to load image. Exiting." << std::endl;
            return -1;
        }
    }
    //2. 构建输入数据，用于输入
    std::vector<MultiPushStream::InputData> src_datas(stream_num);
    for(int i=0;i<stream_num;++i)
    {
        src_datas[i].img = src_imgs[i];
        GenerateUUID(src_datas[i].uuid);
        std::cout<<"i: "<<src_datas[i].uuid<<std::endl;
    }
    //3. 构建流的参数
    std::vector<MultiPushStream::PushStreamParam> stream_params(stream_num);
    int fps = 24;
    std::string url_base = "rtmp://127.0.0.1:1935/";
    for(int i=0;i<stream_num;++i)
    {
        stream_params[i].uuid = src_datas[i].uuid;
        stream_params[i].height = src_datas[i].img.rows;
        stream_params[i].width = src_datas[i].img.cols;
        stream_params[i].fps = fps;
        stream_params[i].url = (url_base + std::to_string(i)).c_str();
        std::cout<<"stream url: "<<stream_params[i].url<<std::endl;
    }
    //4. 初始化 MultiPushStream 类
    MultiPushStream push_tool(stream_num,stream_params);
    //5. 子线程Invoke(),负责监听任务
    std::thread invoke_thread(&MultiPushStream::Invoke,&push_tool);
    invoke_thread.detach();
    //6. 循环推流
    while (1)
    {
        for(int i=0;i<stream_num;++i)
        {
            push_tool.PushData(src_datas[i]);
        }
        //推流速度太快，会导致服务器推流不过来，从而崩溃
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return 0;
}
int TestOnePushStream()
{
    //测试单线程推流
    // 1. 读取一张本地图片，用于推流
    cv::Mat src_img = cv::imread(PROJECT_ROOT_DIR "/../_sources/src.PNG");
    if (src_img.empty()) {
        std::cerr << "Failed to load image. Exiting." << std::endl;
        return -1;
    }

    // 2. 初始化 ffmpeg 推流
    int width = src_img.cols;
    int height = src_img.rows;
    const char* url = "rtmp://127.0.0.1:1935/input";
    FFmpegPushStream ffmpeg(width, height, 5,url);
    ffmpeg.InitFFmpeg();

    // 4. 推流循环
    //TODO: 时间戳过大问题？
    uint64_t frame_count = 0;
    while (1) {
        frame_count++;
        if (!ffmpeg.PushStream(src_img, frame_count)) {
            std::cerr << "Failed to push frame " << frame_count << std::endl;
            break;
        }
    }

    // 5. 程序退出前的清理工作
    ffmpeg.ReleaseResources();
    std::cout << "Program exited cleanly." << std::endl;
    return 0;
}

void GenerateUUID(std::string& m_id) {
    uuid_t uuid;
    uuid_generate(uuid);
    char uuid_str[37]; // UUID格式为36字符 + 1个空字符
    uuid_unparse(uuid, uuid_str);
    m_id = uuid_str;
}