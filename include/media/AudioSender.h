/*
  * Created by Astro
  * Date : 2022.01.27
  * Descryption:
    *		用于将audioReceiver接收到的pcm音频数据编码为aac，交付给netmanager发送到rtmpURL
*/

#pragma once

#include <net/NetManager.h>
#include <media/other/AudioUtil.hpp>
#include <media/encoder/AudioEncoder.h>

class AudioSender {
public:
  //相关参数通过构造函数传入
  AudioSender(std::shared_ptr<NetManager> _netManager);
  ~AudioSender();
  int stop();
  void initAudioEncoder(CoderInfo encoderInfo);
  int send(uint8_t* buf, int len);
  void setStartTime(long long _startTime);
private:
  //netManager
  std::shared_ptr<NetManager> netManager = nullptr;

  // encoder
  AudioInfo info, outInfo;
  std::unique_ptr<EncoderImpl> encoder;  // 音频编码器
  CodecType encoderCodecType;        // 编码封装格式
  int count = 0;
  long long startTime = 0;
  AudioInfo out;
  AudioInfo in;
  int lastPts = 0;
};


