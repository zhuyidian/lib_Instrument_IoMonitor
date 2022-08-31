# IoMonitor

## 1.使用
* 1，初始化
```xml
FileIOMonitor.start();
```
## 2.项目引用
* 1，root build.gradle中
```groovy
classpath 'com.hujiang.aspectjx:gradle-android-plugin-aspectjx:2.0.8'
```
* 2，module build.gradle中
```groovy
apply plugin: 'android-aspectjx'
implementation 'com.github.zhuyidian.lib_Instrument:excel:V1.1.8'
```
## 3.版本更新
* V1.0.0
```
首次成功运行版本
```
## 4.说明
实现的效果：

其他业务不允许主线程长时间的 IO 操作，主线程的 IO 操作耗时超过 100ms 会警告，腾讯微视，腾讯体育，腾讯视频
1. 主线程读写文件真实耗时 > 100MS 抛警告弹窗，弹窗里面需要的信息（读写的buffer大小，操作的文件，耗时的时间，操作堆栈信息）
2. 读写时间没啥问题，但是操作的 buffer < page_size 弹窗（读写的buffer大小，操作的文件，耗时的时间，操作堆栈信息）
3. 打开操作了文件没有关闭，警告弹窗（操作的堆栈）动态代理
