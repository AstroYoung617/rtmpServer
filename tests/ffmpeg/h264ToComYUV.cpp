/*
* Created by Astro 2022.1.25
* Description: 测试解码，主要参考ffmpeg的example以及网页上的相关流程讲解

* FFMPEG视频解码流程
* 1、av_register_all()：				注册所有组件。
* 2、avformat_open_input()：			打开输入视频文件。
* 3、avformat_find_stream_info()：	获取视频文件信息
* 4、avcodec_find_decoder()：		查找解码器。
* 5、avcodec_open2()：				打开解码器。
* 6、av_read_frame()：				从输入文件读取一帧压缩数据。
* 7、avcodec_decode_video2()：		解码一帧压缩数据。
* 8、avcodec_close()：				关闭解码器。
* 9、avformat_close_input()：		关闭输入视频文件。
*/


#include <stdio.h>
#include <iostream>
#include <thread>

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
};
#else

//Linux...
#ifdef __cplusplus
extern "C"
{
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
};
#endif
#endif

int main()
{
	//文件格式上下文
	AVFormatContext* pFormatCtx;	// 封装格式上下文结构体，也是统领全局的结构体，保存了视频文件封装 格式相关信息。 
	AVCodecContext* pCodecCtx;		// 编码器上下文结构体，保存了视频（音频）编解码相关信息。
	AVCodec* pCodec;				// AVCodec是存储编解码器信息的结构体。
	AVFrame* pFrame, * pFrameYUV;	// AVFrame是包含码流参数较多的结构体
	AVPacket* packet;				// AVPacket是存储压缩编码数据相关信息的结构体
	unsigned char* out_buffer;
	int	i = 0, videoindex;
	int y_size;
	int ret, got_picture;

	// struct SwsContext结构体位于libswscale类库中, 该类库主要用于处理图片像素数据, 可以完成图片像素格式的转换, 图片的拉伸等工作.
	struct SwsContext* img_convert_ctx;
	char filepath[] = "E:/common/video2.h264";
	FILE* fp_yuv = fopen("E:/common/output.yuv", "wb+");
	av_register_all();    // 注册所有组件
	avformat_network_init();   // 对网络库进行全局初始化。
	pFormatCtx = avformat_alloc_context();   // 初始化AVFormatContext结构体指针。使用avformat_free_context()释放内存。
	if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0)  // 打开输入流并读取header。必须使用avformat_close_input()接口关闭。
	{
		printf("Couldn't open input stream.\n");
		return -1;
	}
	//读取一部分视音频数据并且获得一些相关的信息
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) // 读取媒体文件的包以获取流信息
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}

	//查找视频编码索引
	videoindex = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
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

	//编解码上下文
	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	//查找解码器
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id); // 查找符合ID的已注册解码器
	if (pCodec == NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}
	//打开解码器
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		printf("Could not open codec.\n");
		return -1;
	}
	//申请AVFrame，用于原始视频
	pFrame = av_frame_alloc();
	//申请AVFrame，用于yuv视频
	pFrameYUV = av_frame_alloc();
	//分配内存，用于图像格式转换
	out_buffer = (unsigned char*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));
	// 根据指定的图像参数和提供的数组设置参数指针和linesize大小
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);
	packet = (AVPacket*)av_malloc(sizeof(AVPacket));
	//Output Info-----------------------------
	printf("--------------- File Information ----------------\n");
	//手工调试函数，输出tbn、tbc、tbr、PAR、DAR的含义
	av_dump_format(pFormatCtx, 0, filepath, 0);
	printf("-------------------------------------------------\n");

	//申请转换上下文。 sws_getContext功能：初始化 SwsContext 结构体指针  
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, NULL, NULL, NULL, NULL);

	//读取数据
	while (av_read_frame(pFormatCtx, packet) >= 0) // 读取码流中的音频若干帧或者视频一帧
	{
		if (packet->stream_index == videoindex)
		{
			// avcodec_decode_video2 功能:解码一帧视频数据
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if (ret < 0)
			{
				printf("Decode Error.\n");
				return -1;
			}

			if (got_picture >= 1)
			{
				//成功解码一帧
				sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
					pFrameYUV->data, pFrameYUV->linesize); // 转换图像格式

				y_size = pCodecCtx->width * pCodecCtx->height;
				// fwrite 功能:把 pFrameYUV 所指向数据写入到 fp_yuv 中。
				fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y 
				fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
				fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
				printf("Succeed to decode 1 frame!\n");
			}
			else
			{
				//未解码到一帧，可能时结尾B帧或延迟帧，在后面做flush decoder处理
			}
		}
		av_free_packet(packet); // free

	}

	//flush decoder
	//FIX: Flush Frames remained in Codec
	while (true)
	{
		if (!(pCodec->capabilities & AV_CODEC_CAP_DELAY))
			return 0;
		// avcodec_decode_video2 功能:解码一帧视频数据
		ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
		if (ret < 0)
		{
			break;
		}
		if (!got_picture)
		{
			break;
		}

		sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
			pFrameYUV->data, pFrameYUV->linesize); // 转换图像格式

		int y_size = pCodecCtx->width * pCodecCtx->height;
		// fwrite 功能:把 pFrameYUV 所指向数据写入到 fp_yuv 中。
		fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y 
		fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
		fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
		printf("Flush Decoder: Succeed to decode 1 frame!\n");
	}

	sws_freeContext(img_convert_ctx);
	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);
	fclose(fp_yuv);

	return 0;
}

