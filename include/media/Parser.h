/**
 * Copyright (c) 2020, SeekLoud Team.
* Date: 2020/6/1
    * Main Developer: zwq
    * Developer:xsr, gy
* Date: 2020/10/1
    * Main Developer: gy
    * Developer:xsr
 * Description: 封装解封装
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

#define MTU 1300  //一个FUA的最大长度，jrtplib中对packet的最大长度限制为1400


class Parser {
private:

  //封装相关
  int SPS_LEN = 1024;
  int SPS_INTERVAL = 30; //每隔30帧发一次sps
  uint64_t frameCount = 0;
  bool needUpdate = false; //更改编码参数后update
  uint8_t* spsData = nullptr;
  size_t spsLen = 0;

  //解封装相关
  std::queue<std::tuple<uint8_t*, size_t, uint64_t>> output_h264Data;
  uint8_t* buff = nullptr;
  int position = 0;
  unsigned int h264_startcode = 0x01000000;

public:
  Parser();
  ~Parser();

  /* H264数据封装成NALU片段（rtp payload）
  * input_pkt: 输入的H264AVpacket
  * outData: 输出分片后的NALU片段，可以直接放在rtp payload里
  */
  void H264Parse2NALU(AVPacket* input_pkt, std::vector<std::pair<uint8_t*, int>>& outData);

  /* 输入待解封装的NALU片段（rtp payload）
  * loaddata: 输入的NALU片段, rtp的payload
  * len: 输入数据长度
  */
  void inputPayLoad(uint8_t* loaddata, int len, uint64_t timestamp);

  /* 获取拼接好的H264数据（解封装）
  * h264Data: 输出的拼接好的h264数据
  */
  int getH264(uint8_t* h264Data, uint64_t& timestamp);

  /* 更改编码参数（宽高）后，更新sps消息
  * sps: 输入的sps数据
  * sps: 输入数据长度
  */
  void updateSpsMsg(uint8_t* sps, size_t spsSize);
};