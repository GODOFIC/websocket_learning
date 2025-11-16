#!/bin/bash

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}=================================${NC}"
echo -e "${YELLOW}清理构建文件${NC}"
echo -e "${YELLOW}=================================${NC}"

if [ "$1" = "full" ]; then
    echo -e "\n${YELLOW}完全清理（删除build目录）...${NC}"
    rm -rf build
    echo -e "${GREEN}✓ 完全清理完成${NC}"
else
    if [ -d "build" ]; then
        echo -e "\n${YELLOW}部分清理（保留CMake配置）...${NC}"
        cd build && make clean
        echo -e "${GREEN}✓ 部分清理完成${NC}"
    else
        echo -e "${RED}build目录不存在，无需清理${NC}"
    fi
fi

echo -e "\n${YELLOW}使用方法：${NC}"
echo "./clean.sh       - 部分清理（保留CMake配置）"
echo "./clean.sh full  - 完全清理（删除build目录）"
