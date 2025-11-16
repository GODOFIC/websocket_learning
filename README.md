# WebSocket摄像头实时流项目

基于libwebsockets和OpenCV的摄像头实时流媒体系统，支持浏览器实时查看摄像头画面。

## 技术栈

- **C++17**
- **libwebsockets** - WebSocket服务器实现
- **OpenCV 4** - 摄像头捕获和图像处理
- **HTML5/JavaScript** - 浏览器客户端

## 功能特性

✅ 实时摄像头画面捕获  
✅ JPEG图像压缩传输  
✅ WebSocket双向通信  
✅ 多客户端同时连接  
✅ 实时FPS和延迟显示  
✅ 优雅的Web界面  

## 系统架构

```
┌─────────────┐      WebSocket       ┌──────────────┐
│  浏览器客户端 │ ◄──────────────────► │ C++ 服务器    │
│             │   (JPEG Binary)      │              │
│  - HTML/JS  │                      │ - libwebsockets│
│  - Canvas   │                      │ - OpenCV      │
└─────────────┘                      └───────┬────────┘
                                             │
                                             ▼
                                        ┌──────────┐
                                        │ USB 摄像头 │
                                        └──────────┘
```

## 依赖安装

### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libwebsockets-dev \
    libopencv-dev \
    pkg-config
```

### 摄像头驱动
确保系统已加载UVC驱动，检查设备：
```bash
ls /dev/video*
v4l2-ctl --list-devices
```

## 编译项目

```bash
cd webcam-websocket-stream
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## 运行

### 启动服务器
```bash
# 使用默认参数（设备0，端口9000）
./webcam_server

# 指定摄像头设备和端口
./webcam_server 0 9000
```

### 访问客户端
在浏览器中打开：
```
http://localhost:9000
```

## 项目结构

```
webcam-websocket-stream/
├── CMakeLists.txt          # CMake构建配置
├── README.md               # 项目文档
├── include/                # 头文件
│   ├── webcam_capture.h    # 摄像头捕获类
│   └── websocket_server.h  # WebSocket服务器类
├── src/                    # 源文件
│   ├── main.cpp            # 主程序入口
│   ├── webcam_capture.cpp  # 摄像头捕获实现
│   └── websocket_server.cpp # WebSocket服务器实现
└── www/                    # Web客户端
    └── index.html          # HTML界面
```

## 核心组件说明

### 1. WebcamCapture类
- 负责打开和读取USB摄像头
- 在独立线程中持续捕获帧
- 将帧编码为JPEG格式
- 提供线程安全的帧访问接口

### 2. WebSocketServer类
- 基于libwebsockets实现WebSocket服务器
- 管理多个客户端连接
- 广播视频帧到所有连接的客户端
- 处理HTTP请求（提供HTML页面）

### 3. HTML客户端
- 使用WebSocket API连接服务器
- 接收二进制JPEG数据
- 在Canvas上实时渲染图像
- 显示FPS、延迟等统计信息

## WebSocket协议细节

### 连接建立
```
客户端 → 服务器: HTTP Upgrade请求
协议: webcam-stream
服务器 → 客户端: 101 Switching Protocols
```

### 数据传输
- **类型**: Binary (JPEG图像数据)
- **方向**: 服务器 → 客户端（单向推送）
- **帧率**: ~30 FPS
- **图像质量**: JPEG 80%

## 性能优化建议

1. **调整JPEG质量**：修改`webcam_capture.cpp`中的`IMWRITE_JPEG_QUALITY`参数
2. **修改分辨率**：在`init()`函数中修改`CAP_PROP_FRAME_WIDTH/HEIGHT`
3. **帧率控制**：调整`captureLoop()`中的`sleep_for`时间

## 常见问题

### Q: 无法打开摄像头？
A: 检查设备权限和驱动
```bash
sudo usermod -a -G video $USER
ls -l /dev/video0
```

### Q: WebSocket连接失败？
A: 确认端口未被占用
```bash
netstat -tulpn | grep 9000
```

### Q: 画面延迟高？
A: 降低图像质量或分辨率，检查网络状况

## 学习要点

通过这个项目，你将学习到：

1. **WebSocket通信**
   - 协议升级过程
   - 二进制数据传输
   - 客户端连接管理

2. **摄像头编程**
   - V4L2设备访问
   - OpenCV视频捕获
   - 图像编码技术

3. **多线程编程**
   - 线程同步（mutex）
   - 线程安全的数据共享
   - 生产者-消费者模式

4. **前端实时通信**
   - WebSocket JavaScript API
   - Canvas图像渲染
   - 性能监控

## 扩展方向

- [ ] 添加音频传输
- [ ] 支持多路摄像头
- [ ] 实现PTZ控制
- [ ] 添加H.264编码
- [ ] 实现录像功能
- [ ] 添加用户认证

