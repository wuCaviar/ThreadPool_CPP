cmake_minimum_required(VERSION 3.15)
project(threadPoll_cpp)

file(GLOB DIR_SRCS ${PROJECT_SOURCE_DIR}/src/*.cpp) # 搜索当前目录下的所有.cpp文件

include_directories(${PROJECT_SOURCE_DIR}/include) # 头文件目录
include_directories(${PROJECT_SOURCE_DIR}/src) # 源文件目录
add_library(threadPoll_cpp SHARED ${DIR_SRCS}) # 生成动态库

target_link_libraries(threadPoll_cpp pthread) # 链接pthread库

add_executable(threadPoll_cpp_app main.cpp) # 生成可执行文件

target_link_libraries(threadPoll_cpp_app threadPoll_cpp) # 链接threadPoll_cpp库