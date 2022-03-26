//
// Created by darrenzeng on 2021/6/13.
//

#include "TXIOCanary.h"

void iocanary::IoCanary::OnOpen(int fd, std::string file_path, std::string java_stack) {
    FileInfo *fileInfo = new FileInfo(file_path, java_stack);
    info_map.insert(std::make_pair(fd, fileInfo));
}

void iocanary::IoCanary::OnRead(int fd, int count, long cost_time) {
    if(info_map.find(fd) == info_map.end()){
        return;
    }
    FileInfo *fileInfo = info_map.at(fd);
    fileInfo->buffer_size = count;
    fileInfo->total_cost_time += cost_time;
}

void iocanary::IoCanary::OnWrite(int fd, int count, long cost_time) {
    if(info_map.find(fd) == info_map.end()){
        return;
    }
    FileInfo *fileInfo = info_map.at(fd);
    fileInfo->buffer_size = count;
    fileInfo->total_cost_time += cost_time;
}

void iocanary::IoCanary::OnClose(int fd) {
    __android_log_print(ANDROID_LOG_ERROR, "TAG", "OnClose");


    if(info_map.find(fd) == info_map.end()){
        return;
    }
    FileInfo *fileInfo = info_map.at(fd);
    if(fileInfo->buffer_size < 4096){
        // 最后在这里做警告弹窗，是否合法，输出到 java 层, debug 包抛一个弹窗
        __android_log_print(ANDROID_LOG_ERROR, "TAG", "缓冲区域太小：%d", fileInfo->buffer_size);
        __android_log_print(ANDROID_LOG_ERROR, "TAG", "%s", fileInfo->java_stack_.c_str());
    }
    if(fileInfo ->total_cost_time > 100){
        __android_log_print(ANDROID_LOG_ERROR, "TAG", "主线程操作文件太耗时：%d", fileInfo->total_cost_time/1000);
        __android_log_print(ANDROID_LOG_ERROR, "TAG", "%s", fileInfo->java_stack_.c_str());
    }
    info_map.erase(fd);
    delete fileInfo;
}
