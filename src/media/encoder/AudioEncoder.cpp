#include "media/encoder/audioEncoder.h"
#include "other/loggerApi.h"
#include "media/other/common.h"
 //#define pcma_time_seg 30 //每30ms一个rtp包
#define amr_time_seg 20

/*using theia::audioEngine::imp_85::EncoderImpl;*/

EncoderImpl::EncoderImpl() {

}

void EncoderImpl::init(AudioInfo in, AudioInfo out, enum CodecType codecType) {
  in_info = in;
  out_info = out;

  //初始化解码器
  AVCodec* encoder;
  int ret;
  codec_type = codecType;
  if (codecType == CodecType::AAC) {
    encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
    enc_ctx = avcodec_alloc_context3(encoder);
    if (!enc_ctx) {
      E_LOG("Could not alloc an encoding context\n");
      return;
    }
    enc_ctx->frame_size = AAC_PCM_SAMPLE; //aac编码 一帧包含1024 sample
    enc_ctx->codec_type = AVMEDIA_TYPE_AUDIO;
    enc_ctx->sample_rate = out_info.sample_rate;
    enc_ctx->channel_layout = out_info.channel_layout;
    enc_ctx->channels = out_info.channels;
    enc_ctx->sample_fmt = out_info.sample_fmt;
    enc_ctx->codec_tag = 0;
    AVRational ar = { 1, enc_ctx->sample_rate };
    enc_ctx->time_base = ar;

    ret = avcodec_open2(enc_ctx, encoder, nullptr);
    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Cannot open audio encoder for stream \n");
      return;
    }
    //初始化重采样器
    if (!(in == out)) {
      resampler.audio_swr_init(&out_info, &in_info);
      resamplerInited = true;
    }
    D_LOG("encoder resampler is inited {}", resamplerInited);

    //初始化成员变量
    nb_sample = enc_ctx->frame_size;

    tmpFrame = allocAudioFrame(out_info.sample_fmt, out_info.channel_layout, out_info.sample_rate,
      enc_ctx->frame_size);
    muxType = MuxType::ADTS;

    cacheLength = CACHECONTAINER;
    dataCache = new uint8_t[CACHECONTAINER];
  }
  else if (codecType == CodecType::PCMA) {
    I_LOG("PCMA encoder init");
    encoder = avcodec_find_encoder(AV_CODEC_ID_PCM_ALAW);
    enc_ctx = avcodec_alloc_context3(encoder);
    if (!enc_ctx) {
      E_LOG("Could not alloc an encoding context\n");
      return;
    }
    enc_ctx->codec_type = AVMEDIA_TYPE_AUDIO;
    enc_ctx->sample_rate = out_info.sample_rate;
    enc_ctx->channel_layout = out_info.channel_layout;
    enc_ctx->channels = out_info.channels;
    enc_ctx->sample_fmt = out_info.sample_fmt;
    enc_ctx->codec_tag = 0;
    AVRational ar = { 1, enc_ctx->sample_rate };
    enc_ctx->time_base = ar;

    ret = avcodec_open2(enc_ctx, encoder, nullptr);
    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Cannot open audio encoder for stream \n");
      return;
    }
    //                    初始化重采样器
    if (!(in == out)) {
      resampler.audio_swr_init(&out_info, &in_info);
      resamplerInited = true;
    }
    D_LOG("encoder resampler is inited {}", resamplerInited);

    if (enc_ctx->frame_size == 0) {
      enc_ctx->frame_size = enc_ctx->sample_rate * in.pcmaTimeSeg / 1000; //每time_seg要发送的采样点
    }
    //初始化成员变量
    nb_sample = enc_ctx->frame_size;

    tmpFrame = allocAudioFrame(out_info.sample_fmt, out_info.channel_layout, out_info.sample_rate,
      enc_ctx->frame_size);
    muxType = MuxType::None;
    cacheLength = enc_ctx->frame_size * 30;
    dataCache = new uint8_t[cacheLength];
  }
  else if (codecType == CodecType::AMRNB) {
    I_LOG("AMR_NB encoder init");
    encoder = avcodec_find_encoder(AV_CODEC_ID_AMR_NB);
    enc_ctx = avcodec_alloc_context3(encoder);
    if (!enc_ctx) {
      E_LOG("Could not alloc an encoding context\n");
      return;
    }
    enc_ctx->codec_type = AVMEDIA_TYPE_AUDIO;
    enc_ctx->frame_size = 160;
    enc_ctx->sample_rate = out_info.sample_rate;
    enc_ctx->channel_layout = out_info.channel_layout;
    enc_ctx->channels = out_info.channels;
    enc_ctx->sample_fmt = out_info.sample_fmt;
    enc_ctx->bit_rate = out_info.bit_rate == 0 ? 12200 : out_info.bit_rate;
    enc_ctx->codec_tag = 0;
    AVRational ar = { 1, enc_ctx->sample_rate };
    enc_ctx->time_base = ar;
    ret = avcodec_open2(enc_ctx, encoder, nullptr);
    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Cannot open audio encoder for stream \n");
      return;
    }
    //  初始化重采样器
    if (!(in == out)) {
      resampler.audio_swr_init(&out_info, &in_info);
      resamplerInited = true;
    }
    D_LOG("encoder resampler is inited {}", resamplerInited);
    nb_sample = enc_ctx->frame_size;

    tmpFrame = allocAudioFrame(out_info.sample_fmt, out_info.channel_layout, out_info.sample_rate,
      enc_ctx->frame_size);
    muxType = MuxType::None;
    cacheLength = 160 * 30;
    dataCache = new uint8_t[cacheLength];
  }
  else if (codecType == CodecType::AMRWB) {
    I_LOG("AMR-WB encoder init");
    encoder = avcodec_find_encoder(AV_CODEC_ID_AMR_WB);
    enc_ctx = avcodec_alloc_context3(encoder);
    if (!enc_ctx) {
      E_LOG("Could not alloc an encoding context\n");
      return;
    }
    enc_ctx->codec_type = AVMEDIA_TYPE_AUDIO;
    enc_ctx->sample_rate = out_info.sample_rate;
    enc_ctx->frame_size = 320; //amr-wb为320
    enc_ctx->channel_layout = out_info.channel_layout;
    enc_ctx->channels = out_info.channels;
    enc_ctx->sample_fmt = out_info.sample_fmt;
    enc_ctx->bit_rate = out_info.bit_rate == 0 ? 23850 : out_info.bit_rate;
    enc_ctx->codec_tag = 0;
    AVRational ar = { 1, enc_ctx->sample_rate };
    enc_ctx->time_base = ar;

    ret = avcodec_open2(enc_ctx, encoder, nullptr);
    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Cannot open audio encoder for stream \n");
      return;
    }
    //                    初始化重采样器
    if (!(in == out)) {
      resampler.audio_swr_init(&out_info, &in_info);
      resamplerInited = true;
    }
    D_LOG("encoder resampler is inited {}", resamplerInited);
    //初始化成员变量
    nb_sample = enc_ctx->frame_size;

    tmpFrame = allocAudioFrame(out_info.sample_fmt, out_info.channel_layout, out_info.sample_rate,
      enc_ctx->frame_size);
    muxType = MuxType::None;
    cacheLength = 320 * 30;
    dataCache = new uint8_t[cacheLength];
  }
  else {
    E_LOG("{} codec type is not supported ", codecType);
    return;
  }
  finished = false;

}


void EncoderImpl::addFrame(uint8_t* buf, const size_t bufSize, bool endOfStream) {

  static long long start = 0;


  if (finished) {
    W_LOG("Encoder: data stream has been finished.");
  }

  //pcma输出数据的frame size
  int perframesize = enc_ctx->channels * av_get_bytes_per_sample(enc_ctx->sample_fmt);
  //输入数据的样本数
  int sampleNb = bufSize / av_get_bytes_per_sample(in_info.sample_fmt) / in_info.channels;
  int out_size = av_rescale_rnd(sampleNb, out_info.sample_rate, in_info.sample_rate, AV_ROUND_UP) *
    out_info.channels * av_get_bytes_per_sample(out_info.sample_fmt);


  //为重采样数据分配空间
  uint8_t* data = new uint8_t[out_size];

  int len = 0;
  if (resamplerInited) {
    len = resampler.audio_swr_resampling_audio(reinterpret_cast<uint8_t**>(&buf),
      reinterpret_cast<uint8_t**>(&data), sampleNb);
    D_LOG("resample sample number is {}", len / av_get_bytes_per_sample(out_info.sample_fmt) / out_info.channels);
  }
  else {
    memcpy((int8_t*)data, buf, bufSize);
    len = bufSize;
  }

  //缓存待处理数据
  uint8_t* preProcessData = new uint8_t[cacheLength];

  //先将未处理完的数据拿来
  memcpy(preProcessData, dataCache, dataCacheIndex);
  //将新数据写入
  memcpy(preProcessData + dataCacheIndex, data, len);
  //                I_LOG("new data len：{}",len);

  if (data)
    delete[] data;

  //preProcessData已经处理的数据索引
  int preProcessDataIndex = 0;

  //总数据长度
  int totalLen = dataCacheIndex + len;
  //    I_LOG("total len is {}",totalLen);

  //每次进行编码的数据长度
  int eachPcmLen = 0;
  //                I_LOG("frame_size {}",enc_ctx->frame_size);
  if (codec_type == CodecType::AAC) {
    eachPcmLen = AAC_PCM_SAMPLE * out_info.channels * av_get_bytes_per_sample(out_info.sample_fmt); //每次处理的数据长度
  }
  else {
    eachPcmLen = enc_ctx->frame_size * out_info.channels * av_get_bytes_per_sample(out_info.sample_fmt);
    //                    eachPcmLen = enc_ctx->frame_size * out_info.channels;
  }

  AVPacket* tmpPkt = nullptr;

  while (totalLen - preProcessDataIndex >= eachPcmLen) {

    // fill data into frame.
    memcpy((int8_t*)tmpFrame->data[0], preProcessData + preProcessDataIndex, eachPcmLen);
    if (codec_type == CodecType::PCMA || codec_type == CodecType::AMRNB || codec_type == CodecType::AMRWB) {
      tmpFrame->nb_samples = eachPcmLen / perframesize;
    }
    int ret;
    if (start == 0)
      start = GetTickCount64();

    ret = avcodec_send_frame(enc_ctx, tmpFrame);
    D_LOG("send frame ");
    if (ret == AVERROR(EAGAIN)) {
      W_LOG("Encoder: input is not accepted in the current state");
    }
    else if (ret == AVERROR_EOF) {
      W_LOG(
        "the encoder has been flushed, and no new frames can be sent to it");
    }
    else if (ret == AVERROR(EINVAL)) {
      I_LOG("tmpFrame->format: {}, tmpFrame->channels: {}, tmpFrame->channel_layout: {}, tmpFrame->sample_rate: {}, tmpFrame->nb_samples: {}", tmpFrame->format, tmpFrame->channels, tmpFrame->channel_layout, tmpFrame->sample_rate, tmpFrame->nb_samples);
      //throw std::runtime_error(
      //    "Encoder: codec not opened, refcounted_frames not set, it is a decoder, or "
      //    "requires flush");
      E_LOG(
        "Encoder: codec not opened, refcounted_frames not "
        "set, it is a decoder, or "
        "requires flush");
    }
    else if (ret == AVERROR(ENOMEM)) {
      W_LOG("failed to add packet to internal queue, or similar");
    }
    else if (ret < 0) {
      W_LOG("Encoder: legitimate decoding errors: ");
    }

    if (endOfStream) {
      avcodec_send_frame(enc_ctx, nullptr);
      finished = true;
    }

    tmpPkt = av_packet_alloc();

    if (tmpPkt == nullptr) {
      E_LOG("av_packet_alloc error");
      if (preProcessData)
        delete[] preProcessData;
      return;
    }
    ret = avcodec_receive_packet(enc_ctx, tmpPkt);
    if (ret != 0) {
      av_packet_free(&tmpPkt);
      I_LOG("av_packet free...");
    }

    if (ret == 0) {
      D_LOG("encode frame cost time is {}", seeker::Time::microTime() - start);
      start = 0;
      std::unique_lock <std::mutex> lock(*(pktList.pkt_list_mu));
      //                        I_LOG("tmppktsiez{}",tmpPkt->size);
      pktList.pkt_list.push_back(tmpPkt);
      D_LOG("add frame input pkt ,pkt size is {} ", pktList.pkt_list.size());
      tmpPkt = nullptr;
    }
    else if (ret == AVERROR(EAGAIN)) {
      I_LOG("add frame output is not available in the current state, user must try to send input");
      if (tmpPkt) {
        av_packet_free(&tmpPkt);
      }
      if (preProcessData)
        delete[] preProcessData;
      return;
    }
    else if (ret == AVERROR_EOF) {
      W_LOG("the encoder has been fully flushed, and there will be no more output packet");
      if (tmpPkt) {
        av_packet_free(&tmpPkt);
      }
      if (preProcessData)
        delete[] preProcessData;
      return;
    }
    else if (ret == AVERROR(EINVAL)) {
      W_LOG("Encoder: codec not opened, or it is an encoder");
    }
    else {
      W_LOG(
        fmt::format("Encoder: legitimate decoding errors: {}", ret));
    }

    preProcessDataIndex += eachPcmLen;

  }
  if (tmpPkt) {
    av_packet_free(&tmpPkt);
  }

  //将data中剩余数据写回缓存
  memcpy(dataCache, preProcessData + preProcessDataIndex, totalLen - preProcessDataIndex);

  //设置缓存索引
  dataCacheIndex = totalLen - preProcessDataIndex;

  if (preProcessData)
    delete[] preProcessData;


};


AVPacket* EncoderImpl::getPacket() {

  AVPacket* tmpPkt = nullptr;
  AVPacket* rstPkt = nullptr;
  std::unique_lock<std::mutex> lock(*(pktList.pkt_list_mu));
  if (pktList.pkt_list.empty()) {

    tmpPkt = av_packet_alloc();

    int ret = avcodec_receive_packet(enc_ctx, tmpPkt);
    if (ret != 0) {
      av_packet_free(&tmpPkt);
    }
    if (ret == 0) {
      rstPkt = tmpPkt;
      tmpPkt = nullptr;
    }
    else if (ret == AVERROR(EAGAIN)) {
      //I_LOG("get packer output is not available in the current state, user must try to send input");
      rstPkt = nullptr;
    }
    else if (ret == AVERROR_EOF) {
      W_LOG("the encoder has been fully flushed, and there will be no more output packets");
      rstPkt = nullptr;
    }
    else if (ret == AVERROR(EINVAL)) {
      W_LOG("AacAdtsEncoder: codec not opened, or it is an encoder");
    }
    else {
      W_LOG("AacAdtsEncoder: legitimate decoding errors");
    }
  }
  else {
    D_LOG("pktList size is {}", pktList.pkt_list.size());
    rstPkt = pktList.pkt_list.front();
    pktList.pkt_list.pop_front();
    D_LOG("after pop pktList size is {}", pktList.pkt_list.size());

  }

  return rstPkt;
}


void EncoderImpl::setMuxType(enum MuxType type) {

  muxType = type;

}

int EncoderImpl::mux(uint8_t* in, int in_len, uint8_t* out) {

  uint8_t* adtsHeader = new uint8_t[7];

  if (muxType == MuxType::ADTS) {
    writeAdtsHeaders(adtsHeader, in_len);
    memcpy(out, adtsHeader, ADTS_HEADER_LENGTH);
    memcpy(out + ADTS_HEADER_LENGTH, in, in_len);
    if (adtsHeader)
      delete[] adtsHeader;
    return in_len + 7;
  }
  else if (muxType == MuxType::None) {
    memcpy(out, in, in_len);
    return in_len;
  }
  else {
    E_LOG("{} muxType is not supported", muxType);
    return 0;
  }
}

int EncoderImpl::writeAdtsHeaders(uint8_t* header, int dataLength) {

  uint8_t profile = 0x02;  // AAC LC
  uint8_t channelCfg = out_info.channels;
  uint32_t packetLength = dataLength + 7;
  uint8_t freqIdx;  // 22.05 KHz

  switch (out_info.sample_rate) {
  case 96000:
    freqIdx = 0x00;
    break;
  case 88200:
    freqIdx = 0x01;
    break;
  case 64000:
    freqIdx = 0x02;
    break;
  case 48000:
    freqIdx = 0x03;
    break;
  case 44100:
    freqIdx = 0x04;
    break;
  case 32000:
    freqIdx = 0x05;
    break;
  case 24000:
    freqIdx = 0x06;
    break;
  case 22050:
    freqIdx = 0x07;
    break;
  case 16000:
    freqIdx = 0x08;
    break;
  case 12000:
    freqIdx = 0x09;
    break;
  case 11025:
    freqIdx = 0x0A;
    break;
  case 8000:
    freqIdx = 0x0B;
    break;
  case 7350:
    freqIdx = 0x0C;
    break;
  default:
    W_LOG("addADTStoPacket: unsupported sampleRate: " +
      std::to_string(out_info.sample_rate));
  }

  //	T_LOG("profile={} channelCfg={} packetLength={} freqIdx={}", (int)profile, (int)channelCfg, (int)packetLength, (int)freqIdx);

  // fill in ADTS data
  header[0] = (uint8_t)0xFF;
  header[1] = (uint8_t)0xF1;

  header[2] = (uint8_t)(((profile - 1) << 6) + (freqIdx << 2) + (channelCfg >> 2));
  header[3] = (uint8_t)(((channelCfg & 3) << 6) + (packetLength >> 11));
  header[4] = (uint8_t)((packetLength & 0x07FF) >> 3);
  header[5] = (uint8_t)(((packetLength & 0x0007) << 5) + 0x1F);
  header[6] = (uint8_t)0xFC;

  return ADTS_HEADER_LENGTH;
}

int EncoderImpl::copyParams(AVCodecParameters* codecpar) {
  int ret = avcodec_parameters_from_context(codecpar, enc_ctx);
  if (ret < 0) {
    printf("audio stream copy params error.\n");
    return ret;
  }
  return ret;
};

EncoderImpl::~EncoderImpl() {
  avcodec_free_context(&enc_ctx);
  if (tmpFrame) {
    av_frame_free(&tmpFrame);
  }

  if (dataCache)
    delete[] dataCache;

  std::unique_lock<std::mutex> lock(*pktList.pkt_list_mu);

  while (!pktList.pkt_list.empty()) {
    av_packet_free(&(pktList.pkt_list.front()));
    pktList.pkt_list.pop_front();
  }
};

