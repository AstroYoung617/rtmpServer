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
  //if (packet->stream_index == 0) {
  //  if (packet->pts == AV_NOPTS_VALUE) {
  //    //Write PTS
  //    AVRational time_base1 = video_st->time_base;
  //    //Duration between 2 frames (us)
  //    int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(video_st->r_frame_rate);
  //    //Parameters
  //    packet->pts = (double)(frame_index_v * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
  //    packet->dts = packet->pts;
  //    packet->duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
  //    frame_index_v++;
  //  }
  //  cur_pts_v = packet->pts;
  //}
  //if (packet->stream_index == 1) {
  //  if (packet->pts == AV_NOPTS_VALUE) {
  //    //Write PTS
  //    AVRational time_base1 = audio_st->time_base;
  //    //Duration between 2 frames (us)
  //    int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(audio_st->r_frame_rate);
  //    //Parameters
  //    packet->pts = (double)(frame_index_a * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
  //    packet->dts = packet->pts;
  //    packet->duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
  //    frame_index_a++;
  //  }
  //  cur_pts_a = packet->pts;
  //}
  if (packet->stream_index == 0)
  {
    std::cout << "video: " << " pts:" << packet->pts << " dts:" << packet->dts << " duration:" << packet->duration << std::endl;
    //printf("%lld", pkt.pts);
  }
  else {
    std::cout << "audio: " << " pts:" << packet->pts << " dts:" << packet->dts << " duration:" << packet->duration << std::endl;
  }
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