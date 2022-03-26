package com.dunn.instrument.iomonitor;

import android.util.Log;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

public class FileIOMonitor {

    static {
        System.loadLibrary("iomonitor-lib");
    }

    private static native void hook();

    public static void start() {
        hook();
        // 开始监控文件有没有关闭
        hookCloseGuardReport();
    }

    private static void hookCloseGuardReport() {
        // 通过反射先获取到 CloseGuard
        try {
            Class<?> closeGuardCls = Class.forName("dalvik.system.CloseGuard");
            // 有可能是版本的问题，可以遍历看一下
            Method getReporterMethod = closeGuardCls.getDeclaredMethod("getReporter", null);
            getReporterMethod.setAccessible(true);
            Object reporter = getReporterMethod.invoke(null);

            Method setEnableMethod = closeGuardCls.getDeclaredMethod("setEnabled", boolean.class);
            setEnableMethod.setAccessible(true);
            setEnableMethod.invoke(null, true);
            // 设置一个新的代理对象
            Class<?> reportInterfaceClass = Class.forName("dalvik.system.CloseGuard$Reporter");
            Object reporterProxy = Proxy.newProxyInstance(FileIOMonitor.class.getClassLoader(),
                    new Class<?>[]{reportInterfaceClass}, new IoCloseInvocationHandler(reporter));
            Method setReporterMethod = closeGuardCls.getDeclaredMethod("setReporter", reportInterfaceClass);
            setReporterMethod.setAccessible(true);
            setReporterMethod.invoke(null, reporterProxy);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * called for jni
     */
    private static String getJavaStack() {
        StackTraceElement[] stackTraceElements = new Throwable().getStackTrace();
        StringBuilder sb = new StringBuilder();
        for (StackTraceElement stackTraceElement : stackTraceElements) {
            if (stackTraceElement.toString().contains(FileIOMonitor.class.getName())) {
                continue;
            }
            sb.append(stackTraceElement.toString()).append("\r\n");
        }
        return sb.toString();
    }

    public static class IoCloseInvocationHandler implements InvocationHandler {
        private Object originalReporter;

        public IoCloseInvocationHandler(Object reporter) {
            this.originalReporter = reporter;
        }

        @Override
        public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
            if ("report".equals(method.getName())) {
                Log.e("TAG", "文件打开没有关闭");
                // 文件泄漏了，打开没有关闭
                Throwable throwable = (Throwable) args[1];
                // 弹窗一个警告弹窗
                throwable.printStackTrace();
            }
            return method.invoke(originalReporter, args);
        }
    }
}
