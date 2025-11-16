#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <libwebsockets.h>
#include <memory>
#include <atomic>
#include <vector>
#include <set>
#include <mutex>

class WebcamCapture;

// 全局函数：更新要广播的帧
void update_global_frame(const std::vector<uint8_t>& frame);

class WebSocketServer {
public:
    WebSocketServer(int port = 9000);
    ~WebSocketServer();

    bool init(std::shared_ptr<WebcamCapture> capture);
    void run();
    void stop();

    // 广播帧到所有连接的客户端
    void broadcastFrame(const std::vector<uint8_t>& frame);

private:
    static int callback_http(struct lws *wsi, enum lws_callback_reasons reason,
                            void *user, void *in, size_t len);
    static int callback_websocket(struct lws *wsi, enum lws_callback_reasons reason,
                                 void *user, void *in, size_t len);

    int port_;
    struct lws_context *context_;
    std::atomic<bool> running_;
    
    std::shared_ptr<WebcamCapture> webcam_capture_;
    
    // 管理所有活动的WebSocket连接
    static std::mutex clients_mutex_;
    static std::set<struct lws*> clients_;
};

#endif // WEBSOCKET_SERVER_H