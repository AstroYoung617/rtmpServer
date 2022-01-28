
#include <iostream>
#include <media/VideoSender.h>


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
  encoder->setGopSize(30);
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

int VideoSender::sendPacket(AVPacket* packet) {
  std::unique_lock<std::mutex> encoderLk(*mutex4Encoder);
  //AVPacket* packet = _packet;
  if (packet) {
    packet->duration = ceil(1000 / frameRate);
    count++;
    packet->pts = count * packet->duration;
    packet->dts = packet->pts;
    I_LOG("video packet index {} pts {} dts {} dura {}", count, packet->pts, packet->dts, packet->duration);
    packet->stream_index = 0;
    if (netManager) {
      int ret = netManager->sendRTMPData(packet);
      encoderLk.unlock();
      //fix memory leak, 这种内存泄漏的地方尤其要注意，是因为使用的函数直接返回数组，那么就应该在不再使用的时候进行释放。
      return ret;
    }
  }
  encoderLk.unlock();
  av_packet_free(&packet);
  return -1;
}

void VideoSender::setStartTime(long long _startTime) {
  startTime = _startTime;
}
