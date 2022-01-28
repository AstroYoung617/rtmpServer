//AudioDecoder.h
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

extern "C" {
#include <libswresample/swresample.h>
#include <libavcodec/avcodec.h>
};

#define AAC_BUF 4096
#define AAC_PCM_SAMPLE 1024
#define INPUT_NUM 30
#define CACHECONTAINER 1024*30
#define RTP_PACKET_SIZE 1024*2
class adts_header_t {
public:
  unsigned char syncword_0_to_8 : 8;

  unsigned char protection_absent : 1;
  unsigned char layer : 2;
  unsigned char ID : 1;
  unsigned char syncword_9_to_12 : 4;

  unsigned char channel_configuration_0_bit : 1;
  unsigned char private_bit : 1;
  unsigned char sampling_frequency_index : 4;
  unsigned char profile : 2;

  unsigned char frame_length_0_to_1 : 2;
  unsigned char copyrignt_identification_start : 1;
  unsigned char copyright_identification_bit : 1;
  unsigned char home : 1;
  unsigned char original_or_copy : 1;
  unsigned char channel_configuration_1_to_2 : 2;

  unsigned char frame_length_2_to_9 : 8;

  unsigned char adts_buffer_fullness_0_to_4 : 5;
  unsigned char frame_length_10_to_12 : 3;

  unsigned char number_of_raw_data_blocks_in_frame : 2;
  unsigned char adts_buffer_fullness_5_to_10 : 6;
};

//音频参数
struct AudioInfo {
  int sample_rate;
  enum AVSampleFormat sample_fmt;
  int channels;
  int64_t channel_layout;
  int bit_rate = 0;
  int pcmaTimeSeg = 0;

  bool operator==(const AudioInfo& b) {
    return sample_rate == b.sample_rate &&
      channels == b.channels &&
      channel_layout == b.channel_layout &&
      sample_fmt == b.sample_fmt &&
      pcmaTimeSeg == b.pcmaTimeSeg;
  }

  void copyFrom(AudioInfo* other) {
    setInfo(other->sample_rate, other->sample_fmt,
      other->channels, other->bit_rate, other->pcmaTimeSeg);
  }

  void setInfo(int sr, enum AVSampleFormat sf, int ch) {
    sample_rate = sr;
    channels = ch;
    channel_layout = av_get_default_channel_layout(ch);
    sample_fmt = sf;
    bit_rate = 0;
  }

  void setInfo(int sr, enum AVSampleFormat sf, int ch, int br) {
    sample_rate = sr;
    channels = ch;
    channel_layout = av_get_default_channel_layout(ch);
    sample_fmt = sf;
    bit_rate = br;
  }

  void setInfo(int sr, enum AVSampleFormat sf, int ch, double pt) {
    sample_rate = sr;
    channels = ch;
    channel_layout = av_get_default_channel_layout(ch);
    sample_fmt = sf;
    pcmaTimeSeg = pt;
  }

  void setInfo(int sr, enum AVSampleFormat sf, int ch, int br, double pt) {
    sample_rate = sr;
    channels = ch;
    channel_layout = av_get_default_channel_layout(ch);
    sample_fmt = sf;
    bit_rate = br;
    pcmaTimeSeg = pt;
  }
};

struct frame_list_info {
  std::list<AVFrame*> frame_list{};

  std::unique_ptr<std::mutex> pkt_list_mu = std::make_unique<std::mutex>();

  std::unique_ptr<std::condition_variable> pkt_list_cv = std::make_unique<std::condition_variable>();
};


class ReSample {
private:
  SwrContext* swr_ctx = nullptr;

  AudioInfo out_info;

  AudioInfo in_info;

public:
  ReSample(const ReSample&) = delete;

  ReSample(ReSample&&) noexcept = delete;

  ReSample operator=(const ReSample&) = delete;

  ReSample();

  ReSample(AudioInfo* out_codec, AudioInfo* in_codec);

  ~ReSample();

  AudioInfo* get_in_info() { return &in_info; }

  AudioInfo* get_out_info() { return &out_info; }

  int audio_swr_init(AudioInfo* out_codec, AudioInfo* in_codec);

  int get_out_size(int nb_samples);

  /**
   * resample: frame => uint8_t, 输入的audioFrame和targetData均需在外部释放
   * @param audioFrame 输入音频帧
   * @param targetData 输出数据
   * @return 重采样后的数据长度
   */
  int audio_swr_resampling_audio(AVFrame* audioFrame, uint8_t** targetData);

  /**
   * resample: uint8_t => uint8_t, 输入的srcData和targetData均需在外部释放
   * @param srcData 输入数据
   * @param targetData 输出数据
   * @param nb_samples 输入sample数
   * @return 重采样后的数据长度
   */
  int audio_swr_resampling_audio(uint8_t** srcData, uint8_t** targetData,
    int nb_samples);
};
class AuDecoder {

  struct frame_list_info frameList;

  AudioInfo in_info; //输入音频的音频格式

  AudioInfo out_info;//输出音频的音频个私

  FILE* refFile;

  enum CodecType codec_type;

  AVFrame* decoded_frame = nullptr;

  uint8_t* jitter_buffer = nullptr;//用来接收从jitter中返回的数据

  int nb_sample = 0;//每个解码后的pcm数据 sample数

  AVFrame* lossFrame = nullptr;

  AVPacket* pkt = nullptr; //将输入的uint8*缓存乘avpacket

  AVFrame* tmpFrame = nullptr;//将重采样之后的数据保存到这里

  AVFrame* jitterFrame = nullptr;//缓存从丢包补偿中获取的

  AVCodec* codec;

  AVCodecContext* dec_ctx = nullptr;

  AVCodecParserContext* parser = nullptr;

  bool reSamplerInited = false;

  ReSample reSampler;

  AVFrame* lastFrame = nullptr;

  MuxType demuxType = MuxType::ADTS;


  bool neteqOpen = false;

  int bit_rate = 0;

  int outBufferSize = 0;//原来的OUTBUFFERSIZE

//                AVRational stream_time_base;

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

  int demuxAdts(uint8_t* in, uint8_t* out);

public:

  AuDecoder();
  ~AuDecoder();

  void init(AudioInfo in, AudioInfo out, enum CodecType codecType, bool setNeteqOpen);

  void addPacket(uint8_t* in, int in_len, uint32_t timestamp, uint16_t sequence);

  AVFrame* getFrame();

  AVFrame* getLastFrame();

  void setDemuxType(enum MuxType type);

  void demux(uint8_t* in, std::pair<int&, uint8_t*> out);

};