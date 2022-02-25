/*
  * Created by Astro
  * Date : 2022.01.27
  * Descryption:
    *		用于将videoReceiver接收到的yuv视频数据编码为h.264，交付给netmanager发送到rtmpURL
*/
#pragma once
#include <stdio.h>
#include <iostream>
#include <other/loggerApi.h>
#include <media/encoder/VideoEncoder.h>
#include <net/NetManager.h>
#include <media/other/common.h>
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
//视频分辨率相关的计算
struct VideoDefinition {
  int width;
  int height;
  VideoDefinition(int _width, int _height) : width(_width), height(_height) {};

  VideoDefinition getEven() const {
    int evenWidth = ((int)width % 2 == 0) ? (int)width : (int)width - 1;
    int evenHeight = ((int)height % 2 == 0) ? (int)height : (int)height - 1;
    VideoDefinition evenDefinition = { evenWidth, evenHeight };
    return evenDefinition;
  }

  std::string getString() const {
    std::string str = std::to_string(width) + "x" + std::to_string(height);
    return str;
  }

  bool operator==(const VideoDefinition& definition) const {
    if ((definition.width == this->width) && (definition.height == this->height)) {
      return true;
    }
    else {
      return false;
    }
  }

  bool operator!=(const VideoDefinition& definition) const {
    if ((definition.width != this->width) || (definition.height != this->height)) {
      return true;
    }
    else {
      return false;
    }
  }

  bool operator<=(const VideoDefinition& definition) const {
    if ((this->width <= definition.width) && (this->height <= definition.height)) {
      return true;
    }
    else {
      return false;
    }
  }
};

class VideoSenderUtil {
public:
  static VideoDefinition calEncoderSize(const VideoDefinition& captureSize,
    const VideoDefinition& maxDefinition) {
    if (captureSize <= maxDefinition) {
      return captureSize.getEven();
    }
    else {
      double captureProportion = (double)captureSize.width / (double)captureSize.height;
      double maxProportion = (double)maxDefinition.width / (double)maxDefinition.height;
      if (captureProportion > maxProportion) {
        VideoDefinition dstDefinition = { (int)maxDefinition.width,
                                         (int)((double)maxDefinition.width / captureProportion) };
        return dstDefinition.getEven();
      }
      else {
        VideoDefinition dstDefinition = { (int)((double)maxDefinition.height * captureProportion),
                                         (int)maxDefinition.height };
        return dstDefinition.getEven();
      }
    }

  }

  static int calBitRate(const VideoDefinition& encoderSize) {
    int bitRate = 0;
    if (encoderSize.height <= 144) {
      bitRate = 100000;
    }
    else if (144 < encoderSize.height && encoderSize.height <= 240) {
      bitRate = 200000;
    }
    else if (240 < encoderSize.height && encoderSize.height <= 360) {
      bitRate = 400000;
    }
    else if (360 < encoderSize.height && encoderSize.height <= 480) {
      bitRate = 600000;
    }
    else if (480 < encoderSize.height && encoderSize.height <= 720) {
      bitRate = 1200000;
    }
    else {
      bitRate = 2000000;
    }
    return bitRate;
  }
};


struct VideoSender {
	VideoSender(std::mutex* _mutex, std::condition_variable* _vdcv, std::shared_ptr<NetManager> _netManager);
	~VideoSender();
  int stop();
  void setStartTime(long long _startTime);
  void initEncoder(const VideoDefinition& captureSize, int frameRate);
	int sendFrame(AVFrame* frame);
  int lastPts = 0;
private:
  //netManager
  std::shared_ptr<NetManager> netManager = nullptr;

	std::mutex* mutex;
	std::condition_variable* vdcv;

  Encoder* encoder = nullptr;
  std::unique_ptr<std::mutex> mutex4Encoder = nullptr;
  VideoDefinition maxDefinition = VideoDefinition(1280, 720);

  int count = 0;
  long long startTime;
  int frameRate;
};

