LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := pcmfft
LOCAL_SRC_FILES := pcmfft.c kiss_fft.c
LOCAL_C_FLAGS = -DFIXED_POINT=16

include $(LOCAL_PATH)/sha1/Android.mk

LOCAL_C_FLAGS += -mtune=native -ffast-math -fomit-frame-pointer -unroll-loops -dA -fverbose-asm

include $(BUILD_EXECUTABLE)
