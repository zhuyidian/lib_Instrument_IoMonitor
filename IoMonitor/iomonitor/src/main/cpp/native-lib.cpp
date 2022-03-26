#include <jni.h>
#include <string>
#include <fcntl.h>
#include "xhook.h"
#include <android/log.h>
#include <fstream>
#include <unistd.h>
#include "TXIOCanary.h"

using namespace iocanary;
IoCanary *ioCanary;

JNIEnv *jniEnv;
jclass file_io_monitor_java_class;
jmethodID get_java_stack_mid;

int64_t getTickCount() {
    timeval tv;
    gettimeofday(&tv, 0);
    return (int64_t) tv.tv_sec * 1000000 + (int64_t) tv.tv_usec;
}

void Init(JNIEnv *env) {
    jniEnv = env;
    // 拿到 mid
    file_io_monitor_java_class = jniEnv->FindClass("com/dunn/instrument/iomonitor/FileIOMonitor");
    get_java_stack_mid = jniEnv->GetStaticMethodID(file_io_monitor_java_class,
                                                   "getJavaStack", "()Ljava/lang/String;");
}

char *jstring2Chars(JNIEnv *jniEnv, jstring jstr) {
    if (jstr == nullptr) {
        return nullptr;
    }
    const char *str = jniEnv->GetStringUTFChars(jstr, JNI_FALSE);
    char *ret = strdup(str);
    jniEnv->ReleaseStringUTFChars(jstr, str);
    return ret;
}

bool isMainThread() {
    return getpid() == gettid();
}

int ProxyOpen(const char *path, int flags, mode_t mode) {
    __android_log_print(ANDROID_LOG_ERROR, "TAG", "监听到文件打开：%s", path);
    // 对接的 sdk 业务方，腾讯新闻，腾讯QQ 是不允许在主线程中做 IO 操作
    // 其他业务不允许长时间的 IO 操作，主线程的 IO 操作耗时超过 100ms 会警告，腾讯微视，腾讯体育，腾讯视频
    // 如果是第一种情况，我们在这里就可以抛异常了，判断是不是主线程操作 IO
    // 但是我们要讲的是第二种
    // 1. 要搞个 map 存起来
    if (!isMainThread()) {
        return open(path, flags, mode);
    }
    int fd = open(path, flags, mode);

    // 这里获取堆栈
    // 获取 java 的堆栈信息
    jstring javaStack = (jstring) jniEnv->CallStaticObjectMethod(file_io_monitor_java_class,
                                                                 get_java_stack_mid);
    // jstring -> char*
    char *java_stack = jstring2Chars(jniEnv, javaStack);
    ioCanary->OnOpen(fd, path, java_stack);
    // free 资源
    free(java_stack);
    jniEnv -> DeleteLocalRef(javaStack);
    return fd;
}


// 2. 监听到 close
int ProxyClose(int fd) {
    __android_log_print(ANDROID_LOG_ERROR, "TAG", "监听到文件关闭：%d", fd);
    // 3. 关闭时间 - 从打开文件的 map 信息中获取开始时间，看主线程操作了多久？ 500ms 报异常
    // 直接算可能不正常的时间
    // 但是如果是每次读的累加时间算就是正常的
    if(!isMainThread()){
        return close(fd);
    }
    int res = close(fd);
    ioCanary -> OnClose(fd);
    return res;
}

// 4. 一般要给一些什么信息呢？
// 4.1 线程：main ，子线程
// 4.2 读写的 buffer 大小
// 4.3 操作时间：真实的读写时间

// 5.监控文件的读
ssize_t ProxyRead(int fd, void *buf, size_t count) {
    // 这这里如果小于一个 pagesize 无论怎样都需要提示，buffer太小不合法，怎么提示大家先自己写一写
    // __android_log_print(ANDROID_LOG_ERROR, "TAG", "监听到文件读：%d", count);
    // 在这里我们才能记录到真实的读写时间，把读写时间累加

    if (!isMainThread()) {
        return read(fd, buf, count);
    }
    // 开始时间
    // time(NULL);  ms = 0
    int64_t start = getTickCount();
    ssize_t read_res = read(fd, buf, count);
    int64_t cost_time = getTickCount() - start;
    // 累加每次的读时间 = 结束时间 - 开始时间
    ioCanary->OnRead(fd, count, cost_time);
    return read_res;
}

// 6.监控文件的写
ssize_t ProxyWrite(int fd, const void *buf, size_t count) {
    // __android_log_print(ANDROID_LOG_ERROR, "TAG", "监听到文件写：%d", count);
    // 开始时间
    if (!isMainThread()) {
        return write(fd, buf, count);
    }
    int64_t start = getTickCount();
    ssize_t write_res = write(fd, buf, count);
    int64_t cost_time = getTickCount() - start;
    ioCanary->OnWrite(fd, count, cost_time);
    // 累加每次的读时间 = 结束时间 - 开始时间
    return write_res;
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv((void **) (&env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    Init(env);
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT void JNICALL
Java_com_dunn_instrument_iomonitor_FileIOMonitor_hook(JNIEnv *env, jclass clazz) {
    ioCanary = new IoCanary();

    xhook_register("libopenjdkjvm.so", "open", (void *) ProxyOpen, NULL);
    xhook_register("libjavacore.so", "open", (void *) ProxyOpen, NULL);
    xhook_register("libopenjdk.so", "open", (void *) ProxyOpen, NULL);
    // 这个 Android 7.0 以上会出现的一些，这个代码严格意义上是不正常的，因为统一调用了 open 方法
    // 下次课会给到大家另外一个版本，这个版本是微信的方案
    xhook_register("libopenjdkjvm.so", "open64", (void *) ProxyOpen, NULL);
    xhook_register("libjavacore.so", "open64", (void *) ProxyOpen, NULL);
    xhook_register("libopenjdk.so", "open64", (void *) ProxyOpen, NULL);
    // 文件关闭
    xhook_register("libopenjdkjvm.so", "close", (void *) ProxyClose, NULL);
    xhook_register("libjavacore.so", "close", (void *) ProxyClose, NULL);
    xhook_register("libopenjdk.so", "close", (void *) ProxyClose, NULL);
    // 文件读
    xhook_register("libopenjdkjvm.so", "read", (void *) ProxyRead, NULL);
    xhook_register("libjavacore.so", "read", (void *) ProxyRead, NULL);
    xhook_register("libopenjdk.so", "read", (void *) ProxyRead, NULL);
    // 文件写
    xhook_register("libopenjdkjvm.so", "write", (void *) ProxyWrite, NULL);
    xhook_register("libjavacore.so", "write", (void *) ProxyWrite, NULL);
    xhook_register("libopenjdk.so", "write", (void *) ProxyWrite, NULL);
    // 及时生效
    xhook_refresh(1);

}