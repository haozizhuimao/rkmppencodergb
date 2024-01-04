# 最小make版本
cmake_minimum_required(VERSION 3.5)


# 设置项目信息
project(encodecvideodemo VERSION 1.0)


# # 设置编译工具
if(UNIX)
    # set(CMAKE_C_COMPILER /usr/bin/gcc)
    # set(CMAKE_CXX_COMPILER /usr/bin/g++)
    # set(CMAKE_C_COMPILER /usr/bin/aarch64-linux-gnu-gcc)
	# set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)
endif()


# set(CMAKE_BUILD_TYPE "Release")

# # 设置编译器支持的C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#设置输出路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../bin)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../bin)
# link_directories(${LIBRARY_OUTPUT_PATH})
# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O3")

# # 设置安装路径
# set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/install)

# OpenCV
#set(OpenCV_DIR "/opencv/library")
find_package(OpenCV 4 REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
link_directories(${OpenCV_LIBRARY_DIRS})




# 添加头文件
# file(GLOB_RECURSE HEADERS include/*.h)

# 添加源文件
file(GLOB_RECURSE SOURCES encodecvideo.cpp muxingvideo.cpp main.cpp)

# 生成共享库
add_executable(${PROJECT_NAME}  ${HEADERS} ${SOURCES})

# #设置依赖路径

# 链接依赖库
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS}  -lavcodec -lavformat -lavutil -lswscale -lswresample -lpthread -lrockchip_mpp)



