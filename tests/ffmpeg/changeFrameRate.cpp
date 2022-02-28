#include <stdio.h>

//对frame进行操作后可以成功的提高帧率，感觉效果还行
extern "C" // 因为FFmpeg是纯C程序
{
  // FFmpeg libraries
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
};



/////////////////////////////////////////////////////////////////////////////////////////
// 视频文件解码为YUV数据

int main(int argc, char* argv[])
{
  AVFormatContext* pFormatCtx;
  AVCodecContext* pCodecCtx;
  AVCodec* pCodec;
  AVFrame* pFrame, * pFrameYUV;
  AVPacket* packet;
  struct SwsContext* img_convert_ctx;
  uint8_t* out_buffer;

  int    videoindex = -1;
  int y_size;
  int ret, got_picture;

  char filepath[] = "E:/common/time.mp4";
  FILE* fp_yuv = fopen("E:/common/output.yuv", "wb+");

  av_register_all();
  //avformat_network_init();
  pFormatCtx = avformat_alloc_context();

  if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0)
  {
    printf("Couldn't open input stream.\n");
    return -1;
  }
  if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
  {
    printf("Couldn't find stream information.\n");
    return -1;
  }

  for (int i = 0; i < pFormatCtx->nb_streams; i++)
  {
    if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
    {
      videoindex = i;
      break;
    }
  }
  if (videoindex == -1)
  {
    printf("Didn't find a video stream.\n");
    return -1;
  }

  pCodecCtx = pFormatCtx->streams[videoindex]->codec;

  // 根据编码器的ID查找FFmpeg的解码器
  pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
  if (pCodec == NULL)
  {
    printf("Codec not found.\n");
    return -1;
  }
  // 初始化一个视音频编解码器的AVCodecContext
  if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
  {
    printf("Could not open codec.\n");
    return -1;
  }

  // 一些内存分配
  packet = (AVPacket*)av_malloc(sizeof(AVPacket));
  pFrame = av_frame_alloc();
  pFrameYUV = av_frame_alloc();
  out_buffer = (uint8_t*)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
  // 为已经分配空间的结构体AVPicture挂上一段用于保存数据的空间
  // AVFrame/AVPicture有一个data[4]的数据字段,buffer里面存放的只是yuv这样排列的数据，
  // 而经过fill 之后，会把buffer中的yuv分别放到data[0],data[1],data[2]中。
  avpicture_fill((AVPicture*)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);

  //Output Info-----------------------------
  printf("--------------- File Information ----------------\n");
  av_dump_format(pFormatCtx, 0, filepath, 0);
  printf("-------------------------------------------------\n");

  // 初始化一个SwsContext
  // 参数：源图像的宽，源图像的高，源图像的像素格式，目标图像的宽，目标图像的高，目标图像的像素格式，设定图像拉伸使用的算法
  img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
    pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
  int count = 0;
  while (av_read_frame(pFormatCtx, packet) >= 0)
  {
    if (packet->stream_index == videoindex)
    {
      // 解码一帧视频数据。输入一个压缩编码的结构体AVPacket，输出一个解码后的结构体AVFrame
      ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
      if (ret < 0)
      {
        printf("Decode Error.\n");
        return -1;
      }
      if (got_picture)
      {
        // 转换像素
        sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
          pFrameYUV->data, pFrameYUV->linesize);
        y_size = pCodecCtx->width * pCodecCtx->height;
        // 向文件写入一个数据块
        fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);      //Y 
        fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
        fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V

        fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);      //Y 
        fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
        fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V

        if (count % 2) {
          fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);      //Y 
          fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
          fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
        }

        count++;

        printf("Succeed to decode 1 frame!\n");

      }
    }
    av_free_packet(packet);
  }

  //flush decoder
  //FIX: Flush Frames remained in Codec
  int count1 = 0;

  while (1)
  {
    ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
    if (ret < 0)
      break;
    if (!got_picture)
      break;
    sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
      pFrameYUV->data, pFrameYUV->linesize);

    int y_size = pCodecCtx->width * pCodecCtx->height;

    fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);      //Y 
    fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
    fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V

    fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);      //Y 
    fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
    fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V

    if (count1 % 2) {
      fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);      //Y 
      fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
      fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
    }

    count1++;
    //printf("Flush Decoder: Succeed to decode 1 frame!\n");
  }

  sws_freeContext(img_convert_ctx);
  fclose(fp_yuv);
  av_frame_free(&pFrameYUV);
  av_frame_free(&pFrame);
  avcodec_close(pCodecCtx);
  avformat_close_input(&pFormatCtx);

  return 0;
}