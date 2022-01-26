
#include "media/encoder/videoEncoder.h"
#include "media/common.h"
#include "other/loggerApi.h"
#include "media/Parser.h"

#define ENPARSER 0
#define LIMITINIT 0
#define ENCODER_LOCKER 1

#if _WIN32
#define ALIGN 1
#else
#define ALIGN 1
#endif


Encoder::Encoder() {
  avcodec_register_all();
  av_register_all();
}

Encoder::~Encoder() {
  I_LOG("h264 encoder is released");
  //Encoder::stop();
}
void Encoder::init() {
#if LIMITINIT
  if (encodeReady) {
    E_LOG("encoder is inited, please init after stop");
    return;
  }
#endif
  if (!encodeParser) {
    encodeParser = std::make_shared<Parser>();
  }

  //TODO 先将编码器里的内容全部拿出来
  if (encodeCtx) {
    //注意释放该内存是否会对整体有影响
    avcodec_free_context(&encodeCtx);
    encodeCtx = nullptr;
  }
  encode = avcodec_find_encoder(AV_CODEC_ID_H264);
  //encode = avcodec_find_encoder(AV_CODEC_ID_VP8);
  if (!encode) {
    auto msg = fmt::format("find encode error");
    throw std::runtime_error(msg);
  }

  encodeCtx = avcodec_alloc_context3(encode);
  if (!encodeCtx) {
    throw std::runtime_error("encodeCtx create error");
  }

  // 最开始的默认值，不然不设置enoderSize的话会出错
  encodeCtx->width = outputWidth < 0 ? 256 : outputWidth;
  encodeCtx->height = outputHeight < 0 ? 144 : outputHeight;
  encodeCtx->color_range = AVCOL_RANGE_MPEG;
  encodeCtx->pix_fmt = AV_PIX_FMT_YUV420P;
  encodeCtx->time_base = { 1, framerate };
  encodeCtx->framerate = { framerate , 1 };
  encodeCtx->bit_rate = bitRate;
  encodeCtx->max_b_frames = maxBframe;

  encodeCtx->codec_id = encode->id;
  encodeCtx->codec_type = AVMEDIA_TYPE_VIDEO;
  encodeCtx->gop_size = gopsize;
  encodeCtx->profile = Profile;

  if (flv_flag)
  {
    encodeCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  AVDictionary* dic = nullptr;
  av_dict_set(&dic, "preset", Preset.c_str(), 0);
  av_dict_set(&dic, "tune", tune.c_str(), 0);

  if (isCrf) {
    av_dict_set(&dic, "crf", Crf.c_str(), 0);
    I_LOG("use crf: {}", Crf.c_str());
  }
  else {
    I_LOG("no crf");
  }

  //av_dict_set(&dic, "profile", Profile.c_str(), 0);

  if (avcodec_open2(encodeCtx, encode, &dic) < 0) {
    throw std::runtime_error("open encodec ctx error");
  }
  I_LOG("init h264 encode success");
  initSws();
  initOutput();
}

void Encoder::push(uint8_t* data, int len, int width, int height) {
  pushWatchFmt(data, len, width, height, pixFmt);
}

void Encoder::pushWatchFmt(uint8_t* data, int len, int width, int height, AVPixelFormat fmt) {
#if ENCODER_LOCKER
  std::unique_lock<std::mutex> locker(sendMtx);
#endif
  if (!encodeReady) {
    E_LOG("encode not init");
    return;
  }
  //如果输入的图片像素格式或图片大小和编码器不一致，则需要对图片进行格式转换
  bool needUpdate = false;
  if (width != inputWidth || height != inputHeight || fmt != pixFmt) {
    inputWidth = width;
    inputHeight = height;
    pixFmt = fmt;
    if (updateOutSize()) {
      initSws();
      initOutput();
    }
    else {
      init();
    }
  }
  int ret = 0;

  // 设置输入frame_
  //I_LOG("encode not directly.");
  AVFrame* frame_ = av_frame_alloc();
  frame_->height = outputHeight;
  frame_->width = outputWidth;
  frame_->format = AV_PIX_FMT_YUV420P;
  count++;
  frame_->pts = count * (encodeCtx->time_base.num * 1000 / encodeCtx->time_base.den);
  frame_->pkt_dts = frame_->pts; //去掉ffmpeg警告
  ret = bufferToAVFrame(data, frame_, width, height);
  if (ret < 0) {
    E_LOG("can not convert data to yuvFrame");
  }

  ret = avcodec_send_frame(encodeCtx, frame_);
  if (ret < 0) {
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      W_LOG("send frame error {}", ret);//
    }
    else {
      E_LOG("send frame error {}", ret);
    }
  }
  if (frame_) {
    av_frame_free(&frame_);
  }


  AVPacket* pkt = av_packet_alloc();
  av_init_packet(pkt);
  pkt->data = NULL;
  pkt->size = 0;
  ret = avcodec_receive_packet(encodeCtx, pkt);
  if (ret < 0) {
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      W_LOG("recv pkt error");
    }
    else {
      E_LOG("recv pkt error");
    }
    av_packet_free(&pkt);
  }
  else {
    if (isSaveToFile && encodeReady) {
      pkt->stream_index = video_st->index;
      ret = av_interleaved_write_frame(pFormatCtx, pkt);
      if (ret) {
        E_LOG("write fail:{}", ret);
      }
      else {
        //I_LOG("write success");
      }
    }

    /*   if (sendList.size() > 150) {
           E_LOG("encode push data failed");
       }
       else {*/
    if (pkt != nullptr) {
      if (pkt->size)
      {
        if (pkt->size > 0)
        {
          sendList.push_back(pkt);
        }
        else
        {
          E_LOG("pkt size: {}", pkt->size);
        }
      }
      else
      {
        E_LOG("pkt size is null");
      }
    }
    else {
      E_LOG("encode pkt size == 0");
    }
    //}
    //sendCv.notify_one();
  }

}



int Encoder::poll(std::vector<std::pair<uint8_t*, int>>& outData) {
  std::unique_lock<std::mutex> locker(sendMtx);
  //sendCv.wait(locker, [&] {return !sendList.empty(); });
  if (sendList.empty()) {
    return -1;
  }
  AVPacket* pkt = sendList.front();
  sendList.pop_front();

#if ENPARSER
  uint8_t* buf = (uint8_t*)malloc(sizeof(uint8_t) * pkt->size);
  memcpy(buf, pkt->data, pkt->size);
  outData.push_back(std::make_pair(buf, pkt->size));
  av_packet_free(&pkt);
  return 0;
#else
  //封装
  if (pkt != nullptr) {
    encodeParser->H264Parse2NALU(pkt, outData);
    av_packet_free(&pkt);
  }
  else {
    E_LOG("encode pkt is nullptr");
    return -2;
  }

  return 0;
#endif
}


AVPacket* Encoder::pollAVPacket()
{
  std::unique_lock<std::mutex> locker(sendMtx);
  if (sendList.empty()) {
    return nullptr;
  }
  AVPacket* pkt = sendList.front();
  sendList.pop_front();

  if (pkt != nullptr) {

    return pkt;

  }
  else {
    E_LOG("encode pkt is nullptr");
    return nullptr;
  }

}

int Encoder::copyParams(AVCodecParameters* codecpar)
{
  if (codecpar && encodeCtx)
  {
    int ret = avcodec_parameters_from_context(codecpar, encodeCtx);
    if (ret < 0) {
      E_LOG("stream copy params error");
    }
    return ret;
  }
  else
  {
    E_LOG("codercapr or encoderCtx is nullptr");
    return -1;
  }

}




void Encoder::stop() {
#if ENCODER_LOCKER
  std::unique_lock<std::mutex> locker(sendMtx);
#endif
  if (encodeCtx) {
    avcodec_free_context(&encodeCtx);
    encodeCtx = nullptr;
  }
  if (swsCtx) {
    sws_freeContext(swsCtx);
    swsCtx = nullptr;
  }
  if (outBuff) {
    av_free(outBuff);
    outBuff = nullptr;
  }
  for (auto data : sendList) {
    av_packet_free(&data);
  }
  sendList.clear();
  encodeParser = nullptr;
  encodeReady = false;
}

void Encoder::setCodecFmt(std::string fmt) {
  if (fmt == "h264") {
    codecFmt = AV_CODEC_ID_H264;
  }
  else {
    codecFmt = AV_CODEC_ID_VP8;
  }
}

void Encoder::setPixFmt(AVPixelFormat fmt) {
  pixFmt = fmt;
}

void Encoder::setBitrate(int bitrate) {
  bitRate = bitrate;
}

void Encoder::setFrameRate(int frameRate) {
  framerate = frameRate;
}

void Encoder::setFrameSize(int outWidth, int outHeight) {
  outputHeight = outHeight;
  outputWidth = outWidth;
  isScale = outHeight < 0 ? true : false;
}

void Encoder::setGopSize(int gopSize) {
  gopsize = gopSize;
}

void Encoder::setMaxBFrame(int maxBFrame) {
  maxBframe = maxBFrame;
}

void Encoder::setPreset(std::string preset) {
  Preset = preset;
}

void Encoder::setTune(std::string Tune) {
  tune = Tune;

}

void Encoder::setCrf(std::string crf) {
  if (isCrf) {
    Crf = crf;
    I_LOG("set crf {}", crf);
  }
  else {
    I_LOG("set crf failed");
  }
}

void Encoder::setProfile(std::string profile) {
  if (profile == "baseline") {
    Profile = FF_PROFILE_H264_BASELINE;
  }
  else if (profile == "main") {
    Profile = FF_PROFILE_H264_MAIN;
  }
  else if (profile == "high") {
    Profile = FF_PROFILE_H264_HIGH;
  }
  else if (profile == "high_444") {
    Profile = FF_PROFILE_H264_HIGH_444;
  }
  else if (profile == "high_444_predictive") {
    Profile = FF_PROFILE_H264_HIGH_444_PREDICTIVE;
  }
  else {
    Profile = FF_PROFILE_H264_BASELINE;
  }
}

void Encoder::setIsCrf(bool setIsCrf) {
  setIsCrf ? isCrf = true : isCrf = false;
}

//private
void Encoder::initSws()
{
  int inWid = inputWidth < 0 ? 256 : inputWidth;
  int inHei = inputHeight < 0 ? 144 : inputHeight;
  int outWid = outputWidth < 0 ? 256 : outputWidth;
  int outHei = outputHeight < 0 ? 144 : outputHeight;
  swsCtx = sws_getCachedContext(swsCtx, inWid, inHei, pixFmt, outWid, outHei, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
  if (outBuff) {
    av_free(outBuff);
    outBuff = nullptr;
  }
  outBuff = (uint8_t*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, outWid, outHei, ALIGN) * sizeof(uint8_t));
}

int Encoder::bufferToAVFrame(uint8_t* data, AVFrame* yuvFrame, size_t width, size_t height) {
  AVFrame* frame = av_frame_alloc();
  int ret = av_image_fill_arrays(frame->data, frame->linesize, data, pixFmt, width, height, ALIGN);
  if (ret < 0) {
    auto msg = fmt::format("fill array error, {}", ret);
    throw std::runtime_error(msg);
  }

  av_frame_get_buffer(yuvFrame, ALIGN);
  int outSliceH =
    sws_scale(swsCtx, frame->data, frame->linesize, 0,
      height, yuvFrame->data, yuvFrame->linesize);
  if (outSliceH < 0) {
    av_frame_unref(frame);
    av_frame_free(&frame);
    E_LOG("can not convert data to yuvFrame");
    return -1;
  }
  av_frame_free(&frame);

  return 0;

}

int Encoder::updateOutSize() {
  //没有设置宽高，按照input大小编码
  if (outputWidth < 0 && outputHeight < 0) {
    outputWidth = inputWidth;
    outputHeight = inputHeight;
  }
  //仅没有设置宽，按照outputwidth等比例缩放
  else if (outputHeight < 0) {
    outputHeight = outputWidth * inputHeight / inputWidth / 2 * 2;
  }
  //仅没有设置高，按照outputheight等比例缩放
  else if (outputWidth < 0) {
    outputWidth = outputHeight * inputWidth / inputHeight / 2 * 2;
  }
  else if (isScale && outputWidth > 0) {
    outputHeight = outputWidth * inputHeight / inputWidth / 2 * 2;
  }
  else {
    // 编码宽度直接按照设置进行，不进行任何操作
    return 1;
  }
  return 0;
}


bool Encoder::needRescale()
{
  if (inputWidth <= MAX_ENCODE_WIDTH && inputHeight <= MAX_ENCODE_HEIGHT) {
    return false;
  }
  else {
    updateOutSize();
    return true;
  }
}



void Encoder::initOutput() {
  if (isSaveToFile && encodeCtx != nullptr) {
    I_LOG("initOutput");
    if (pFormatCtx != nullptr) {
      deleteOutput();
    }
    setOutput();
  }
  encodeReady = true;
}

void Encoder::deleteOutput() {
  I_LOG("delete output");
  av_write_trailer(pFormatCtx);
  if (video_st) {
    avcodec_close(video_st->codec);
  }

  avio_close(pFormatCtx->pb);
  avformat_free_context(pFormatCtx);
  video_st = nullptr;
  pFormatCtx = nullptr;

}
void Encoder::setOutput() {

  pFormatCtx = avformat_alloc_context();
  I_LOG("output:{}", out_file_path.c_str());
  //设置输出文件
  int ret = avformat_alloc_output_context2(&pFormatCtx, NULL, nullptr, out_file_path.c_str());
  if (ret) {
    E_LOG("open output fail:{}", ret);
  }
  fmt = pFormatCtx->oformat;


  //新建一个输出流
  if (video_st == nullptr) {
    video_st = avformat_new_stream(pFormatCtx, nullptr);
    if (video_st == NULL) {
      E_LOG("failed allocating output stram\n");
    }
  }

  //copy 编码器的参数到输出流中
  ret = avcodec_parameters_from_context(video_st->codecpar, encodeCtx);
  if (ret < 0) {
    printf("v copy params error.\n");
    //return ret;
  }
  //打开输出文件
  if (avio_open(&pFormatCtx->pb, out_file_path.c_str(), AVIO_FLAG_READ_WRITE)) {
    E_LOG("output file open fail!");
    //	return -1;
  }

  //写文件头
  ret = avformat_write_header(pFormatCtx, nullptr);
  if (ret) {
    E_LOG("write header error:{}", ret);
  }


  av_dump_format(pFormatCtx, 0, out_file_path.c_str(), 1);

}

void Encoder::setIsRTMP(bool isRtmp)
{
  flv_flag = isRtmp;
}