#include "websocket_server.h"
#include "webcam_capture.h"
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>

// 静态成员初始化
std::mutex WebSocketServer::clients_mutex_;
std::set<struct lws*> WebSocketServer::clients_;

// 全局变量存储最新帧（简化实现）
static std::mutex g_frame_mutex;
static std::vector<uint8_t> g_current_frame;
static bool g_frame_updated = false;

// 每个WebSocket连接的会话数据
struct SessionData {
    std::vector<uint8_t> pending_frame;
};

WebSocketServer::WebSocketServer(int port) 
    : port_(port), context_(nullptr), running_(false) {
}

WebSocketServer::~WebSocketServer() {
    stop();
}

bool WebSocketServer::init(std::shared_ptr<WebcamCapture> capture) {
    webcam_capture_ = capture;

    // 定义协议
    static struct lws_protocols protocols[] = {
        {
            "http",
            callback_http,
            0,
            0,
            0,
            NULL,
            0
        },
        {
            "webcam-stream",
            callback_websocket,
            sizeof(SessionData),
            1024 * 1024, // 1MB接收缓冲区
            0,
            NULL,
            0
        },
        { NULL, NULL, 0, 0, 0, NULL, 0 } // 终止符
    };

    // 定义HTTP挂载点（静态文件服务）
    static const struct lws_http_mount mount = {
        .mount_next = NULL,
        .mountpoint = "/",
        .origin = "../www",
        .def = "index.html",
        .protocol = NULL,
        .cgienv = NULL,
        .extra_mimetypes = NULL,
        .interpret = NULL,
        .cgi_timeout = 0,
        .cache_max_age = 0,
        .auth_mask = 0,
        .cache_reusable = 0,
        .cache_revalidate = 0,
        .cache_intermediaries = 0,
        .origin_protocol = LWSMPRO_FILE,
        .mountpoint_len = 1,
        .basic_auth_login_file = NULL,
    };

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

    info.port = port_;
    info.protocols = protocols;
    info.mounts = &mount;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_VALIDATE_UTF8;

    context_ = lws_create_context(&info);

    if (!context_) {
        std::cerr << "无法创建libwebsockets上下文" << std::endl;
        return false;
    }

    std::cout << "WebSocket服务器初始化成功，监听端口: " << port_ << std::endl;
    return true;
}

void WebSocketServer::run() {
    running_.store(true);
    
    std::cout << "WebSocket服务器开始运行..." << std::endl;
    std::cout << "请在浏览器中访问: http://localhost:" << port_ << std::endl;
    
    while (running_.load()) {
        // 处理事件，10ms超时
        lws_service(context_, 10);
        
        // 定期请求所有客户端可写
        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            for (auto* client : clients_) {
                lws_callback_on_writable(client);
            }
        }
        
        // 短暂休眠以避免CPU占用过高
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void WebSocketServer::stop() {
    running_.store(false);
    
    if (context_) {
        lws_context_destroy(context_);
        context_ = nullptr;
    }
    
    std::cout << "WebSocket服务器已停止" << std::endl;
}

void WebSocketServer::broadcastFrame(const std::vector<uint8_t>& frame) {
    (void)frame; // 此方法已废弃，使用update_global_frame代替
    std::lock_guard<std::mutex> lock(clients_mutex_);
    
    for (auto* client : clients_) {
        // 为每个客户端触发可写回调
        lws_callback_on_writable(client);
    }
}

int WebSocketServer::callback_http(struct lws *wsi, enum lws_callback_reasons reason,
                                   void *user, void *in, size_t len) {
    (void)wsi;
    (void)user;
    (void)len;

    switch (reason) {
        case LWS_CALLBACK_HTTP: {
            const char* url = (const char*)in;
            std::cout << "HTTP请求: " << url << std::endl;
            // mount会自动处理静态文件，这里只记录日志
            // 返回1表示让libwebsockets使用mount处理
            break;
        }

        case LWS_CALLBACK_HTTP_BODY:
            // HTTP body接收
            break;

        case LWS_CALLBACK_HTTP_BODY_COMPLETION:
            // HTTP body接收完成
            break;

        case LWS_CALLBACK_HTTP_WRITEABLE:
            // HTTP可写
            break;

        case LWS_CALLBACK_HTTP_FILE_COMPLETION:
            // 文件传输完成
            break;

        case LWS_CALLBACK_CLOSED_HTTP:
            // HTTP连接关闭
            break;

        default:
            break;
    }

    return 0;
}

int WebSocketServer::callback_websocket(struct lws *wsi, enum lws_callback_reasons reason,
                                       void *user, void *in, size_t len) {
    (void)user;
    (void)in;
    (void)len;
    
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            std::cout << "新客户端连接 (WebSocket)" << std::endl;
            {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                clients_.insert(wsi);
            }
            // 请求立即发送数据
            lws_callback_on_writable(wsi);
            break;
            
        case LWS_CALLBACK_CLOSED:
            std::cout << "客户端断开连接 (WebSocket)" << std::endl;
            {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                clients_.erase(wsi);
            }
            break;
            
        case LWS_CALLBACK_SERVER_WRITEABLE: {
            std::vector<uint8_t> frame_to_send;
            
            {
                std::lock_guard<std::mutex> lock(g_frame_mutex);
                if (!g_current_frame.empty()) {
                    frame_to_send = g_current_frame;
                }
            }
            
            if (!frame_to_send.empty()) {
                size_t frame_size = frame_to_send.size();
                
                // 为libwebsockets预留头部空间
                std::vector<uint8_t> buffer(LWS_PRE + frame_size);
                memcpy(&buffer[LWS_PRE], frame_to_send.data(), frame_size);
                
                int written = lws_write(wsi, &buffer[LWS_PRE], frame_size, LWS_WRITE_BINARY);
                
                if (written < 0) {
                    std::cerr << "发送帧失败" << std::endl;
                    return -1;
                }
                
                if (written < (int)frame_size) {
                    std::cerr << "帧发送不完整: " << written << "/" << frame_size << std::endl;
                }
            }
            
            // 请求下一次发送
            lws_callback_on_writable(wsi);
            break;
        }
        
        case LWS_CALLBACK_RECEIVE:
            // 接收客户端消息（可选）
            std::cout << "收到客户端消息: " << std::string((char*)in, len) << std::endl;
            break;
            
        default:
            break;
    }
    
    return 0;
}

// 更新全局帧（供外部调用）
void update_global_frame(const std::vector<uint8_t>& frame) {
    std::lock_guard<std::mutex> lock(g_frame_mutex);
    g_current_frame = frame;
    g_frame_updated = true;
}