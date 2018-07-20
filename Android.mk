LOCAL_PATH := $(call my-dir)

#================#

include $(CLEAR_VARS)

APP_STL := stlport_static

LOCAL_MODULE := clIntercept

#LOCAL_CPPFLAGS += -std=c++11

LOCAL_SHARED_LIBRARIES := libdl liblog
LOCAL_SRC_FILES += Src/dispatch.cpp Src/enummap.cpp Src/intercept.cpp Src/main.cpp Src/stubs.cpp
LOCAL_SRC_FILES += OS/OS_linux_common.cpp OS/OS_linux.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)/Src


include external/libcxx/libcxx.mk

include $(BUILD_SHARED_LIBRARY)
