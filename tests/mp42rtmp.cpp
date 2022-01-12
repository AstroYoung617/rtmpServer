
extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/time.h"
}
#include <iostream>
using namespace std;
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"avcodec.lib")

//����av_strerror��ϸ������Ϣ
int XError(int errNum)
{
  char buf[1024] = { 0 };
  av_strerror(errNum, buf, sizeof(buf));
  cout << buf << endl;
  getchar();
  return -1;
}

static double r2d(AVRational r)
{
  return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}

int main(int argc, char *argv[])
{
  //�������
  char *inUrl = "E:/common/video1.mp4";

  char *outUrl = "E:/testflv.flv";

  //��ʼ�����з�װ�ͽ��װ flv mp4 mov mp3
  av_register_all();

  //��ʼ�������
  avformat_network_init();

  //
  //������ 1 ���ļ������װ
  //�����װ������
  AVFormatContext *ictx = NULL;

  //���ļ�������ļ�ͷ
  int re = avformat_open_input(&ictx, inUrl, 0, 0);
  if (re != 0)
  {
    return XError(re);
  }
  cout << "open file " << inUrl << " Success." << endl;

  //��ȡ��Ƶ��Ƶ����Ϣ ,h264 flv
  re = avformat_find_stream_info(ictx, 0);
  if (re != 0)
  {
    return XError(re);
  }
  av_dump_format(ictx, 0, inUrl, 0);
  //


  //
  //�����

  //���������������
  AVFormatContext *octx = NULL;
  re = avformat_alloc_output_context2(&octx, 0, "flv", outUrl);
  if (!octx)
  {
    return XError(re);
  }
  cout << "octx create success!" << endl;

  //���������
  //���������AVStream
  for (int i = 0; i < ictx->nb_streams; i++)
  {
    //���������
    AVStream *out = avformat_new_stream(octx, ictx->streams[i]->codec->codec);
    if (!out)
    {
      return XError(0);
    }
    //����������Ϣ,ͬ��MP4
    re = avcodec_copy_context(out->codec, ictx->streams[i]->codec);
    //re = avcodec_parameters_copy(out->codecpar, ictx->streams[i]->codecpar);
    out->codec->codec_tag = 0;
  }
  av_dump_format(octx, 0, outUrl, 1);
  //


  //rtmp����

  //��io
  re = avio_open(&octx->pb, outUrl, AVIO_FLAG_WRITE);
  if (!octx->pb)
  {
    return XError(re);
  }

  //д��ͷ��Ϣ
  re = avformat_write_header(octx, 0);
  printf("in code id = %d  out code id = %d\n", ictx->streams[0]->codecpar->codec_id, octx->streams[0]->codecpar->codec_id);
  printf("in code id = %d  out code id = %d\n", ictx->streams[1]->codecpar->codec_id, octx->streams[1]->codecpar->codec_id);
  if (re < 0)
  {
    return XError(re);
  }
  cout << "avformat_write_header " << re << endl;
  AVPacket pkt;
  long long startTime = av_gettime();
  for (;;)
  {
    re = av_read_frame(ictx, &pkt);
    if (re != 0)
    {
      break;
    }

    //����ת��pts dts
    AVRational itime = ictx->streams[pkt.stream_index]->time_base;
    AVRational otime = octx->streams[pkt.stream_index]->time_base;
    pkt.pts = av_rescale_q(pkt.pts, itime, otime);
    pkt.dts = av_rescale_q(pkt.dts, itime, otime);
    pkt.duration = av_rescale_q(pkt.duration, itime, otime);
    pkt.pos = -1;

    //��Ƶ֡�����ٶ�
    if (ictx->streams[pkt.stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
    {
      //�Ѿ���ȥ��ʱ��
      long long now = av_gettime() - startTime;
      long long pts = 0;
      pts = pkt.pts * (1000 * 1000 * r2d(otime));
      if (pts > now)
      {
        av_usleep(pts - now);
        cout <<"now:"<<now<<" pts:"<<pts<<" p-n:"<< pts - now<<endl;
      }

      //cout << pkt.dts << "-----" << pkt.pts << endl;
    }
    re = av_interleaved_write_frame(octx, &pkt);
    if (re<0)
    {
      return XError(re);
    }
  }

  cout << "file to rtmp test" << endl;
  getchar();
  return 0;
}

