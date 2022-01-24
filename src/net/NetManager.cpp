/*
* Created by Astro 2022.1.24
* Descryption: 用于创建不同的socket（根据音视频类型和端口号，创建socket并且进行接收数据回调给AudioReceiver，目前先在receiver中进行处理，后面再将其移植到该文件中
*/

//#ifdef _WIN32
//#include "stdafx.h"
//#endif

#include "net/NetManager.h"

NetManager::NetManager() {
  I_LOG("NetManager construct ...");

}

int NetManager::createChannel(int mediaType, ON_CREATE_CHANNEL_FINISH createChannelCallback) {  // 0-音频 1-主流 2-辅流
  auto connection = std::make_shared<SFUConnection_imp_88>();
  // ice finish callback func
  auto connectionCallback = [&](theia::NetIo::SKPeerConnectionState state) {
    if (state == theia::NetIo::SKPeerConnectionState::FAILED) {
      if (createChannelCallback) {
        createChannelCallback(CreateChannelState::FAILED);
      }
    }
    else {
      if (createChannelCallback) {
        createChannelCallback(CreateChannelState::SUCCESS);
      }
      if (mediaType == 0) {
        audioConnection = connection;
      }
      else if (mediaType == 1) {
        videoConnection = connection;
      }
      else if (mediaType == 2) {
        secondVideoConnection = connection;
      }
      else {
        W_LOG("create channel: mediaType-{} error", mediaType);
      }
    }
  };

  // start to connect
  SelfInfo& selfInfo = SelfInfo::getInstance();
  I_LOG("userName = {}", selfInfo.getNickName());
  I_LOG("password = {}", selfInfo.getPassword());
  switch (mediaType) {
  case 0: {
    connection->setUserName(selfInfo.getAudioChannelInfo().userName);
    connection->setPassword(selfInfo.getAudioChannelInfo().password);
    connection->setRealm(selfInfo.getAudioChannelInfo().realm);
    connection->setSsrc(selfInfo.getAudioChannelInfo().ssrc);
    E_LOG("ssrc = {}", selfInfo.getAudioChannelInfo().ssrc);
    auto receiveCallback = [&](uint8_t* data, size_t len, int64_t ts, uint16_t seqNum,
      std::vector<uint32_t> ssrcVec, int payloadType) {
        this->onReceiveRtpData(0, data, len, ts, seqNum, ssrcVec, payloadType);
    };
    connection->setRTPDataRecvCallback(receiveCallback);
    connection->startGatherAndConnect(
      selfInfo.getAudioChannelInfo().sfuIp + ":" +
      std::to_string(selfInfo.getAudioChannelInfo().sfuPort),
      connectionCallback);
    return 0;
  }
  case 1: {
    connection->setUserName(selfInfo.getVideoChannelInfo().userName);
    connection->setPassword(selfInfo.getVideoChannelInfo().password);
    connection->setRealm(selfInfo.getVideoChannelInfo().realm);
    connection->setSsrc(selfInfo.getVideoChannelInfo().ssrc);
    E_LOG("ssrc = {}", selfInfo.getVideoChannelInfo().ssrc);
    auto receiveCallback = [&](uint8_t* data, size_t len, int64_t ts, uint16_t seqNum,
      std::vector<uint32_t> ssrcVec, int payloadType) {
        this->onReceiveRtpData(1, data, len, ts, seqNum, ssrcVec, payloadType);
    };
    connection->setRTPDataRecvCallback(receiveCallback);
    connection->startGatherAndConnect(
      selfInfo.getVideoChannelInfo().sfuIp + ":" +
      std::to_string(selfInfo.getVideoChannelInfo().sfuPort),
      connectionCallback);
    return 0;
  }
  case 2: {
    connection->setUserName(selfInfo.getExtraChannelInfo().userName);
    connection->setPassword(selfInfo.getExtraChannelInfo().password);
    connection->setRealm(selfInfo.getExtraChannelInfo().realm);
    connection->setSsrc(selfInfo.getExtraChannelInfo().ssrc);
    auto receiveCallback = [&](uint8_t* data, size_t len, int64_t ts, uint16_t seqNum,
      std::vector<uint32_t> ssrcVec, int payloadType) {
        this->onReceiveRtpData(2, data, len, ts, seqNum, ssrcVec, payloadType);
    };
    connection->setRTPDataRecvCallback(receiveCallback);
    connection->startGatherAndConnect(
      selfInfo.getExtraChannelInfo().sfuIp + ":" +
      std::to_string(selfInfo.getExtraChannelInfo().sfuPort),
      connectionCallback);
    return 0;
  }
  default: {
    W_LOG("create channel: mediaType-{} error", mediaType);
    return -1;
  }
  }
}

void NetManager::destroyAllChannels() {
  if (audioConnection) {
    audioConnection->stop();
    audioConnection = nullptr;
  }
  if (videoConnection) {
    videoConnection->stop();
    videoConnection = nullptr;
  }
  if (secondVideoConnection) {
    secondVideoConnection->stop();
    secondVideoConnection = nullptr;
  }
}

STATE_CODE NetManager::sendRTPData(int mediaType, uint8_t* data, size_t len, int64_t ts, uint8_t payLoadType, bool mark) {
  STATE_CODE rst = STATE_CODE::CHANNEL_UNCOMPLETE;
  switch (mediaType) {
  case 0: {
    if (audioConnection) {
      rst = audioConnection->sendRTPData(data, len, ts, payLoadType, mark);
    }
    break;
  }
  case 1: {
    if (videoConnection) {
      rst = videoConnection->sendRTPData(data, len, ts, payLoadType, mark);
    }
    break;
  }
  case 2: {
    if (secondVideoConnection) {
      rst = secondVideoConnection->sendRTPData(data, len, ts, payLoadType, mark);
    }
    break;
  }
  default: {
    W_LOG("sendRTPData: mediaType-{} error", mediaType);
    break;
  }
  }
  return rst;
}

int NetManager::sendRTPData(int mediaType, std::vector<std::pair<uint8_t*, int>>& nalus,
  int64_t ts, uint8_t payloadType, bool mark) {

  std::shared_ptr<SFUConnection_imp_88> sendChannel = nullptr;
  switch (mediaType) {
  case 0: {
    sendChannel = audioConnection;
    break;
  }
  case 1: {
    sendChannel = videoConnection;
    break;
  }
  case 2: {
    sendChannel = secondVideoConnection;
    break;
  }
  default: {
    W_LOG("sendRTPData: mediaType-{} error", mediaType);
    break;
  }
  }
  if (!sendChannel) {
    return -1;
  }

  if (nalus.size() == 0) {
    W_LOG("nalus size == 0");
  }
  else if (nalus.size() == 1) {
    STATE_CODE state =
      sendChannel->sendRTPData(nalus.at(0).first, nalus.at(0).second, 0, payloadType, true);
    if (state != STATE_CODE::SUCCESS) {
      W_LOG("send packet error, code = {}", state);
      return -1;
    }
    free(nalus.at(0).first);
    std::this_thread::sleep_for(std::chrono::microseconds(1000));
  }
  else {
    int idx = 0;
    for (auto nalu : nalus) {
      STATE_CODE state =
        sendChannel->sendRTPData(nalu.first, nalu.second, seeker::Time::currentTime(),
          payloadType, idx == nalus.size() - 1);
      if (state != theia::NetIo::STATE_CODE::SUCCESS) {
        W_LOG("send packet error, code = {}", state);
        return -1;
      }
      idx++;
      free(nalu.first);
      std::this_thread::sleep_for(std::chrono::microseconds(1000));
    }
  }
  return 0;
}

int NetManager::setRTPDataRecvCallback(int mediaType, RTP_DATA_CALLBACK callback) {
  switch (mediaType) {
  case 0: {
    E_LOG("audio");
    audioChannelDataCb = callback;
    return 0;
  }
  case 1: {
    E_LOG("video");
    videoChannelDataCb = callback;
    return 0;
  }
  case 2: {
    secondVideoChannelDataCb = callback;
    return 0;
  }
  default: {
    W_LOG("setRTPDataRecvCallback: mediaType-{} error", mediaType);
    return -1;
  }
  }
}

bool NetManager::isSecondVideoChannelCreated() {
  if (secondVideoConnection) {
    return true;
  }
  else {
    return false;
  }
}

void NetManager::onReceiveRtpData(int mediaType, uint8_t* data, size_t len, int64_t ts,
  uint16_t seqNum, std::vector<uint32_t> ssrcVec,
  int payloadType) {
  switch (mediaType) {
  case 0: {
    if (audioChannelDataCb) {
      //E_LOG("audio cb");
      audioChannelDataCb(data, len, ts, seqNum, ssrcVec, payloadType);
    }
    break;
  }
  case 1: {
    if (videoChannelDataCb) {
      E_LOG("video cb");
      videoChannelDataCb(data, len, ts, seqNum, ssrcVec, payloadType);
    }
    break;
  }
  case 2: {
    if (secondVideoChannelDataCb) {
      secondVideoChannelDataCb(data, len, ts, seqNum, ssrcVec, payloadType);
    }
    break;
  }
  default: {
    W_LOG("onReceiveRtpData: mediaType-{} error", mediaType);
    break;
  }
  }
}

theia::NetIo::ConnectionNetMeta NetManager::getNetCondition(int type) {
  //if (type == 0) {
  //  if (audioConnection) {
  //    auto audioSfuNetMeta = audioConnection->getConnectionNetMeta();
  //    return audioSfuNetMeta;
  //  }
  //} else if (type == 1) {
  //  if (videoConnection) {
  //    auto videoSfuNetMeta = videoConnection->getConnectionNetMeta();
  //    return videoSfuNetMeta;
  //  }
  //}else if (type == 2) {
  //  if (secondVideoConnection) {
  //    auto secondVideoSfuNetMeta = secondVideoConnection->getConnectionNetMeta();
  //    return secondVideoSfuNetMeta;
  //  }
  //}
  theia::NetIo::ConnectionNetMeta meta;
  return meta;
}

theia::NetIo::OtherLossInfo NetManager::getLossInfo(int32_t ssrc, int type) {
  theia::NetIo::OtherLossInfo lossInfo;
  //if (type == 0) {
  //  auto audioSfuNetMeta = this->getNetCondition(type);
  //  lossInfo = audioSfuNetMeta.getOtherLossInfo(ssrc);
  //} else if (type == 1) {
  //  auto videoSfuNetMeta = this->getNetCondition(type);
  //  lossInfo = videoSfuNetMeta.getOtherLossInfo(ssrc);
  //}
  return lossInfo;
}

void NetManager::setRtmpUrl(std::string _rtmpUrl) {
  rtmpUrl = _rtmpUrl;
}

int NetManager::rtmpInit(int step) {
  int ret;
  mutex4Sender = std::make_unique<std::mutex>();
  if (step == 0) {
    av_register_all();
    //初始化网络库
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();

    I_LOG("connecting to rtmp server :{}", rtmpUrl.c_str());
    //设置输出文件
    ret = avformat_alloc_output_context2(&pFormatCtx, 0, "flv", rtmpUrl.c_str());
    if (ret) {
      E_LOG("connect rtmp fail:{}", ret);
      return -1;
    }
    fmt = pFormatCtx->oformat;

    //新建一个输出流

    if (video_st == nullptr) {
      video_st = avformat_new_stream(pFormatCtx, NULL);
      if (video_st == NULL) {
        E_LOG("failed allocating video stream\n");
        return -1;
      }
    }

    //新建一个输出流

    if (audio_st == nullptr) {
      audio_st = avformat_new_stream(pFormatCtx, NULL);
      if (audio_st == NULL) {
        E_LOG("failed allocating audio stream\n");
        return -1;
      }
    }

  }
  else {
    //打开输出文件
    if (avio_open(&pFormatCtx->pb, rtmpUrl.c_str(), AVIO_FLAG_WRITE)) {
      E_LOG("connect to rtmp server fail!");
      return -1;
    }

    //写文件头
//    AVDictionary * opts = nullptr;
//    av_dict_set(&opts, "flvflags", "no_duration_filesize", 0);
    ret = avformat_write_header(pFormatCtx, nullptr);
    if (ret) {
      E_LOG("write header error:{}", ret);
      return -1;
    }

    av_dump_format(pFormatCtx, 0, rtmpUrl.c_str(), 1);
  }
  return 0;
}

int NetManager::sendRTMPData(AVPacket* packet) {
  std::unique_lock<std::mutex> senderLk(*mutex4Sender);
  int ret = av_interleaved_write_frame(pFormatCtx, packet);
  if (ret < 0) {
    char buf[1024] = { 0 };
    av_strerror(ret, buf, sizeof(buf));
    E_LOG("push error code {}, error info {}", ret, buf);
  }
  senderLk.unlock();
  return ret;
}

int NetManager::setVideoStream(AVCodecContext* encodeCtx) {
  //copy 编码器的参数到输出流中
  int ret = avcodec_parameters_from_context(video_st->codecpar, encodeCtx);
  if (ret < 0) {
    printf("video stream copy params error.\n");
    return ret;
  }
  return ret;
}

int NetManager::setAudioStream(AVCodecContext* encodeCtx) {
  //copy 编码器的参数到输出流中
  int ret = avcodec_parameters_from_context(audio_st->codecpar, encodeCtx);
  if (ret < 0) {
    printf("audio stream copy params error.\n");
    return ret;
  }
  return ret;
}

int NetManager::newAudioStream(AVCodecContext* encodeCtx) {
  //新建一个输出流

  if (audio_st == nullptr) {
    audio_st = avformat_new_stream(pFormatCtx, encodeCtx->codec);
    if (audio_st == NULL) {
      E_LOG("failed allocating audio stream\n");
      return -1;
    }
  }

  //copy 编码器的参数到输出流中
  int ret = avcodec_parameters_from_context(audio_st->codecpar, encodeCtx);
  if (ret < 0) {
    printf("audio stream copy params error.\n");
    return ret;
  }
  return ret;
}

int NetManager::newVideoStream(AVCodecContext* encodeCtx) {
  //新建一个输出流

  if (video_st == nullptr) {
    video_st = avformat_new_stream(pFormatCtx, encodeCtx->codec);
    if (video_st == NULL) {
      E_LOG("failed allocating video stream\n");
      return -1;
    }
  }

  //copy 编码器的参数到输出流中
  int ret = avcodec_parameters_from_context(video_st->codecpar, encodeCtx);
  if (ret < 0) {
    printf("video stream copy params error.\n");
    return ret;
  }
  return ret;
}

int NetManager::stopRTMP() {
  //Write file trailer
  //av_write_trailer(pFormatCtx);
  /* close output */
  if (pFormatCtx && !(fmt->flags & AVFMT_NOFILE))
    avio_close(pFormatCtx->pb);
  avformat_free_context(pFormatCtx);
  return 0;
}

NetManager::~NetManager() {
  I_LOG("NetManager destruct ...");
  if (pFormatCtx) {
    av_write_trailer(pFormatCtx);
  }
  /* close output */
  if (pFormatCtx && !(pFormatCtx->flags & AVFMT_NOFILE))
    avio_close(pFormatCtx->pb);
  avformat_free_context(pFormatCtx);
}