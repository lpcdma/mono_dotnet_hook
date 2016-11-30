#pragma once
#include <android/log.h>

#define LOG_TAG "XMONO"
#define LOGD(fmt, args...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG, fmt, ##args)
#define LOGE(fmt, args...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG, fmt, ##args)
#define LOGW(fmt, args...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG, fmt, ##args)