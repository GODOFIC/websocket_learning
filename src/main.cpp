#include "webcam_capture.h"
#include "websocket_server.h"
#include <iostream>
#include <csignal>
#include <memory>
#include <atomic>

static std::atomic<bool> keep_running(true);

void signal_handler(int signal) {
    (void)signal;
    std::cout << "\n接收到停止信号，正在关闭..." << std::endl;
    keep_running.store(false);
}

int main(int argc, char* argv[]) {
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    int device_id = 0;
    int port = 9000;
    
    // 解析命令行参数
    if (argc > 1) {
        device_id = std::atoi(argv[1]);
    }
    if (argc > 2) {
        port = std::atoi(argv[2]);
    }
    
    std::cout << "==================================" << std::endl;
    std::cout << "WebSocket摄像头流媒体服务器" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "摄像头设备ID: " << device_id << std::endl;
    std::cout << "WebSocket端口: " << port << std::endl;
    std::cout << "==================================" << std::endl;
    
    // 初始化摄像头
    auto webcam = std::make_shared<WebcamCapture>(device_id);
    
    if (!webcam->init()) {
        std::cerr << "摄像头初始化失败！" << std::endl;
        return 1;
    }
    
    webcam->start();
    
    // 初始化WebSocket服务器
    WebSocketServer server(port);
    
    if (!server.init(webcam)) {
        std::cerr << "WebSocket服务器初始化失败！" << std::endl;
        webcam->stop();
        return 1;
    }
    
    // 在单独的线程中运行服务器
    std::thread server_thread([&server]() {
        server.run();
    });
    
    // 帧广播线程
    std::thread frame_broadcast_thread([&webcam]() {
        std::vector<uint8_t> frame;
        while (keep_running.load()) {
            if (webcam->getLatestFrame(frame)) {
                update_global_frame(frame);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 FPS
        }
    });
    
    // 主线程等待退出信号
    while (keep_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // 清理资源
    std::cout << "正在停止服务..." << std::endl;
    server.stop();
    webcam->stop();
    
    if (server_thread.joinable()) {
        server_thread.join();
    }
    
    if (frame_broadcast_thread.joinable()) {
        frame_broadcast_thread.join();
    }
    
    std::cout << "程序已安全退出" << std::endl;
    return 0;
}