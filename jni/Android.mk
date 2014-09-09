LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := pcm_dejavu
LOCAL_SRC_FILES := pcm_dejavu.c kiss_fft.c
LOCAL_CFLAGS := -DFIXED_POINT=16 -std=c1x -std=c++11

include $(LOCAL_PATH)/sha1/Android.mk

LOCAL_CFLAGS += -ffast-math -fomit-frame-pointer -unroll-loops -dA -fverbose-asm

include $(BUILD_EXECUTABLE)
