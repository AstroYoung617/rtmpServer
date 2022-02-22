
#include <stdio.h>
#include <iostream>

#define __STDC_CONSTANT_MACROS
#define FRAMEWITH 640
#define FRAMEHEIGTH 480

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/pixfmt.h"
};

#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "avfilter.lib")
//#pragma comment(lib, "postproc.lib")
#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "swscale.lib")

int main(int argc, char* argv[])
{
    AVFormatContext* pFormatCtx1, * pFormatCtx2;
    int             i, videoindex1;
    int             i2, videoindex2;
    AVCodecContext* pCodecCtx1, * pCodecCtx2;
    AVCodec* pCodec1, * pCodec2;
    AVFrame* pFrame1, * pFrameYUV1, * pDstFrame;
    AVFrame* pFrame2, * pFrameYUV2;
    uint8_t* out_buffer1, * out_buffer2;
    AVPacket* packet1, * packet2;
    int y_size;
    int ret1,ret2, got_picture1, got_picture2;
    struct SwsContext* img_convert_ctx1, * img_convert_ctx2;

    //输入文件路径
    char filepath1[] = "E:/common/video1.mp4";
    char filepath2[] = "E:/common/video2.mp4";
    int frame_cnt1, frame_cnt2;

    av_register_all();
    avformat_network_init();
    pFormatCtx1 = avformat_alloc_context();
    pFormatCtx2 = avformat_alloc_context();

    if (avformat_open_input(&pFormatCtx1, filepath1, NULL, NULL) != 0) {
        printf("不能打开输入的视频文件1\n");
        return -1;
    }
    if (avformat_open_input(&pFormatCtx2, filepath2, NULL, NULL) != 0) {
        printf("不能打开输入的视频文件2\n");
        return -1;
    }
    if (avformat_find_stream_info(pFormatCtx1, NULL) < 0) {
        printf("不能获取文件信息1.\n");
        return -1;
    }
    if (avformat_find_stream_info(pFormatCtx2, NULL) < 0) {
        printf("不能获取文件信息2.\n");
        return -1;
    }


    videoindex1 = -1;
    for (i = 0; i < pFormatCtx1->nb_streams; i++)
        if (pFormatCtx1->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex1 = i;
            break;
        }
    if (videoindex1 == -1) {
        printf("Didn't find a video stream.\n");
        return -1;
    }

    videoindex2 = -1;
    for (i = 0; i < pFormatCtx2->nb_streams; i++)
        if (pFormatCtx2->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex2 = i;
            break;
        }
    if (videoindex2 == -1) {
        printf("Didn't find2 a video stream.\n");
        return -1;
    }

    //avcodec_find_decoder()查找解码器
    pCodecCtx1 = pFormatCtx1->streams[videoindex1]->codec;
    pCodec1 = avcodec_find_decoder(pCodecCtx1->codec_id);
    if (pCodec1 == NULL) {
        printf("Codec1 not found.\n");
        return -1;
    }
    pCodecCtx2 = pFormatCtx2->streams[videoindex2]->codec;
    pCodec2 = avcodec_find_decoder(pCodecCtx2->codec_id);
    if (pCodec2 == NULL) {
        printf("Codec2 not found.\n");
        return -1;
    }

    //avcodec_open2()打开解码器
    if (avcodec_open2(pCodecCtx1, pCodec1, NULL) < 0) {
        printf("Could not open codec2.\n");
        return -1;
    }
    if (avcodec_open2(pCodecCtx2, pCodec2, NULL) < 0) {
        printf("Could not open codec2.\n");
        return -1;
    }

    pFrame1 = av_frame_alloc();
    pFrameYUV1 = av_frame_alloc();
    out_buffer1 = (uint8_t*)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, FRAMEWITH, FRAMEHEIGTH));
    avpicture_fill((AVPicture*)pFrameYUV1, out_buffer1, AV_PIX_FMT_YUV420P, FRAMEWITH, FRAMEHEIGTH);

    pFrame2 = av_frame_alloc();
    pFrameYUV2 = av_frame_alloc();
    out_buffer2 = (uint8_t*)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, FRAMEWITH, FRAMEHEIGTH));
    avpicture_fill((AVPicture*)pFrameYUV2, out_buffer2, AV_PIX_FMT_YUV420P, FRAMEWITH, FRAMEHEIGTH);


    pDstFrame = av_frame_alloc();
    uint8_t* dstbuf = (uint8_t*)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, FRAMEWITH * 2, FRAMEHEIGTH));
    avpicture_fill((AVPicture*)pDstFrame, dstbuf, AV_PIX_FMT_YUV420P, FRAMEWITH * 2, FRAMEHEIGTH);

    pDstFrame->width = FRAMEWITH * 2;
    pDstFrame->height = FRAMEHEIGTH;
    pDstFrame->format = AV_PIX_FMT_YUV420P;


    //将预先分配的AVFrame图像背景数据设置为黑色背景  
    memset(pDstFrame->data[0], 0,    FRAMEWITH * FRAMEHEIGTH * 2);
    memset(pDstFrame->data[1], 0x80, FRAMEWITH * FRAMEHEIGTH / 2);
    memset(pDstFrame->data[2], 0x80, FRAMEWITH * FRAMEHEIGTH / 2);

    packet1 = (AVPacket*)av_malloc(sizeof(AVPacket));
    packet2 = (AVPacket*)av_malloc(sizeof(AVPacket));
    //av_dump_format() 打印关于输入或输出格式的详细信息
    printf("\n");
    av_dump_format(pFormatCtx1, 0, filepath1, 0);
    av_dump_format(pFormatCtx2, 0, filepath2, 0);
    printf("\n");

    //struct SwsContext *img_convert_ctx;定义
    img_convert_ctx1 = sws_getContext(pCodecCtx1->width, pCodecCtx1->height, pCodecCtx1->pix_fmt,
        FRAMEWITH, FRAMEHEIGTH, AV_PIX_FMT_YUV420P, NULL, NULL, NULL, NULL);
    int count = 0;
    frame_cnt1 = 0, frame_cnt2 = 0;

    img_convert_ctx2 = sws_getContext(pCodecCtx2->width, pCodecCtx2->height, pCodecCtx2->pix_fmt,
        FRAMEWITH, FRAMEHEIGTH, AV_PIX_FMT_YUV420P, NULL, NULL, NULL, NULL);


    FILE* fp_yuv420 = fopen("E:/common/testCom.yuv", "wb+");
    while (av_read_frame(pFormatCtx1, packet1) >= 0 && av_read_frame(pFormatCtx2, packet2) >= 0)
    {
        if (packet1->stream_index == videoindex1 && packet2->stream_index == videoindex2) {
            //avcodec_decode_video2()解码一帧压缩数据。
            ret1 = avcodec_decode_video2(pCodecCtx1, pFrame1, &got_picture1, packet1);
            if (ret1 < 0) {
                printf("解码1失败.\n");
                return -1;
            }
            ret2 = avcodec_decode_video2(pCodecCtx2, pFrame2, &got_picture2, packet2);
            if (ret2 < 0) {
                printf("解码2失败.\n");
                return -1;
            }


            if (got_picture1 && got_picture2) {
                sws_scale(img_convert_ctx1, (const uint8_t* const*)pFrame1->data, pFrame1->linesize, 0, pCodecCtx1->height,
                    pFrameYUV1->data, pFrameYUV1->linesize);
                printf("1解码后的帧index: %d\n", frame_cnt1);

                sws_scale(img_convert_ctx2, (const uint8_t* const*)pFrame2->data, pFrame2->linesize, 0, pCodecCtx2->height,
                    pFrameYUV2->data, pFrameYUV2->linesize);
                printf("2解码后的帧index: %d\n", frame_cnt2);
                 
                //for (int i = 0; i < FRAMEHEIGTH; i++)
                //{
                //  fwrite(pFrameYUV1->data[0] + i * pFrameYUV1->linesize[0], 1, pFrameYUV1->linesize[0], fp_yuv420);    //Y1   
                // //fwrite(pFrameYUV2->data[0] + i * pFrameYUV2->linesize[0], 1, pFrameYUV2->linesize[0], fp_yuv420);    //Y2
                //}
                //for (int i = 0; i < FRAMEHEIGTH; i++)
                //{
                //  fwrite(pFrameYUV2->data[0] + i * pFrameYUV2->linesize[0], 1, pFrameYUV2->linesize[0], fp_yuv420);    //Y2
                //}
                //for (int i = 0; i < FRAMEHEIGTH / 2; i++)
                //{
                //  fwrite(pFrameYUV1->data[1] + i * pFrameYUV1->linesize[1], 1, pFrameYUV1->linesize[1], fp_yuv420);  //U1    
                // // fwrite(pFrameYUV2->data[1] + i * pFrameYUV2->linesize[1], 1, pFrameYUV2->linesize[1], fp_yuv420);  //U2    
                //}
                //for (int i = 0; i < FRAMEHEIGTH / 2; i++)
                //{
                //  fwrite(pFrameYUV2->data[1] + i * pFrameYUV2->linesize[1], 1, pFrameYUV2->linesize[1], fp_yuv420);  //U2    
                //}
                //for (int i = 0; i < FRAMEHEIGTH / 2; i++)
                //{
                //  fwrite(pFrameYUV1->data[2] + i * pFrameYUV1->linesize[2], 1, pFrameYUV1->linesize[2], fp_yuv420);  //V1  
                //  //fwrite(pFrameYUV2->data[2] + i * pFrameYUV2->linesize[2], 1, pFrameYUV2->linesize[2], fp_yuv420);  //V2  
                //}
                //for (int i = 0; i < FRAMEHEIGTH / 2; i++)
                //{
                //  fwrite(pFrameYUV2->data[2] + i * pFrameYUV2->linesize[2], 1, pFrameYUV2->linesize[2], fp_yuv420);  //V2  
                //}
                if (pFrameYUV1 && pFrameYUV2)
                {
                    int nYIndex = 0;//每一帧Y值中的第n行
                    int nUIndex = 0;
                    int nVIndex = 0;
                
                    //如果只有Y是没有问题的
                    for (int i = 0; i < FRAMEHEIGTH; i++)
                    {
                        //Y  
                        memcpy(pDstFrame->data[0] + i * pDstFrame->linesize[0], pFrameYUV1->data[0] + nYIndex * pFrameYUV1->linesize[0], pFrameYUV1->linesize[0]);
                        memcpy(pDstFrame->data[0] + pDstFrame->linesize[0] + i * pDstFrame->linesize[0], pFrameYUV2->data[0] + nYIndex * pFrameYUV2->linesize[0], pFrameYUV2->linesize[0]);
                
                        nYIndex++;
                    }
    
                    for (int i = 0; i < FRAMEHEIGTH / 2; i++)
                    {
                      //U
                      memcpy(pDstFrame->data[1] + i * pDstFrame->linesize[1], pFrameYUV1->data[1] + nUIndex * pFrameYUV1->linesize[1], pFrameYUV1->linesize[1]);
                      memcpy(pDstFrame->data[1] + i * pDstFrame->linesize[1] + pDstFrame->linesize[1], pFrameYUV2->data[1] + nUIndex * pFrameYUV2->linesize[1], pFrameYUV2->linesize[1]);
                      nUIndex++;
                    }
                    for (int i = 0; i < FRAMEHEIGTH / 2; i++)
                    {
                        //V  
                        memcpy(pDstFrame->data[2] + i * pDstFrame->linesize[2], pFrameYUV1->data[2] + nVIndex * pFrameYUV1->linesize[2], pFrameYUV1->linesize[2]);
                        memcpy(pDstFrame->data[2] + i * pDstFrame->linesize[2] + pDstFrame->linesize[2], pFrameYUV2->data[2] + nVIndex * pFrameYUV2->linesize[2], pFrameYUV2->linesize[2]);
                        nVIndex++;
                    }
                }
                
                fwrite(pDstFrame->data[0], 1, FRAMEWITH * FRAMEHEIGTH * 2, fp_yuv420);
                fwrite(pDstFrame->data[1], 1, FRAMEWITH * FRAMEHEIGTH / 2, fp_yuv420);
                fwrite(pDstFrame->data[2], 1, FRAMEWITH * FRAMEHEIGTH / 2, fp_yuv420);

                frame_cnt1++;
                frame_cnt2++;
            }
        }
    
        count++;
        av_free_packet(packet1);
        av_free_packet(packet2);
     }
    std::cout << pDstFrame->linesize[0] << " " << pDstFrame->linesize[1] << " " << pDstFrame->linesize[2] << std::endl;

    fclose(fp_yuv420);
    sws_freeContext(img_convert_ctx1);
    av_frame_free(&pFrameYUV1);
    av_frame_free(&pFrame1);
    avcodec_close(pCodecCtx1);
    avformat_close_input(&pFormatCtx1);

    sws_freeContext(img_convert_ctx2);
    av_frame_free(&pFrameYUV2);
    av_frame_free(&pFrame2);
    avcodec_close(pCodecCtx2);
    avformat_close_input(&pFormatCtx2);

    return 0;

}
