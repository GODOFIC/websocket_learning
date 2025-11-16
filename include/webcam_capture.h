#ifndef WEBCAM_CAPTURE_H
#define WEBCAM_CAPTURE_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>

class WebcamCapture {
public:
    WebcamCapture(int device_id = 0);
    ~WebcamCapture();

    bool init();
    void start();
    void stop();
    
    // 获取最新的JPEG编码帧
    bool getLatestFrame(std::vector<uint8_t>& jpeg_buffer);
    
    bool isRunning() const { return running_.load(); }

private:
    void captureLoop();
    
    int device_id_;
    cv::VideoCapture capture_;
    
    std::atomic<bool> running_;
    std::thread capture_thread_;
    
    std::mutex frame_mutex_;
    std::vector<uint8_t> latest_frame_;
    bool frame_ready_;
};

#endif // WEBCAM_CAPTURE_H