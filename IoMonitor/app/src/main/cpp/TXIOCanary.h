//
// Created by darrenzeng on 2021/6/13.
//

#ifndef OPTIMIZE_DAY15_TXIOCANARY_H
#define OPTIMIZE_DAY15_TXIOCANARY_H

#include <string>
#include <map>
#include <jni.h>
#include <android/log.h>

namespace iocanary {

    /**
     * 文件的信息，java堆栈，文件路径，读写缓冲区的大小，真实的读写时间
     */
    class FileInfo {
    public:
        FileInfo(std::string file_path, std::string java_stack) :
                file_path_(file_path), java_stack_(java_stack) {

        }

        long total_cost_time = 0;
        long buffer_size = 0;
        std::string file_path_;
        std::string java_stack_;
    };

    /**
     * 具体的操作
     */
    class IoCanary {
    public:
        void OnOpen(int fd, std::string file_path, std::string java_stack);

        void OnRead(int fd, int count, long cost_time);

        void OnWrite(int fd, int count, long cost_time);

        void OnClose(int fd);

    private:
        std::map<int, FileInfo *> info_map;
    };

}


#endif //OPTIMIZE_DAY15_TXIOCANARY_H
