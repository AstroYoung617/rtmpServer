
#include <iostream>
#include <media/VideoSender.h>
long long last2 = 0;

uint8_t* AVFrame2Img(AVFrame* pFrame) {
  int frameHeight = pFrame->height;
  int frameWidth = pFrame->width;
  int channels = 3;



  //创建保存yuv数据的buffer
  uint8_t* pDecodedBuffer = (uint8_t*)malloc(
    frameHeight * frameWidth * sizeof(uint8_t) * channels);

  //从AVFrame中获取yuv420p数据，并保存到buffer
  int i, j, k;
  //拷贝y分量
  for (i = 0; i < frameHeight; i++) {
    memcpy(pDecodedBuffer + frameWidth * i,
      pFrame->data[0] + pFrame->linesize[0] * i,
      frameWidth);
  }
  //拷贝u分量
  for (j = 0; j < frameHeight / 2; j++) {
    memcpy(pDecodedBuffer + frameWidth * i + frameWidth / 2 * j,
      pFrame->data[1] + pFrame->linesize[1] * j,
      frameWidth / 2);
  }
  //拷贝v分量
  for (k = 0; k < frameHeight / 2; k++) {
    memcpy(pDecodedBuffer + frameWidth * i + frameWidth / 2 * j + frameWidth / 2 * k,
      pFrame->data[2] + pFrame->linesize[2] * k,
      frameWidth / 2);
  }
  return pDecodedBuffer;
}

VideoSender::VideoSender(std::mutex* _mutex, std::condition_variable* _vdcv, std::shared_ptr<NetManager> _netManager) {
  mutex4Encoder = std::make_unique<std::mutex>();
  netManager = _netManager;
};

VideoSender::~VideoSender() {
  I_LOG("VideoSender destruct ...");
  //stop();
}

void VideoSender::initEncoder(const VideoDefinition& captureSize, int frameRate) {
  std::unique_lock<std::mutex> encoderLk(*mutex4Encoder);
  if (encoder) {
    encoder->stop();
    delete encoder;
    encoder = nullptr;
    I_LOG("re new an encoder... ");
    encoder = new Encoder();
  }
  else {
    I_LOG("new an encoder...");
    encoder = new Encoder();
  }
  encoder->setIsRTMP(true);
  encoder->setCodecFmt("h264");
  encoder->setIsCrf(false);
  encoder->setProfile("high");
  encoder->setMaxBFrame(0);
  encoder->setGopSize(20);
  encoder->setPixFmt(AV_PIX_FMT_BGRA);
  this->frameRate = frameRate;
  encoder->setFrameRate(frameRate);
  VideoDefinition encoderSize = VideoSenderUtil::calEncoderSize(captureSize, maxDefinition);
  encoder->setFrameSize(encoderSize.width, encoderSize.height);
  int bitRate = VideoSenderUtil::calBitRate(encoderSize);
  encoder->setBitrate(bitRate);
  encoder->init();
  encoderLk.unlock();
  I_LOG("init encoder finished: size={}, bitRate={}", encoderSize.getString(), bitRate);

  //netManager->newVideoStream(encoder->getEncoderCtx());

  encoder->copyParams(netManager->getVideoStream()->codecpar);
}

int VideoSender::stop() {
  if (encoder) {
    std::unique_lock<std::mutex> encoderLk(*mutex4Encoder);
    encoder->stop();
    encoderLk.unlock();
  }
  return 0;
}


int VideoSender::sendFrame(AVFrame* frame) {
  std::unique_lock<std::mutex> encoderLk(*mutex4Encoder);
  auto img = AVFrame2Img(frame);
  encoder->pushWatchFmt(img, -1, frame->width, frame->height,
    static_cast<AVPixelFormat>(frame->format));
  AVPacket* packet = encoder->pollAVPacket();
  if (packet) {
    packet->duration = ceil(1000 / frameRate);
    count++;
    I_LOG("video packet index {} pts {} dts {} dura {}", count, packet->pts, packet->dts, packet->duration);
    packet->stream_index = 0;
    if (netManager) {
      //long long now = GetTickCount64();
      //if (packet->pts > now)
      //{
      //  std::this_thread::sleep_for(std::chrono::microseconds((packet->pts - now) * 1000));
      //}
      int ret = netManager->sendRTMPData(packet);
      encoderLk.unlock();
      //fix memory leak, 这种内存泄漏的地方尤其要注意，是因为使用的函数直接返回数组，那么就应该在不再使用的时候进行释放。
      delete img;
      return ret;
    }
  }
  encoderLk.unlock();
  delete img;
  av_frame_free(&frame);
  return -1;
}

void VideoSender::setStartTime(long long _startTime) {
  startTime = _startTime;
}
