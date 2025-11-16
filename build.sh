#!/bin/bash

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=================================${NC}"
echo -e "${GREEN}WebSocket摄像头流编译脚本${NC}"
echo -e "${GREEN}=================================${NC}"

# 检查依赖
echo -e "\n${YELLOW}检查依赖...${NC}"

check_package() {
    if pkg-config --exists $1; then
        echo -e "✓ $1 已安装"
        return 0
    else
        echo -e "${RED}✗ $1 未安装${NC}"
        return 1
    fi
}

ALL_DEPS=true
check_package "libwebsockets" || ALL_DEPS=false
check_package "opencv4" || ALL_DEPS=false

if [ "$ALL_DEPS" = false ]; then
    echo -e "\n${RED}缺少必要的依赖！${NC}"
    echo -e "${YELLOW}请运行以下命令安装：${NC}"
    echo "sudo apt-get update"
    echo "sudo apt-get install -y libwebsockets-dev libopencv-dev pkg-config"
    exit 1
fi

# 创建构建目录
echo -e "\n${YELLOW}创建构建目录...${NC}"
mkdir -p build
cd build

# 运行CMake
echo -e "\n${YELLOW}运行CMake配置...${NC}"
cmake .. || {
    echo -e "${RED}CMake配置失败！${NC}"
    exit 1
}

# 编译
echo -e "\n${YELLOW}开始编译...${NC}"
bear --append -- make -j$(nproc) || {
    echo -e "${RED}编译失败！${NC}"
    exit 1
}

cp compile_commands.json ..

echo -e "\n${GREEN}=================================${NC}"
echo -e "${GREEN}编译成功！${NC}"
echo -e "${GREEN}=================================${NC}"

# 检查摄像头
echo -e "\n${YELLOW}检查摄像头设备...${NC}"
if ls /dev/video* 1> /dev/null 2>&1; then
    echo -e "${GREEN}找到摄像头设备：${NC}"
    ls -la /dev/video*
else
    echo -e "${RED}未找到摄像头设备！${NC}"
    echo "请确保摄像头已连接并且驱动已加载"
fi

echo -e "\n${YELLOW}运行程序：${NC}"
echo "./webcam_server [设备ID] [端口]"
echo -e "${YELLOW}示例：${NC}"
echo "./webcam_server 0 9000"
echo -e "\n${YELLOW}然后在浏览器中访问：${NC}"
echo "http://localhost:9000"