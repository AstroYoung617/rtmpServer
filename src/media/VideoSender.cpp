#include <iostream>
#include <media/VideoSender.h>

VideoSender::VideoSender(std::mutex* _mutex, std::condition_variable* _vdcv) {
	I_LOG("construct videoSender success");
  this->mutex = _mutex;
  this->vdcv = _vdcv;
}

static double r2d(AVRational r)
{
  return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}


void VideoSender::sendRtmpData() {
  AVOutputFormat* ofmt = NULL;
  //Input AVFormatContext and Output AVFormatContext
  AVFormatContext* ifmt_ctx_v = NULL, * ofmt_ctx = NULL;
  AVPacket pkt;
  int ret, i;
  int videoindex_v = -1, videoindex_out = -1;
  //int audioindex_a=-1,audioindex_out=-1;
  int frame_index_v = 0;
  //int frame_index_a=0;
  int frame_index = 0;
  int64_t cur_pts_v = 0;
  long long startTime;

  //FIX
#if USE_H264BSF
  AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
#endif
#if USE_AACBSF
  AVBitStreamFilterContext* aacbsfc = av_bitstream_filter_init("aac_adtstoasc");
#endif

  const char* in_filename_v = "./receive.264";
  const char* out_filename = "rtmp://live-push.bilivideo.com/live-bvc/?streamname=live_98579344_16311849&key=41a6b8b86a64eaeccb3efe3679940c43&schedule=rtmp&pflag=1";
  //const char *out_filename = "E:/testflv.flv";

  av_register_all();
  //初始化网络库
  avformat_network_init();
  //Input
  if ((ret = avformat_open_input(&ifmt_ctx_v, in_filename_v, 0, 0)) < 0) {
    printf("Could not open input file.");
    goto end;
  }
  if ((ret = avformat_find_stream_info(ifmt_ctx_v, 0)) < 0) {
    printf("Failed to retrieve input stream information");
    goto end;
  }

  printf("===========Input Information==========\n");
  av_dump_format(ifmt_ctx_v, 0, in_filename_v, 0);
  printf("======================================\n");
  //Output
  avformat_alloc_output_context2(&ofmt_ctx, 0, "flv", out_filename);
  if (!ofmt_ctx) {
    printf("Could not create output context\n");
    ret = AVERROR_UNKNOWN;
    goto end;
  }
  ofmt = ofmt_ctx->oformat;

  for (i = 0; i < ifmt_ctx_v->nb_streams; i++) {
    //Create output AVStream according to input AVStream
    if (ifmt_ctx_v->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
      AVStream* in_stream = ifmt_ctx_v->streams[i];
      AVStream* out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
      videoindex_v = i;
      if (!out_stream) {
        printf("Failed allocating output stream\n");
        ret = AVERROR_UNKNOWN;
        goto end;
      }
      videoindex_out = out_stream->index;
      //Copy the settings of AVCodecContext
      if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
        printf("Failed to copy context from input to output stream codec context\n");
        goto end;
      }
      out_stream->codec->codec_tag = 0;
      //      if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
      //        out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
      break;
    }
  }


  printf("==========Output Information==========\n");
  av_dump_format(ofmt_ctx, 0, out_filename, 1);
  printf("======================================\n");
  //Open output file
  if (!(ofmt->flags & AVFMT_NOFILE)) {
    if (avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE) < 0) {
      printf("Could not open output file '%s'", out_filename);
      goto end;
    }
  }
  //Write file header
  if (avformat_write_header(ofmt_ctx, NULL) < 0) {
    printf("Error occurred when opening output file\n");
    goto end;
  }

  startTime = av_gettime();
  while (1) {
    AVFormatContext* ifmt_ctx;
    int stream_index = 0;
    AVStream* in_stream, * out_stream;

    //Get an AVPacket
    ifmt_ctx = ifmt_ctx_v;
    stream_index = videoindex_out;
    if (av_read_frame(ifmt_ctx, &pkt) >= 0) {
      do {
        std::unique_lock<std::mutex> lck(*mutex);
        //vdcv->wait_for(lck, std::chrono::milliseconds(10));
        in_stream = ifmt_ctx->streams[pkt.stream_index];
        out_stream = ofmt_ctx->streams[stream_index];

        if (pkt.stream_index == videoindex_v) {
          //FIX：No PTS (Example: Raw H.264)
          //Simple Write PTS
          if (pkt.pts == AV_NOPTS_VALUE) {
            //Write PTS
            AVRational time_base1 = in_stream->time_base;
            //Duration between 2 frames (us)
            int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
            //Parameters
            pkt.pts = (double)(frame_index_v * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE);
            pkt.dts = pkt.pts;
            pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE);
            frame_index_v++;
          }
          cur_pts_v = pkt.pts;
          break;
          lck.unlock();
        }
      } while (av_read_frame(ifmt_ctx, &pkt) >= 0);
    }
    else {
      break;
    }

    //FIX:Bitstream Filter
#if USE_H264BSF
    av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif
#if USE_AACBSF
    av_bitstream_filter_filter(aacbsfc, out_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
#endif


    //Convert PTS/DTS
//    pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
//    pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
    pkt.pts = av_rescale_q(pkt.pts, in_stream->time_base, out_stream->time_base);
    pkt.dts = av_rescale_q(pkt.dts, in_stream->time_base, out_stream->time_base);
    pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
    pkt.pos = -1;
    pkt.stream_index = stream_index;

    //视频帧推送速度
    if (stream_index == videoindex_out)
    {
      //已经过去的时间
      long long now = av_gettime() - startTime;
      long long pts = 0;
      pts = pkt.pts * (1000 * 1000 * r2d(out_stream->time_base));
      if (pts > now)
      {
        av_usleep(pts - now);
        std::cout << "now:" << now << " pts:" << pts << " p-n:" << pts - now << std::endl;
      }

      //cout << pkt.dts << "-----" << pkt.pts << endl;
    }

    //printf("Write 1 Packet. size:%5d\tpts:%lld\n",pkt.size,pkt.pts);
    //Write
    if (stream_index == videoindex_out)
    {
      std::cout << "video: " << " pts:" << pkt.pts << " dts:" << pkt.dts << " duration:" << pkt.duration << std::endl;
      //printf("%lld", pkt.pts);
    }
    if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
      printf("Error muxing packet\n");
      break;
    }
    av_free_packet(&pkt);

  }
  //Write file trailer
  //av_write_trailer(ofmt_ctx);

#if USE_H264BSF
  av_bitstream_filter_close(h264bsfc);
#endif
#if USE_AACBSF
  av_bitstream_filter_close(aacbsfc);
#endif

end:
  avformat_close_input(&ifmt_ctx_v);
  /* close output */
  if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
    avio_close(ofmt_ctx->pb);
  avformat_free_context(ofmt_ctx);
  if (ret < 0 && ret != AVERROR_EOF) {
    printf("Error occurred.\n");
    return;
  }
  return;
}

VideoSender::~VideoSender() {
	I_LOG("videoSender destruct...");
}