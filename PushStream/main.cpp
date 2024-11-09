#include <iostream>
#include <opencv2/opencv.hpp>
#include "FFmpegPushStream.h"


int TestOnePushStream();//测试单线程推流
int TestMulPushStream();//测试多线程推流
int main() {
    TestOnePushStream();
    //TestMulPushStream();
    return 0;
}
int TestMulPushStream()
{
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
