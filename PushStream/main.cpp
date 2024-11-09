#include <iostream>
#include <opencv2/opencv.hpp>
#include "FFmpegPushStream.h"
#include <csignal>
#include <atomic>

std::atomic<bool> program_running(true);  // 用于控制程序的运行状态

void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\nCtrl+C detected. Preparing to release resources..." << std::endl;
        program_running = false;  // 设置标记来退出主循环
    }
}

int main() {
    // 1. 读取一张本地图片，用于推流
    cv::Mat src_img = cv::imread(PROJECT_ROOT_DIR "/src.PNG");
    if (src_img.empty()) {
        std::cerr << "Failed to load image. Exiting." << std::endl;
        return -1;
    }

    // 2. 初始化 ffmpeg 推流
    int width = src_img.cols;
    int height = src_img.rows;
    const char* url = "rtmp://127.0.0.1:1935/input";
    FFmpegPushStream ffmpeg(width, height, 24,url);
    ffmpeg.InitFFmpeg();

    // 3. 注册信号处理器，以便在检测到 Ctrl+C 时释放资源
    std::signal(SIGINT, signal_handler);

    // 4. 推流循环
    int frame_count = 0;
    while (program_running) {
        frame_count++;
        if (!ffmpeg.PushStream(src_img, frame_count,1)) {
            std::cerr << "Failed to push frame " << frame_count << std::endl;
            break;
        }
    }
    // 5. 程序退出前的清理工作
    ffmpeg.ReleaseResources();
    std::cout << "Program exited cleanly." << std::endl;
    return 0;
}
