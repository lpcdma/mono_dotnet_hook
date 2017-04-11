LOCAL_PATH := $(call my-dir)

############# injector ##############
include $(CLEAR_VARS)

LOCAL_MODULE	:= injector
LOCAL_ARM_MODE	:= arm
LOCAL_CPP_EXTENSION	:= .cpp
LOCAL_C_INCLUDES	:= $(LOCAL_PATH)/include/
LOCAL_CFLAGS	+= -fvisibility=hidden
LOCAL_LDLIBS	+= -L$(SYSROOT)/usr/lib -llog
LOCAL_SRC_FILES	:= \
		injector\android-injector.cpp \
		injector\injector.cpp

include $(BUILD_EXECUTABLE)

############# mono ##############
include $(CLEAR_VARS)

LOCAL_MODULE := mono
LOCAL_SRC_FILES := libmono.so
LOCAL_CFLAGS += -fvisibility=hidden

include $(PREBUILT_SHARED_LIBRARY)

############# mainso ##############
include $(CLEAR_VARS)

LOCAL_MODULE	:= libmonohook
LOCAL_ARM_MODE	:= arm
LOCAL_CPP_EXTENSION	:= .cpp
LOCAL_C_INCLUDES	:= $(LOCAL_PATH)/include/
LOCAL_SHARED_LIBRARIES	+= mono
LOCAL_LDLIBS	+= -L$(SYSROOT)/usr/lib -llog -lz
LOCAL_CFLAGS	+= -fvisibility=hidden
LOCAL_CPPFLAGS	+= -fexceptions

LOCAL_SRC_FILES	:= \
		core\common-help.cpp \
		core\mono-help.cpp \
		core\armhook.cpp \
		core\game-plugin.cpp \
		core\dotnet-hook.cpp

include $(BUILD_SHARED_LIBRARY)

############# install ##############
include $(CLEAR_VARS)

dest_path	:= /data/local/tmp

all:
	adb push $(NDK_APP_DST_DIR)/injector $(dest_path)/
	adb push $(NDK_APP_DST_DIR)/libmonohook.so $(dest_path)/
	adb shell "chmod 755 $(dest_path)/*.*"