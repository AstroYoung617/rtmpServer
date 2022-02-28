/*
* Created by Astro 2022.1.25
* Description: ���Խ��룬��Ҫ�ο�ffmpeg��example�Լ���ҳ�ϵ�������̽���

* FFMPEG��Ƶ��������
* 1��av_register_all()��				ע�����������
* 2��avformat_open_input()��			��������Ƶ�ļ���
* 3��avformat_find_stream_info()��	��ȡ��Ƶ�ļ���Ϣ
* 4��avcodec_find_decoder()��		���ҽ�������
* 5��avcodec_open2()��				�򿪽�������
* 6��av_read_frame()��				�������ļ���ȡһ֡ѹ�����ݡ�
* 7��avcodec_decode_video2()��		����һ֡ѹ�����ݡ�
* 8��avcodec_close()��				�رս�������
* 9��avformat_close_input()��		�ر�������Ƶ�ļ���
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
	//�ļ���ʽ������
	AVFormatContext* pFormatCtx;	// ��װ��ʽ�����Ľṹ�壬Ҳ��ͳ��ȫ�ֵĽṹ�壬��������Ƶ�ļ���װ ��ʽ�����Ϣ�� 
	AVCodecContext* pCodecCtx;		// �����������Ľṹ�壬��������Ƶ����Ƶ������������Ϣ��
	AVCodec* pCodec;				// AVCodec�Ǵ洢���������Ϣ�Ľṹ�塣
	AVFrame* pFrame, * pFrameYUV;	// AVFrame�ǰ������������϶�Ľṹ��
	AVPacket* packet;				// AVPacket�Ǵ洢ѹ���������������Ϣ�Ľṹ��
	unsigned char* out_buffer;
	int	i = 0, videoindex;
	int y_size;
	int ret, got_picture;

	// struct SwsContext�ṹ��λ��libswscale�����, �������Ҫ���ڴ���ͼƬ��������, �������ͼƬ���ظ�ʽ��ת��, ͼƬ������ȹ���.
	struct SwsContext* img_convert_ctx;
	char filepath[] = "E:/common/video2.h264";
	FILE* fp_yuv = fopen("E:/common/output.yuv", "wb+");
	av_register_all();    // ע���������
	avformat_network_init();   // ����������ȫ�ֳ�ʼ����
	pFormatCtx = avformat_alloc_context();   // ��ʼ��AVFormatContext�ṹ��ָ�롣ʹ��avformat_free_context()�ͷ��ڴ档
	if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0)  // ������������ȡheader������ʹ��avformat_close_input()�ӿڹرա�
	{
		printf("Couldn't open input stream.\n");
		return -1;
	}
	//��ȡһ��������Ƶ���ݲ��һ��һЩ��ص���Ϣ
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) // ��ȡý���ļ��İ��Ի�ȡ����Ϣ
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}

	//������Ƶ��������
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

	//�����������
	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	//���ҽ�����
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id); // ���ҷ���ID����ע�������
	if (pCodec == NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}
	//�򿪽�����
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		printf("Could not open codec.\n");
		return -1;
	}
	//����AVFrame������ԭʼ��Ƶ
	pFrame = av_frame_alloc();
	//����AVFrame������yuv��Ƶ
	pFrameYUV = av_frame_alloc();
	//�����ڴ棬����ͼ���ʽת��
	out_buffer = (unsigned char*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));
	// ����ָ����ͼ��������ṩ���������ò���ָ���linesize��С
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);
	packet = (AVPacket*)av_malloc(sizeof(AVPacket));
	//Output Info-----------------------------
	printf("--------------- File Information ----------------\n");
	//�ֹ����Ժ��������tbn��tbc��tbr��PAR��DAR�ĺ���
	av_dump_format(pFormatCtx, 0, filepath, 0);
	printf("-------------------------------------------------\n");

	//����ת�������ġ� sws_getContext���ܣ���ʼ�� SwsContext �ṹ��ָ��  
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, NULL, NULL, NULL, NULL);

	//��ȡ����
	while (av_read_frame(pFormatCtx, packet) >= 0) // ��ȡ�����е���Ƶ����֡������Ƶһ֡
	{
		if (packet->stream_index == videoindex)
		{
			// avcodec_decode_video2 ����:����һ֡��Ƶ����
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if (ret < 0)
			{
				printf("Decode Error.\n");
				return -1;
			}

			if (got_picture >= 1)
			{
				//�ɹ�����һ֡
				sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
					pFrameYUV->data, pFrameYUV->linesize); // ת��ͼ���ʽ

				y_size = pCodecCtx->width * pCodecCtx->height;
				// fwrite ����:�� pFrameYUV ��ָ������д�뵽 fp_yuv �С�
				fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y 
				fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U
				fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V
				printf("Succeed to decode 1 frame!\n");
			}
			else
			{
				//δ���뵽һ֡������ʱ��βB֡���ӳ�֡���ں�����flush decoder����
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
		// avcodec_decode_video2 ����:����һ֡��Ƶ����
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
			pFrameYUV->data, pFrameYUV->linesize); // ת��ͼ���ʽ

		int y_size = pCodecCtx->width * pCodecCtx->height;
		// fwrite ����:�� pFrameYUV ��ָ������д�뵽 fp_yuv �С�
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

