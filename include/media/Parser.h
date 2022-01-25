/**
 * Copyright (c) 2020, SeekLoud Team.
* Date: 2020/6/1
    * Main Developer: zwq
    * Developer:xsr, gy
* Date: 2020/10/1
    * Main Developer: gy
    * Developer:xsr
 * Description: ��װ���װ
 * Refer:
 */

#pragma once
#include <media/common.h>
#include <queue>
 //extern "C" {
 //#include <libavcodec/avcodec.h>
 //#include <libavfilter/buffersink.h>
 //#include <libavfilter/buffersrc.h>
 //#include <libavformat/avformat.h>
 //#include <libavutil/avutil.h>
 //#include <libavutil/fifo.h>
 //#include <libavutil/opt.h>
 //#include <libavutil/pixdesc.h>
 //#include <libswresample/swresample.h>
 //#include "libavutil/imgutils.h"
 //#include "libswscale/swscale.h"
 //}

#define MTU 1300  //һ��FUA����󳤶ȣ�jrtplib�ж�packet����󳤶�����Ϊ1400


class Parser {
private:

  //��װ���
  int SPS_LEN = 1024;
  int SPS_INTERVAL = 30; //ÿ��30֡��һ��sps
  uint64_t frameCount = 0;
  bool needUpdate = false; //���ı��������update
  uint8_t* spsData = nullptr;
  size_t spsLen = 0;

  //���װ���
  std::queue<std::tuple<uint8_t*, size_t, uint64_t>> output_h264Data;
  uint8_t* buff = nullptr;
  int position = 0;
  unsigned int h264_startcode = 0x01000000;

public:
  Parser();
  ~Parser();

  /* H264���ݷ�װ��NALUƬ�Σ�rtp payload��
  * input_pkt: �����H264AVpacket
  * outData: �����Ƭ���NALUƬ�Σ�����ֱ�ӷ���rtp payload��
  */
  void H264Parse2NALU(AVPacket* input_pkt, std::vector<std::pair<uint8_t*, int>>& outData);

  /* ��������װ��NALUƬ�Σ�rtp payload��
  * loaddata: �����NALUƬ��, rtp��payload
  * len: �������ݳ���
  */
  void inputPayLoad(uint8_t* loaddata, int len, uint64_t timestamp);

  /* ��ȡƴ�Ӻõ�H264���ݣ����װ��
  * h264Data: �����ƴ�Ӻõ�h264����
  */
  int getH264(uint8_t* h264Data, uint64_t& timestamp);

  /* ���ı����������ߣ��󣬸���sps��Ϣ
  * sps: �����sps����
  * sps: �������ݳ���
  */
  void updateSpsMsg(uint8_t* sps, size_t spsSize);
};