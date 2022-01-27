#include "media/Decoder/AudioDecoder.h"
#include "media/other/common.h"
#include <string>


AuDecoder::AuDecoder() {
  I_LOG("audio Decoder construct");
}

void AuDecoder::init(AudioInfo in, AudioInfo out, enum CodecType codecType, bool setNeteqOpen) {
  in_info = in;
  out_info = out;

  refFile = fopen("/Users/hopeworld/Documents/实验室/2021下半年/audioengine/pcm/test.pcm", "wb");
  //初始化解码器

  if (codecType == CodecType::AAC) {
    codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
    if (!codec) {
      E_LOG("Codec not found");
      return;
    }

    parser = av_parser_init(codec->id);
    if (!parser) {
      E_LOG("Parser not found");
      return;
    }

    dec_ctx = avcodec_alloc_context3(codec);
    if (!dec_ctx) {
      E_LOG("Could not allocate audio codec context");
      return;
    }

    /* open it */
    if (avcodec_open2(dec_ctx, codec, NULL) < 0) {
      fprintf(stderr, "Could not open codec\n");
      exit(1);
    }

    //初始化重采样器
    if (!(in == out)) {
      if (reSampler.audio_swr_init(&out_info, &in_info) == 0) {
        reSamplerInited = true;
      }
    }
    D_LOG("Decoder reSampler is inited {}", reSamplerInited);


    //初始化成员变量

    if (!(decoded_frame = av_frame_alloc())) {
      E_LOG("Could not allocate audio frame");
      return;
    }

    pkt = av_packet_alloc();

    if (pkt == nullptr) {
      E_LOG("Could not allocate audio packet");
      return;
    }

    nb_sample = AAC_PCM_SAMPLE;
    jitter_buffer = new uint8_t[RTP_PACKET_SIZE];
    outBufferSize = 1024 * 20;
  }
  else if (codecType == CodecType::PCMA) {
    I_LOG("PCMA Decoder init");
    codec = avcodec_find_decoder(AV_CODEC_ID_PCM_ALAW);
    if (!codec) {
      fprintf(stderr, "Codec not found\n");
      return;
    }
    dec_ctx = avcodec_alloc_context3(codec);
    if (!dec_ctx) {
      fprintf(stderr, "Could not allocate audio codec context\n");
      return;
    }
    dec_ctx->sample_fmt = in.sample_fmt;
    dec_ctx->sample_rate = in.sample_rate;
    dec_ctx->channels = in.channels;
    /* open it */
    if (avcodec_open2(dec_ctx, codec, NULL) < 0) {
      fprintf(stderr, "Could not open codec\n");
      return;
    }
    pkt = av_packet_alloc();
    if (NULL == pkt)
    {
      return;
    }
    decoded_frame = av_frame_alloc();
    if (NULL == decoded_frame)
    {
      return;
    }
    if (!(in == out)) {
      if (reSampler.audio_swr_init(&out_info, &in_info) == 0) {
        reSamplerInited = true;
      }
    }
    nb_sample = in.sample_rate * in.pcmaTimeSeg / 1000;
    jitter_buffer = new uint8_t[nb_sample * 2];
    outBufferSize = nb_sample * 20;
    I_LOG("nbsamples {}", nb_sample);
    D_LOG("Decoder reSampler is inited {}", reSamplerInited);
  }
  else if (codecType == CodecType::AMRNB) {
    codec = avcodec_find_decoder(AV_CODEC_ID_AMR_NB);
    if (!codec) {
      E_LOG("Codec not found");
      return;
    }

    dec_ctx = avcodec_alloc_context3(codec);
    if (!dec_ctx) {
      E_LOG("Could not allocate audio codec context");
      return;
    }

    /* open it */
    if (avcodec_open2(dec_ctx, codec, NULL) < 0) {
      fprintf(stderr, "Could not open codec\n");
      exit(1);
    }

    bit_rate = in.bit_rate == 0 ? 12200 : in.bit_rate;
    //初始化重采样器
    if (!(in == out)) {
      if (reSampler.audio_swr_init(&out_info, &in_info) == 0) {
        reSamplerInited = true;
      }
    }
    D_LOG("Decoder reSampler is inited {}", reSamplerInited);


    //初始化成员变量

    if (!(decoded_frame = av_frame_alloc())) {
      E_LOG("Could not allocate audio frame");
      return;
    }

    pkt = av_packet_alloc();

    if (pkt == nullptr) {
      E_LOG("Could not allocate audio packet");
      return;
    }

    nb_sample = 160; // amr-nb为160
    jitter_buffer = new uint8_t[160 * 2];
    outBufferSize = nb_sample * 20;
    I_LOG("nbsamples {}", nb_sample);
    D_LOG("decoder reSampler is inited {}", reSamplerInited);
  }
  else if (codecType == CodecType::AMRWB) {
    codec = avcodec_find_decoder(AV_CODEC_ID_AMR_WB);
    if (!codec) {
      E_LOG("Codec not found");
      return;
    }

    dec_ctx = avcodec_alloc_context3(codec);
    if (!dec_ctx) {
      E_LOG("Could not allocate audio codec context");
      return;
    }

    /* open it */
    if (avcodec_open2(dec_ctx, codec, NULL) < 0) {
      fprintf(stderr, "Could not open codec\n");
      exit(1);
    }

    bit_rate = in.bit_rate == 0 ? 23850 : in.bit_rate;
    //初始化重采样器
    if (!(in == out)) {
      if (reSampler.audio_swr_init(&out_info, &in_info) == 0) {
        reSamplerInited = true;
      }
    }
    D_LOG("decoder reSampler is inited {}", reSamplerInited);


    //初始化成员变量

    if (!(decoded_frame = av_frame_alloc())) {
      E_LOG("Could not allocate audio frame");
      return;
    }

    pkt = av_packet_alloc();

    if (pkt == nullptr) {
      E_LOG("Could not allocate audio packet");
      return;
    }

    nb_sample = 320; // amr-wb为320
    jitter_buffer = new uint8_t[320 * 2];
    outBufferSize = nb_sample * 20;
    I_LOG("nbsamples {}", nb_sample);
    D_LOG("decoder reSampler is inited {}", reSamplerInited);
  }
  else {
    E_LOG("{} codec type is not supported ", codecType);
    return;
  }
  codec_type = codecType;

}


void AuDecoder::addPacket(uint8_t* in, int in_len, uint32_t timestamp, uint16_t sequence) {

  static long long startTime = 0;

  AVFrame* d_frame = nullptr;
  int ret = 0;

  int jitter_buffer_len = 0;

  int pcmaFrameSize = nb_sample * in_info.channels;
  int amrPacketSize = round(bit_rate * 1.0 / 50 / 8) + 1;
  //                I_LOG("pcmafz {}",pcmaFrameSize);
                  //如果是success状态,就进行正常解码，解码后的avframe保存在list，同时放入neteqController中
  uint8_t* data = in;
  int data_size = in_len;

  D_LOG("pop jitter buffer data size is {}", data_size);
  while (data_size > 0) {
    if (codec_type == CodecType::AAC) {
      ret = av_parser_parse2(parser, dec_ctx, &pkt->data, &pkt->size, data, data_size,
        AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0); //返回parse的长度
      if (ret < 0) {
        E_LOG("Error while parsing");
        return;
      }
    }
    else if (codec_type == CodecType::PCMA) {
      //                                ret = data_size;
      if (data_size >= pcmaFrameSize) {
        ret = pcmaFrameSize;
        pkt->size = ret;
        pkt->data = (uint8_t*)av_malloc(pkt->size);
        memcpy(pkt->data, data, ret);
      }
      else {
        //                                    I_LOG("data_size {} pcmafz{}",data_size,pcmaFrameSize);
        ret = 0;
        return;
      }
    }
    else if (codec_type == CodecType::AMRWB || codec_type == CodecType::AMRNB) {
      if (data_size >= amrPacketSize) {
        ret = amrPacketSize;
        pkt->size = ret;
        pkt->data = (uint8_t*)av_malloc(pkt->size);
        memcpy(pkt->data, data, ret);
      }
      else {
        ret = 0;
        return;
      }
    }

    data += ret;
    data_size -= ret;
    if (pkt->size) {
      //此处将avpacket放入jitter buffer中
      if (startTime == 0)
        startTime = GetTickCount64();
      ret = avcodec_send_packet(dec_ctx, pkt);
      if (codec_type != CodecType::AAC) {
        if (pkt->data) {
          av_free(pkt->data);
        }
      }
      D_LOG("avcodec_send_packet ret is {}", ret);
      if (ret < 0) {
        E_LOG("Error submitting the packet to the Decoder");
        return;
      }
      /* read all the output frames (in general there may be any number of them */
      while (ret >= 0) {
        if (!d_frame)
          d_frame = av_frame_alloc();
        ret = avcodec_receive_frame(dec_ctx, d_frame);
        D_LOG("avcodec_receive_frame is {}", ret);
        if (d_frame->nb_samples == 0) {
          av_frame_free(&d_frame);
          return;
        }
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
          av_frame_free(&d_frame);
          return;
        }
        else if (ret < 0) {
          av_frame_free(&d_frame);
          E_LOG("Error during decoding");
          return;
        }
        D_LOG("frame nb sample is {}", d_frame->nb_samples);
        if (startTime != 0) {
          D_LOG("decode cost time is {}", seeker::Time::microTime() - startTime);
          startTime = 0;
        }
        if (reSamplerInited) {

          uint8_t* out_buffer = new uint8_t[outBufferSize];
          int bytes = reSampler.audio_swr_resampling_audio(d_frame,
            (uint8_t**)&out_buffer);

          D_LOG("resample out bytes is {}", bytes);
          if (!tmpFrame) {
            int samples = bytes / out_info.channels /
              av_get_bytes_per_sample(out_info.sample_fmt);
            D_LOG("sample is {}", samples);
            tmpFrame = allocAudioFrame(out_info.sample_fmt, out_info.channel_layout,
              out_info.sample_rate,
              samples);
          }
          memcpy((int8_t*)tmpFrame->data[0], out_buffer, bytes);
          if (out_buffer) delete[] out_buffer;
          tmpFrame->pts = d_frame->pts;
          tmpFrame->best_effort_timestamp = d_frame->best_effort_timestamp;
          av_frame_free(&d_frame);

        }
        else {
          tmpFrame = d_frame;
        }

        std::unique_lock<std::mutex> lock(*(frameList.pkt_list_mu));
        //av_frame_free(&d_frame);
        frameList.frame_list.push_back(tmpFrame);
        D_LOG("add a packet add packet list size is {} ", frameList.frame_list.size());
        d_frame = nullptr;
        tmpFrame = nullptr;
      }
    }
  }
  //如果 buffering状态就不进行解码


}


AVFrame* AuDecoder::getFrame() {
  std::unique_lock<std::mutex> lock(*(frameList.pkt_list_mu));

  AVFrame* frame = nullptr;
  if (!frameList.frame_list.empty()) {

    frame = frameList.frame_list.front();
    if (lastFrame)
      av_frame_free(&lastFrame);
    lastFrame = av_frame_clone(frame);
    frameList.frame_list.pop_front();
    D_LOG("get a frame add packet list size is {} ", frameList.frame_list.size());

  }
  return frame;
}

AVFrame* AuDecoder::getLastFrame() {

  return lastFrame;
}

void AuDecoder::setDemuxType(enum MuxType type) {

  demuxType = type;
}

int AuDecoder::demuxAdts(uint8_t* in, uint8_t* out) {

  adts_header_t* adts = (adts_header_t*)in;
  if (adts->syncword_0_to_8 != 0xff ||
    adts->syncword_9_to_12 != 0xf) {
    W_LOG("demux adts fail, adts header wrong!");
    return 0;
  }

  int aac_frame_size = adts->frame_length_0_to_1 << 11 |
    adts->frame_length_2_to_9 << 3 |
    adts->frame_length_10_to_12;

  memcpy(out, in, aac_frame_size);

  return aac_frame_size;
}

void AuDecoder::demux(uint8_t* in, std::pair<int&, uint8_t*> out) {

  int len = 0;
  switch (demuxType) {
  case MuxType::ADTS:
    out.first = demuxAdts(in, out.second);
    break;
  case MuxType::None:
    break;
  default:
    E_LOG("{} MuxHeaderType is not supported", demuxType);
  }


}

AuDecoder::~AuDecoder() {
  I_LOG("Decoder release...");

  if (decoded_frame)
    av_frame_free(&decoded_frame);
  if (pkt)
    av_packet_free(&pkt);
  if (tmpFrame)
    av_frame_free(&tmpFrame);
  if (dec_ctx)
  {
    avcodec_flush_buffers(dec_ctx);
    avcodec_free_context(&dec_ctx);
  }
  if (parser)
    av_parser_close(parser);

  if (lastFrame)
    av_frame_free(&lastFrame);
  if (jitter_buffer)
    delete[] jitter_buffer;

  std::unique_lock<std::mutex> lock(*(frameList.pkt_list_mu));

  while (!frameList.frame_list.empty()) {
    av_frame_free(&(frameList.frame_list.front()));
    frameList.frame_list.pop_front();
  }
}

/*   重采样工具  */

ReSample::ReSample() {
  D_LOG("Now create a resample ...");
  swr_ctx = swr_alloc();
}

ReSample::ReSample(AudioInfo* out_codec, AudioInfo* in_codec) {
  D_LOG("Now create a resample with in and out info ...");
  audio_swr_init(out_codec, in_codec);
}

ReSample::~ReSample() {
  I_LOG("Now uninit a resample ...");
  if (swr_ctx) {
    swr_free(&swr_ctx);
  }
}

int ReSample::audio_swr_init(AudioInfo* out_codec, AudioInfo* in_codec) {
  D_LOG("Now init a resample ...");

  /* initialize the resampling context */

  swr_ctx = swr_alloc_set_opts(swr_ctx, out_codec->channel_layout,
    out_codec->sample_fmt, out_codec->sample_rate,
    in_codec->channel_layout, in_codec->sample_fmt,
    in_codec->sample_rate, 0, nullptr);

  int ret = 0;

  if ((ret = swr_init(swr_ctx)) < 0) {
    E_LOG("Failed to initialize the resampling context\n");
    if (swr_ctx) {
      swr_free(&swr_ctx);
    }
  }

  out_info.copyFrom(out_codec);
  in_info.copyFrom(in_codec);

  return ret;
}

int ReSample::get_out_size(int nb_samples) {
  int out_size = av_rescale_rnd(nb_samples, out_info.sample_rate,
    in_info.sample_rate, AV_ROUND_UP) *
    out_info.channels *
    av_get_bytes_per_sample(out_info.sample_fmt);

  return out_size;
}

// resample: frame => uint8_t
int ReSample::audio_swr_resampling_audio(AVFrame* audioFrame,
  uint8_t** targetData) {
  int out_size = av_rescale_rnd(
    swr_get_delay(swr_ctx, in_info.sample_rate) + audioFrame->nb_samples,
    out_info.sample_rate, in_info.sample_rate, AV_ROUND_UP);
  if (!swr_ctx) {
    E_LOG("swr not init");
    return -1;
  }
  int len =
    swr_convert(swr_ctx, targetData, out_size,
      (const uint8_t**)&audioFrame->data[0], audioFrame->nb_samples);

  if (len < 0) {
    E_LOG("error swr_convert");
    return -1;
  }
  else {
    int dst_bufsize = len * out_info.channels *
      av_get_bytes_per_sample(out_info.sample_fmt);
    return dst_bufsize;
  }
}

// resample: uint8_t => uint8_t
int ReSample::audio_swr_resampling_audio(uint8_t** srcData,
  uint8_t** targetData, int nb_samples) {
  int out_size = av_rescale_rnd(
    swr_get_delay(swr_ctx, in_info.sample_rate) + nb_samples,
    out_info.sample_rate, in_info.sample_rate, AV_ROUND_UP);

  if (!swr_ctx) {
    E_LOG("swr not init");
    return -1;
  }

  int len = swr_convert(swr_ctx, targetData, out_size,
    (const uint8_t**)srcData, nb_samples);

  if (len < 0) {
    E_LOG("error swr_convert");
    return -1;
  }
  else {
    int dst_bufsize = len * out_info.channels *
      av_get_bytes_per_sample(out_info.sample_fmt);
    return dst_bufsize;
  }
}