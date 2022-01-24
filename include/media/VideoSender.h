//发送rtmp视频文件
#pragma once
#include <stdio.h>
#include <iostream>
#include <other/loggerApi.h>
#include <net/NetManager.h>
#include <media/common.h>
#include <utility>
#include <mutex>

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavformat/avformat.h"
#include <libavutil/time.h>
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#ifdef __cplusplus
};
#endif
#endif

struct VideoSender {
	VideoSender(std::mutex* _mutex, std::condition_variable* _vdcv);
	~VideoSender();

	void sendRtmpData();
private:
	std::mutex* mutex;
	std::condition_variable* vdcv;
};