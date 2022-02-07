/*
  * Created by Astro
  * Date : 2022.01.27
  * Descryption:
    *		���ڽ�audioReceiver���յ���pcm��Ƶ���ݱ���Ϊaac��������netmanager���͵�rtmpURL
*/

#pragma once

#include <net/NetManager.h>
#include <media/other/AudioUtil.hpp>
#include <media/encoder/AudioEncoder.h>

class AudioSender {
public:
  //��ز���ͨ�����캯������
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
  std::unique_ptr<EncoderImpl> encoder;  // ��Ƶ������
  CodecType encoderCodecType;        // �����װ��ʽ
  int count = 0;
  long long startTime = 0;
  AudioInfo out;
  AudioInfo in;
  int lastPts = 0;
};


