/*
	* Created by Astro
	* Date : 2022.01.24
	* Descryption:
		*		���ڽ��ոö˿ڵ�h264��Ƶ��ĿǰӦ��Ҫ������rtp����Ƶ�����浽����
*/

#include <iostream>
#include <string>
#include <other/loggerApi.h>
#include <net/NetManager.h>
#include <media/other/common.h>
#include <media/decoder/VideoDecoder.h>
#include <utility>
#include <mutex>

struct VideoReceiver 
{
	VideoReceiver(int _port, std::mutex* _mutex, std::condition_variable* _vdcv);
	~VideoReceiver();

	//���ü���ȡvideoReceiver���յĶ˿�
	void setPort(int _port);
	int getPort();

	//��ȡ���������ݣ���ʱδ����deque
	AVFrame* getData();

	//��ȡ���������ݣ���ʱδ����deque
	AVPacket* getPacket();

	// NOT USE
	void setData(uint8_t* _data);

	//������յ���Rtp����
	void processRecvRtpData();

private:
	void initSocket();

	void closeSocket();

	void initParser();

	//��ȡAVPacket
	void gnrtPacket(uint8_t* data, size_t len, int64_t ts);

	//decode ����payload ��parser�����д���nalu header...
	void decode(uint8_t* data, size_t len, int64_t ts);

	//���rtp parameter: ��������ݣ� �������ݳ���
	void process(char* bufIn, int len);

	//���������yuv���ļ�
	int saveYUVFrameToFile(AVFrame* frame, int width, int height);

	NALU_t* AllocNALU(int buffersize);
	int port = 0;

	std::mutex* mutex;
	std::condition_variable* vdcv;

	VdDecoder* decoder = nullptr;
	AVFrame* recvFrameCache = nullptr;
	AVFrame* recvFrame = nullptr;

	AVPacket* pkt = nullptr;
	AVPacket* packet = nullptr;
	AVFormatContext* formatCtx = nullptr;
	AVCodecParserContext* parser = NULL;
	const AVCodec* codec = nullptr;
	std::shared_ptr<Parser> decodeParser = nullptr;
	AVCodecContext* c = NULL;
	uint8_t* h264Data = nullptr;

	//network 
	char recvbuf[MAXDATASIZE];  //����ͷ��������� 1500
	int sockfd;
	//FILE* client_fd;
	int sin_size;
	char sendbuf[BUFFER_SIZE];
	struct sockaddr_in server_sockaddr, client_sockaddr;
	AVFrame* frame;
	int	receive_bytes = 0;

};