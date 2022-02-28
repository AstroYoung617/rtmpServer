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
  int ret, ret1;
  mutex4Sender = std::make_unique<std::mutex>();
  if (step == 0) {
    av_register_all();
    //初始化网络库
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();
    pFormatCtx1 = avformat_alloc_context();

    I_LOG("connecting to rtmp server :{}", rtmpUrl.c_str());
    //设置输出文件
    ret = avformat_alloc_output_context2(&pFormatCtx, 0, "flv", rtmpUrl.c_str());
    if (ret) {
      E_LOG("connect rtmp fail:{}", ret);
      return -1;
    }
    I_LOG("connecting to rtmp server :{}", rtmpUrl1.c_str());
    ret1 = avformat_alloc_output_context2(&pFormatCtx1, 0, "flv", rtmpUrl1.c_str());
    if (ret1) {
      E_LOG("connect rtmp fail:{}", ret1);
      return -1;
    }
    fmt = pFormatCtx->oformat;
    fmt1 = pFormatCtx1->oformat;

    //新建一个输出流

    if (video_st == nullptr) {
      video_st = avformat_new_stream(pFormatCtx, NULL);
      if (video_st == NULL) {
        E_LOG("failed allocating video stream\n");
        return -1;
      }
    }    
    if (video_st1 == nullptr) {
      video_st1 = avformat_new_stream(pFormatCtx1, NULL);
      if (video_st1 == NULL) {
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
    if (audio_st1 == nullptr) {
      audio_st1 = avformat_new_stream(pFormatCtx1, NULL);
      if (audio_st1 == NULL) {
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
    if (avio_open(&pFormatCtx1->pb, rtmpUrl1.c_str(), AVIO_FLAG_WRITE)) {
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
    ret1 = avformat_write_header(pFormatCtx1, nullptr);
    if (ret1) {
      E_LOG("write header error:{}", ret1);
      return -1;
    }

    av_dump_format(pFormatCtx, 0, rtmpUrl.c_str(), 1);
    av_dump_format(pFormatCtx1, 0, rtmpUrl1.c_str(), 1);
  }
  return 0;
}

int NetManager::sendRTMPData(AVPacket* packet) {
  std::unique_lock<std::mutex> senderLk(*mutex4Sender);

  if (packet->stream_index == 0) {
    vtime = packet->pts;
    I_LOG("video timestamp = {}", packet->pts * av_q2d(video_st->time_base)); //这样计算出来的时间就是秒
  }
  else {
    atime = packet->pts;
    I_LOG("audio timestamp = {}", packet->pts * av_q2d(audio_st->time_base));
  }
  //I_LOG("audio time = {}, video time = {}", av_q2d(audio_st->time_base), av_q2d(video_st->time_base));
  auto packet1 = av_packet_clone(packet);
  int ret = av_interleaved_write_frame(pFormatCtx, packet);
  if (ret < 0) {
    char buf[1024] = { 0 };
    av_strerror(ret, buf, sizeof(buf));
    E_LOG("push error code {}, error info {}", ret, buf);
  }
  int ret1 = av_interleaved_write_frame(pFormatCtx1, packet1);
  if (ret1 < 0) {
    char buf[1024] = { 0 };
    av_strerror(ret1, buf, sizeof(buf));
    E_LOG("push error code {}, error info {}", ret1, buf);
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