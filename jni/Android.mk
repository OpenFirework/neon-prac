# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_ARM_MODE := arm
TARGET_ARCH_ABI := armeabi-v7a
LOCAL_ARM_NEON := true
LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
LOCAL_PRELINK_MODULE :=false  

LOCAL_CPPFLAGS += -std=c++11 -fPIC \
                  -DANDROID -marm -mfloat-abi=softfp -mfpu=neon -march=armv7-a \
                  -Wl,--no-warn-mismatch -llog -ldl \
		              -fvisibility=hidden -fvisibility-inlines-hidden \
                  -fomit-frame-pointer -fstrict-aliasing -ffunction-sections -fdata-sections -ffast-math
LOCAL_LDFLAGS += -fuse-ld=bfd -fPIC -march=armv7-a \
                  -Wl,--no-warn-mismatch -llog -ldl -lstdc++ 

LOCAL_C_INCLUDES += $(LOCAL_PATH)/.. 
                  

LOCAL_MODULE := neon_test

LOCAL_SRC_FILES := ../main.cpp

include $(BUILD_EXECUTABLE)


