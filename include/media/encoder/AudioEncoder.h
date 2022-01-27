#pragma once
#include <cstdint>
#include <chrono>
#include <iostream>
#include <map>
#include <list>
#include <other/loggerApi.h>
#include <memory>
#include <mutex>
#include <vector>
#include <media/other/AudioUtil.hpp>
#include <media/decoder/AudioDecoder.h>

extern "C" {
#include <libswresample/swresample.h>
#include <libavcodec/avcodec.h>
};

#define AAC_BUF 4096
#define AAC_PCM_SAMPLE 1024
#define INPUT_NUM 30
#define CACHECONTAINER 1024*30
#define RTP_PACKET_SIZE 1024*2

//pkt�������
struct pkt_list_info {
  std::list<AVPacket*> pkt_list{};

  std::unique_ptr<std::mutex> pkt_list_mu = std::make_unique<std::mutex>();

  std::unique_ptr<std::condition_variable> pkt_list_cv = std::make_unique<std::condition_variable>();
};

//��Ƶ����

class EncoderImpl {

  enum MuxType muxType;
  enum CodecType codec_type;
  AudioInfo in_info;//������Ƶ����Ƶ����
  AudioInfo out_info;//�����Ƶ����Ƶ����
  AVCodecContext* enc_ctx;
  bool finished;
  struct pkt_list_info pktList;
  AVFrame* tmpFrame;//������Ľ���������ʱת����avframe
  ReSample resampler;
  bool resamplerInited = false;
  int nb_sample = 0;

  static const int ADTS_HEADER_LENGTH = 7;

  //����ÿ��û�����������
  uint8_t* dataCache;

  //��¼�������ݵĴ�С
  int dataCacheIndex = 0;

  int cacheLength = 0; //��ԭ����CACHECONTAINER����


  static AVFrame* allocAudioFrame(AVSampleFormat sample_fmt, uint64_t channel_layout,
    int sample_rate, int nb_samples) {
    AVFrame* frame = av_frame_alloc();
    int ret;

    if (!frame) {
      throw std::runtime_error("Error allocating an audio frame");
    }

    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if (nb_samples) {
      ret = av_frame_get_buffer(frame, 0);
      if (ret < 0) {
        throw std::runtime_error("AacAdtsEncoder: Error allocating an audio buffer");
      }
    }
    D_LOG("allocAudioFrame, frame->linesize[0]={}", frame->linesize[0]);
    return frame;
  }


  int writeAdtsHeaders(uint8_t* header, int dataLength);


public:

  EncoderImpl();

  void init(AudioInfo in, AudioInfo out, enum CodecType codecType);


  ~EncoderImpl();

  void addFrame(uint8_t* buf, const size_t bufSize, bool endOfStream = false);

  AVPacket* getPacket();

  void setMuxType(enum MuxType type);

  int mux(uint8_t* in, int in_len, uint8_t* out);

  int copyParams(AVCodecParameters* codecpar);
};