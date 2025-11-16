cmake_minimum_required(VERSION 3.10)
project(WebcamWebSocketStream)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖库
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBWEBSOCKETS REQUIRED libwebsockets)
pkg_check_modules(OPENCV REQUIRED opencv4)

# 添加可执行文件
add_executable(webcam_server
    src/main.cpp
    src/webcam_capture.cpp
    src/websocket_server.cpp
)

# 包含头文件目录
target_include_directories(webcam_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${LIBWEBSOCKETS_INCLUDE_DIRS}
    ${OPENCV_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(webcam_server
    ${LIBWEBSOCKETS_LIBRARIES}
    ${OPENCV_LIBRARIES}
    pthread
)

# 编译选项
target_compile_options(webcam_server PRIVATE -Wall -Wextra)