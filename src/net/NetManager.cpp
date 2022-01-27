/*
* Created by Astro 2022.1.24
* Descryption: 用于发送rtmp数据到rtmp服务器
*/

//#ifdef _WIN32
//#include "stdafx.h"
//#endif

#include "net/NetManager.h"

NetManager::NetManager() {
  I_LOG("NetManager construct ...");

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
  if (pFormatCtx && !(pFormatCtx->flags & AVFMT_NOFILE)) {
    avio_close(pFormatCtx->pb);
  }
  avformat_free_context(pFormatCtx);
}