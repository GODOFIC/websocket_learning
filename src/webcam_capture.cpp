#include "webcam_capture.h"
#include <iostream>

WebcamCapture::WebcamCapture(int device_id) 
    : device_id_(device_id), running_(false), frame_ready_(false) {
}

WebcamCapture::~WebcamCapture() {
    stop();
}

bool WebcamCapture::init() {
    capture_.open(device_id_);
    
    if (!capture_.isOpened()) {
        std::cerr << "无法打开摄像头设备 " << device_id_ << std::endl;
        return false;
    }
    
    // 设置摄像头参数
    capture_.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    capture_.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    capture_.set(cv::CAP_PROP_FPS, 30);
    
    std::cout << "摄像头初始化成功: " 
              << capture_.get(cv::CAP_PROP_FRAME_WIDTH) << "x"
              << capture_.get(cv::CAP_PROP_FRAME_HEIGHT) << " @ "
              << capture_.get(cv::CAP_PROP_FPS) << " FPS" << std::endl;
    
    return true;
}

void WebcamCapture::start() {
    if (running_.load()) {
        return;
    }
    
    running_.store(true);
    capture_thread_ = std::thread(&WebcamCapture::captureLoop, this);
    std::cout << "摄像头捕获线程已启动" << std::endl;
}

void WebcamCapture::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_.store(false);
    
    if (capture_thread_.joinable()) {
        capture_thread_.join();
    }
    
    if (capture_.isOpened()) {
        capture_.release();
    }
    
    std::cout << "摄像头捕获线程已停止" << std::endl;
}

void WebcamCapture::captureLoop() {
    cv::Mat frame;
    std::vector<uint8_t> jpeg_buffer;
    std::vector<int> encode_params = {cv::IMWRITE_JPEG_QUALITY, 80};
    
    while (running_.load()) {
        if (!capture_.read(frame) || frame.empty()) {
            std::cerr << "无法读取摄像头帧" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        // 编码为JPEG格式
        if (cv::imencode(".jpg", frame, jpeg_buffer, encode_params)) {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            latest_frame_ = jpeg_buffer;
            frame_ready_ = true;
        }
        
        // 控制帧率，避免CPU占用过高
        std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 FPS
    }
}

bool WebcamCapture::getLatestFrame(std::vector<uint8_t>& jpeg_buffer) {
    std::lock_guard<std::mutex> lock(frame_mutex_);
    
    if (!frame_ready_) {
        return false;
    }
    
    jpeg_buffer = latest_frame_;
    return true;
}