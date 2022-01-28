#include <iostream>
#include <media/VideoReceiver.h>

FILE* poutfile = NULL;
FILE* fileHandle = fopen("./test.yuv", "wb+");
char* outputfilename = "./receive.264";

VideoReceiver::VideoReceiver(int _port, std::mutex* _mutex, std::condition_variable* _vdcv) {
	I_LOG("VideoReceiver struct success");
	port = _port;
	mutex = move(_mutex);
	vdcv = _vdcv;
	//��ʼ��ͨ��
	initSocket();
}


void VideoReceiver::setPort(int _port) {
	port = _port;
}

int VideoReceiver::getPort() {
	return port;
}

AVFrame* VideoReceiver::getData() {
	std::unique_lock<std::mutex> lk(*mutex);
	if (recvFrameCache && recvFrameCache->data[0]) {
		lk.unlock();
		return recvFrameCache;
	}
	else
		return nullptr;
}

void VideoReceiver::setData(uint8_t* _data) {
	;
}

void VideoReceiver::initSocket() {

	//
	//socket ����
	 //����winsock �����ǿͻ��˻��Ƿ���������Ҫ
	WORD socketVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	//��ʼ��socket��Դ
	if (WSAStartup(socketVersion, &wsaData) != 0)
	{
		return;
	}

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}//����socket���ӣ����ݱ�socket��IPv4Э��
	printf("create socket success!\n");

	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = INADDR_ANY;//0.0.0.0��ȷ����ַ  
	server_sockaddr.sin_port = htons(port);
	memset(&(server_sockaddr.sin_zero), 0, 8);  //���0�Ա�����struct sockaddrͬ����С

	if (bind(sockfd, (struct sockaddr*)&server_sockaddr,
		sizeof(struct sockaddr)) < 0)
	{
		perror("ERROR on binding");
		exit(1);
	}
	printf("bind success!\n");

	sin_size = sizeof(struct sockaddr_in);
	printf("waiting for client connection...\n");
}

void VideoReceiver::closeSocket() {
	strcpy(sendbuf, "success");
	sendto(sockfd, sendbuf, BUFFER_SIZE, 0, (struct sockaddr*)&client_sockaddr, sin_size);
	//fclose(client_fd);
	closesocket(sockfd);
}

void VideoReceiver::processRecvRtpData() {
	//���մӿͻ��˷���������
	RtpPacket* p = NULL;
	RtpHeader* rtp_hdr = NULL;

	if ((receive_bytes = recvfrom(sockfd, recvbuf, MAXDATASIZE, 0, (struct sockaddr*)&client_sockaddr, &sin_size)) > 0)
	{
		if (strncmp(recvbuf, "over", 4) == 0)
			return;
		process(recvbuf, receive_bytes);
	}
	return;
}

void VideoReceiver::decode(uint8_t* data, size_t len, int64_t ts) {
	if (!decoder) {
		D_LOG("new a video decoder...");
		decoder = new VdDecoder();
		decoder->setPixFmt(AV_PIX_FMT_YUV420P);
		decoder->init();
	}
	decoder->push(data, len, ts);
	std::unique_lock<std::mutex> lk(*mutex);
	auto tempFrame = av_frame_alloc();
	if (decoder) {
		auto decodeRtn = decoder->poll(tempFrame);
		if (decodeRtn < 0) {
			lk.unlock();
			av_frame_free(&tempFrame);
			return;
		}
	}
	else {
		lk.unlock();
		av_frame_free(&tempFrame);
		return;
	}
	if (recvFrameCache) {
		av_frame_unref(recvFrameCache);
	}
	else {
		recvFrameCache = av_frame_alloc();
	}
	av_frame_ref(recvFrameCache, tempFrame);
	//saveYUVFrameToFile(tempFrame, tempFrame->width, tempFrame->height);
	lk.unlock();
	av_frame_free(&tempFrame);
	return;
}

//���rtp parameter: ��������ݣ� �������ݳ��ȣ� ��������ݣ� �������ݵĳ��ȣ� ʱ���
void VideoReceiver::process(char* bufIn, int len) {
	unsigned char recvbuf[1500];
	RtpPacket* p = NULL;
	RtpHeader* rtp_hdr = NULL;

	memcpy(recvbuf, bufIn, len);          //����rtp�� 
	
	p = (RtpPacket*)&recvbuf[0];
	if ((p = (RtpPacket*)malloc(sizeof(RtpPacket))) == NULL)
	{
		printf("RTPpacket_t MMEMORY ERROR\n");
	}
	if ((p->payload = (unsigned char*)malloc(MAXDATASIZE)) == NULL)
	{
		printf("RTPpacket_t payload MMEMORY ERROR\n");
	}

	if ((rtp_hdr = (RtpHeader*)malloc(sizeof(RtpHeader))) == NULL)
	{
		printf("RTP_FIXED_HEADER MEMORY ERROR\n");
	}
	//��������ת���������ֽ���
	rtp_hdr = (RtpHeader*)&recvbuf[0];
	rtp_hdr->seq_no = htons(rtp_hdr->seq_no);
	rtp_hdr->timestamp = htonl(rtp_hdr->timestamp);
	rtp_hdr->ssrc = htonl(rtp_hdr->ssrc);

	//���ֻ��Ҫ�������н���Ļ���ֱ�ӻ�ȡ��payload�����ˣ�����Ҫ���ڽ��н���Ĳ���
	memcpy(p->payload, &recvbuf[12], len - 12);
	p->paylen = len - 12;
	decode(p->payload, p->paylen, p->timestamp);
	memset(recvbuf, 0, 1500);
	free(p->payload);
	free(p);
	return;
}

NALU_t* VideoReceiver::AllocNALU(int buffersize)
{
	NALU_t* n;

	if ((n = (NALU_t*)calloc(1, sizeof(NALU_t))) == NULL)
	{
		printf("AllocNALU Error: Allocate Meory To NALU_t Failed ");
		exit(0);
	}
	return n;
}


int VideoReceiver::saveYUVFrameToFile(AVFrame* frame, int width, int height)
{
	int y, writeError;
	char filename[32];
	static int frameNumber = 0;

	if (fileHandle == NULL)
	{
		return 0;
	}

	/*Writing Y plane data to file.*/
	for (y = 0; y < height; y++)
	{
		writeError = fwrite(frame->data[0] + y * frame->linesize[0], 1, width, fileHandle);
		if (writeError != width)
		{
			I_LOG("Unable to write Y plane data!");
			return 0;
		}
	}

	/*Dividing by 2.*/
	height >>= 1;
	width >>= 1;

	/*Writing U plane data to file.*/
	for (y = 0; y < height; y++)
	{
		writeError = fwrite(frame->data[1] + y * frame->linesize[1], 1, width, fileHandle);
		if (writeError != width)
		{
			I_LOG("Unable to write U plane data!");
			return 0;
		}
	}

	/*Writing V plane data to file.*/
	for (y = 0; y < height; y++)
	{
		writeError = fwrite(frame->data[2] + y * frame->linesize[2], 1, width, fileHandle);
		if (writeError != width)
		{
			I_LOG("Unable to write V plane data!\n");
			return 0;
		}
	}

	frameNumber++;
	return 1;
}


VideoReceiver::~VideoReceiver() {
	I_LOG("VideoReceiver destruct...");
	closeSocket();
	fclose(fileHandle);
}