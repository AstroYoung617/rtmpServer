//
// Created by hopeworld on 2021/11/30.
//
#include <media/AudioSender.h>
#include <libavutil/time.h>
long long last = 0;

void av_packet_rescale_ts(AVPacket* pkt, AVRational src_tb, AVRational dst_tb) {
  if (pkt->pts != AV_NOPTS_VALUE)
    pkt->pts = av_rescale_q(pkt->pts, src_tb, dst_tb);
  if (pkt->dts != AV_NOPTS_VALUE)
    pkt->dts = av_rescale_q(pkt->dts, src_tb, dst_tb);
  if (pkt->duration > 0)
    pkt->duration = av_rescale_q(pkt->duration, src_tb, dst_tb);
}

AudioSender::AudioSender(std::shared_ptr<NetManager> _netManager) {
  netManager = _netManager;
  I_LOG("AudioSender construct ...");
};

AudioSender::~AudioSender() {
  I_LOG("AudioSender destruct ...");
  stop();
}

void AudioSender::initAudioEncoder(CoderInfo encoderInfo) {
  I_LOG("initAudioEncoder");

  AudioInfo in;
  in.sample_rate = encoderInfo.inSampleRate;
  in.channels = encoderInfo.inChannels;
  in.sample_fmt = (AVSampleFormat)(encoderInfo.inFormate);
  in.channel_layout = av_get_default_channel_layout(encoderInfo.inChannels);
  if (encoderInfo.cdtype == CodecType::PCMA) {
    in.pcmaTimeSeg = 30;
  }
  AudioInfo out;
  out.sample_rate = encoderInfo.outSampleRate;
  out.channels = encoderInfo.outChannels;
  out.sample_fmt = (AVSampleFormat)(encoderInfo.outFormate);
  out.channel_layout = av_get_default_channel_layout(encoderInfo.outChannels);

  encoderCodecType = encoderInfo.cdtype;
  encoder = std::make_unique<EncoderImpl>();
  encoder->init(in, out, encoderInfo.cdtype);
  encoder->setMuxType(encoderInfo.muxType);

  //netManager->newAudioStream(encoder->getEncoderCtx());
  this->in = in;
  this->out = out;

  encoder->copyParams(netManager->getAudioStream()->codecpar);
}

int AudioSender::stop() {
  return 0;
}

int AudioSender::send(uint8_t* buf, int len) {
  encoder->addFrame(buf, len, false);
  AVPacket* packet = encoder->getPacket();
  if (packet) {
    if (netManager) {
      //已经过去的时间
      //long long now = 0;
      int duration = ceil(1024 * 1000 / out.sample_rate);
      packet->pts = count*duration;    
      packet->dts = packet->pts;
      I_LOG("audio packet index {} pts {} dts {} dura {}", count, packet->pts, packet->dts, duration);
      packet->duration = packet->pts - lastPts;
      lastPts = packet->pts;
      if (count > 0) {
        //I_LOG("audio packet index {} pts {} dura {} frameRate {} expectPts {} deltaPts {}",count,packet->pts,packet->duration,count*1000/packet->pts,count*duration,count*duration-packet->pts);
      }
      count++;
      packet->stream_index = 1;
      //if (packet->pts > now)
      //{
      //    I_LOG("delay");
      //  //usleep((packet->pts - now) * 1000);
      //  std::this_thread::sleep_for(std::chrono::microseconds((packet->pts - now) * 1000));
      //}
      int ret = netManager->sendRTMPData(packet);
      if (ret < 0) {
        char buf[1024] = { 0 };
        av_strerror(ret, buf, sizeof(buf));
        std::cout << "audio: " << buf << std::endl;
      }
      return ret;
    }
  }
  return -1;
}

void AudioSender::setStartTime(long long _startTime) {
  startTime = _startTime;
}