#pragma once
#include <iostream>
#include <other/loggerApi.h>
//对于C++而言的网络库，需要引入下面的头文件和lib库

//接收方实现组播接收需要加入组播，才能接收到组播消息
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
//引入ws2tcpip来使用inet_pton
#include <ws2tcpip.h> 
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "wsock32.lib")
#endif
//引入ws2ipdef来使用IP_MREQ
#include <ws2ipdef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
//引入thread来使用this_thread进行轮询
#include <thread>

extern "C" {
#include <libavformat/avformat.h>
};

//#define ADTS_HEADER_LENGTH 7
#define MAXDATASIZE 1500
#define AAC_FILE "./recvAAC.aac"
#define BUFFER_SIZE	10

struct NetManager {
	NetManager();
	~NetManager();

  //todo rtmp
  int sendRTMPData(AVPacket* packet);
  int setVideoStream(AVCodecContext* encodeCtx);
  int setAudioStream(AVCodecContext* encodeCtx);
  int stopRTMP();

  int rtmpInit(int step);
  void setRtmpUrl(std::string _rtmpUrl);
  AVStream* getAudioStream() {
    return audio_st;
  }
  AVStream* getVideoStream() {
    return video_st;
  }
  AVFormatContext* getOutFormatCtx() {
    return pFormatCtx;
  }

  int newAudioStream(AVCodecContext* encodeCtx);

  int newVideoStream(AVCodecContext* encodeCtx);

private:
  AVFormatContext* pFormatCtx = nullptr;
  AVOutputFormat* fmt = nullptr;
  AVStream* video_st = nullptr;
  AVStream* audio_st = nullptr;
  int64_t cur_pts_v = 0, cur_pts_a = 0;
  int frame_index_v = 0;
  int frame_index_a = 0;

  std::string rtmpUrl = "rtmp://live-push.bilivideo.com/live-bvc/?streamname=live_352390639_21965190&key=24f4401750f2d2d7c1590021bc0946d0&schedule=rtmp&pflag=9";
  std::unique_ptr<std::mutex> mutex4Sender = nullptr;
};