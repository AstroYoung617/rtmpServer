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
    AVFormatContext *pFormatCtx;
    int             i, videoindex;
    AVCodecContext  *pCodecCtx;
    AVCodec         *pCodec;
    AVFrame *pFrame, *pFrameYUV, *pDstFrame;
    uint8_t *out_buffer;
    AVPacket *packet;
    int y_size;
    int ret, got_picture;
    struct SwsContext *img_convert_ctx;
    //输入文件路径
    char filepath[]="E:/common/video_640.mp4";

    int frame_cnt;

    av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();

    if(avformat_open_input(&pFormatCtx,filepath,NULL,NULL)!=0){
        printf("Couldn't open input stream.\n");
        return -1;
    }
    if(avformat_find_stream_info(pFormatCtx,NULL)<0){
        printf("Couldn't find stream information.\n");
        return -1;
    }
    videoindex=-1;
    for(i=0; i<pFormatCtx->nb_streams; i++) 
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
            videoindex=i;
            break;
        }
    if(videoindex==-1){
        printf("Didn't find a video stream.\n");
        return -1;
    }

    pCodecCtx=pFormatCtx->streams[videoindex]->codec;
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL){
        printf("Codec not found.\n");
        return -1;
    }
    if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){
        printf("Could not open codec.\n");
        return -1;
    }
    /*
     * 在此处添加输出视频信息的代码
     * 取自于pFormatCtx，使用fprintf()
     */
    pFrame=av_frame_alloc();
    pFrameYUV=av_frame_alloc();
    out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, FRAMEWITH, FRAMEHEIGTH));
    avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, FRAMEWITH, FRAMEHEIGTH);

    pDstFrame = av_frame_alloc();
    int nDstSize = avpicture_get_size(AV_PIX_FMT_YUV420P, FRAMEWITH * 2, FRAMEHEIGTH);
    uint8_t *dstbuf = new uint8_t[nDstSize];
    avpicture_fill((AVPicture*)pDstFrame, dstbuf, AV_PIX_FMT_YUV420P, FRAMEWITH * 2, FRAMEHEIGTH);

    pDstFrame->width = FRAMEWITH * 2;
    pDstFrame->height = FRAMEHEIGTH;
    pDstFrame->format = AV_PIX_FMT_YUV420P;

    //将预先分配的AVFrame图像背景数据设置为黑色背景  
    memset(pDstFrame->data[0], 0, FRAMEWITH * FRAMEHEIGTH * 2);
    memset(pDstFrame->data[1], 0x80, FRAMEWITH * FRAMEHEIGTH / 2);
    memset(pDstFrame->data[2], 0x80, FRAMEWITH * FRAMEHEIGTH / 2);

    packet=(AVPacket *)av_malloc(sizeof(AVPacket));
    //Output Info-----------------------------
    printf("--------------- File Information ----------------\n");
    av_dump_format(pFormatCtx,0,filepath,0);
    printf("-------------------------------------------------\n");
    std::cout << "-------------" << pCodecCtx->pix_fmt << "-----------" << std::endl;

    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, 
        FRAMEWITH, FRAMEHEIGTH, AV_PIX_FMT_YUV420P, 4, NULL, NULL, NULL);
     //   FRAMEWITH, FRAMEHEIGTH, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    int count = 0;
    frame_cnt=0;

    FILE *fp_h264 = fopen("E:/testCom.h264", "wb+");
    FILE *fp_yuv420 = fopen("E:/testCom.yuv", "wb+");
    //这里的packet就是h264的数据，通过av_decode_video2可以得到pFrame
    while(av_read_frame(pFormatCtx, packet)>=0){
        if(packet->stream_index==videoindex){
                /*
                 * 在此处添加输出H264码流的代码
                 * 取自于packet，使用fwrite()
                 */
            fwrite(packet->data,1,packet->size,fp_h264);

            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if(ret < 0){
                printf("Decode Error.\n");
                return -1;
            }
            if(got_picture){
                sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, 
                    pFrameYUV->data, pFrameYUV->linesize);
                printf("Decoded frame index: %d\n",frame_cnt);
                /*
                 * 在此处添加输出YUV的代码
                 * 取自于pFrameYUV，使用fwrite()
                 */

                // 如果需要直接写为YUV文件
                for (int i = 0; i <FRAMEHEIGTH; i++)
                {
                    fwrite(pFrameYUV->data[0] + i*FRAMEWITH, 1, FRAMEWITH, fp_yuv420);    //Y   
                    fwrite(pFrameYUV->data[0] + i*FRAMEWITH, 1, FRAMEWITH, fp_yuv420);    //Y   
                }
                for (int i = 0; i < FRAMEHEIGTH / 4; i++)
                {
                    fwrite(pFrameYUV->data[1] + i*FRAMEWITH, 1, FRAMEWITH, fp_yuv420);  //U    
                    fwrite(pFrameYUV->data[1] + i*FRAMEWITH, 1, FRAMEWITH, fp_yuv420);  //U  
                }
                for (int i = 0; i < FRAMEHEIGTH / 4; i++)
                {
                    fwrite(pFrameYUV->data[2] + i*FRAMEWITH, 1, FRAMEWITH, fp_yuv420);  //V  
                    fwrite(pFrameYUV->data[2] + i*FRAMEWITH, 1, FRAMEWITH, fp_yuv420);  //V  
                }
                

                if (pFrameYUV)
                {
                    int nYIndex = 0;
                    int nUVIndex = 0;

                    for (int i = 0; i < FRAMEHEIGTH; i++)
                    {
                        //Y  
                        memcpy(pDstFrame->data[0] + i*FRAMEWITH * 2, pFrameYUV->data[0] + nYIndex*FRAMEWITH, FRAMEWITH);
                        memcpy(pDstFrame->data[0] + FRAMEWITH + i*FRAMEWITH * 2, pFrameYUV->data[0] + nYIndex*FRAMEWITH, FRAMEWITH);

                        nYIndex++;
                    }

                    for (int i = 0; i < FRAMEHEIGTH / 4; i++)
                    {
                        //U
                        memcpy(pDstFrame->data[1] + i*FRAMEWITH * 2, pFrameYUV->data[1] + nUVIndex*FRAMEWITH, FRAMEWITH);
                        memcpy(pDstFrame->data[1] + FRAMEWITH + i*FRAMEWITH * 2, pFrameYUV->data[1] + nUVIndex*FRAMEWITH, FRAMEWITH);

                        //V  
                        memcpy(pDstFrame->data[2] + i*FRAMEWITH * 2, pFrameYUV->data[2] + nUVIndex*FRAMEWITH, FRAMEWITH);
                        memcpy(pDstFrame->data[2] + FRAMEWITH + i*FRAMEWITH * 2, pFrameYUV->data[2] + nUVIndex*FRAMEWITH, FRAMEWITH);

                        nUVIndex++;
                    }
                }
                fwrite(pDstFrame->data[0], 1, FRAMEWITH*FRAMEHEIGTH * 2, fp_yuv420);
                fwrite(pDstFrame->data[1], 1, FRAMEWITH*FRAMEHEIGTH / 2, fp_yuv420);
                fwrite(pDstFrame->data[2], 1, FRAMEWITH*FRAMEHEIGTH / 2, fp_yuv420);

                frame_cnt++;
            }
        }
        count++;
        av_free_packet(packet);
    }

    //fclose(fp_h264);
    fclose(fp_yuv420);
    sws_freeContext(img_convert_ctx);

    av_frame_free(&pFrameYUV);
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    return 0;
}