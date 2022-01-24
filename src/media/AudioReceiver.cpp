#include <iostream>
#include <media/AudioReceiver.h>
FILE* outFile = nullptr;

AudioReceiver::AudioReceiver(int _port) {
	I_LOG("AudioReceiver struct success");
	initSocket(_port);
}

void AudioReceiver::setPort(int _port) {
	port = _port;
}

int AudioReceiver::getPort() {
	return port;
}

char* AudioReceiver::getData() {
	return data;
}

void AudioReceiver::setData(char* _data) {
	data = _data;
}

void AudioReceiver::initSocket(int _port) {
	char recvbuf[MAXDATASIZE];
	int fd;
	int sin_size;
	struct sockaddr_in server_sockaddr, client_sockaddr;
	char sendbuf[10];

	int recv_bytes = 0; //���յ����ֽ�����recvFrom�����ķ���ֵ������

	//socket ����
	 //����winsock �����ǿͻ��˻��Ƿ���������Ҫ
	WORD socketVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	//��ʼ��socket��Դ
	if (WSAStartup(socketVersion, &wsaData) != 0)
	{
		return;
	}

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}//����socket���ӣ����ݱ�socket��IPv4Э��
	printf("create socket success!\n");

	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = INADDR_ANY;//0.0.0.0��ȷ����ַ  
	server_sockaddr.sin_port = htons(_port);
	memset(&(server_sockaddr.sin_zero), 0, 8);  //���0�Ա�����struct sockaddrͬ����С

	//����ַ��socket���а�
	if (bind(fd, (struct sockaddr*)&server_sockaddr,
		sizeof(struct sockaddr)) < 0)
	{
		perror("ERROR on binding");
		exit(1);
	}
	printf("bind success!\n");

	//��ַ����
	sin_size = sizeof(struct sockaddr_in);
	printf("waiting for client connection...\n");
	while ((recv_bytes = recvfrom(fd, recvbuf, MAXDATASIZE, 0, (struct sockaddr*)&client_sockaddr, &sin_size)) > 0)
	{
		if (strncmp(recvbuf, "over", 4) == 0)
			break;
		outFile = fopen(AAC_FILE, "ab+");
		rtp_unpackage_au(recvbuf, recv_bytes);
		fclose(outFile);
	}
	strcpy(sendbuf, "success");
	sendto(fd, sendbuf, 10, 0, (struct sockaddr*)&client_sockaddr, sin_size);
	closesocket(fd);
	return;
}

inline void AudioReceiver::writeAdtsHeaders(uint8_t* header, int dataLength, int channel,
	int sample_rate) {

	uint8_t profile = 0x02;  // AAC LC
	uint8_t channelCfg = channel;
	uint32_t packetLength = dataLength + 7;
	uint8_t freqIdx;  // 22.05 KHz

	switch (sample_rate) {
	case 96000:
		freqIdx = 0x00;
		break;
	case 88200:
		freqIdx = 0x01;
		break;
	case 64000:
		freqIdx = 0x02;
		break;
	case 48000:
		freqIdx = 0x03;
		break;
	case 44100:
		freqIdx = 0x04;
		break;
	case 32000:
		freqIdx = 0x05;
		break;
	case 24000:
		freqIdx = 0x06;
		break;
	case 22050:
		freqIdx = 0x07;
		break;
	case 16000:
		freqIdx = 0x08;
		break;
	case 12000:
		freqIdx = 0x09;
		break;
	case 11025:
		freqIdx = 0x0A;
		break;
	case 8000:
		freqIdx = 0x0B;
		break;
	case 7350:
		freqIdx = 0x0C;
		break;
	default:
		std::cout << "addADTStoPacket: unsupported sampleRate: {}" << std::endl;
		break;
	}

	header[0] = (uint8_t)0xFF;
	header[1] = (uint8_t)0xF1;

	header[2] =
		(uint8_t)(((profile - 1) << 6) + (freqIdx << 2) + (channelCfg >> 2));
	header[3] = (uint8_t)(((channelCfg & 3) << 6) + (packetLength >> 11));
	header[4] = (uint8_t)((packetLength & 0x07FF) >> 3);
	header[5] = (uint8_t)(((packetLength & 0x0007) << 5) + 0x1F);
	header[6] = (uint8_t)0xFC;
}

void AudioReceiver::rtp_unpackage_au(char* bufIn, int len) {
	unsigned char recvbuf[1500];
	RtpPacket* rtp_pkt = NULL;
	RtpHeader* rtp_hdr = NULL;
	uint8_t* adts_hdr = new uint8_t[7];
	int total_bytes = 0;						//��ǰ������������
	static int total_recved = 0;		//�ܴ��������
	int fwrite_number = 0;					//д���ļ������ݳ���

	memcpy(recvbuf, bufIn, len);
	std::cout << "�ܳ��� = " << len << std::endl;

	rtp_pkt = (RtpPacket*)&recvbuf[0];
	//����rtppacket���ڴ�ռ�
	if ((rtp_pkt = (RtpPacket*)malloc(sizeof(RtpPacket))) == NULL)
	{
		printf("RTPpacket_t MEMORY ERROR\n");
	}
	if ((rtp_pkt->payload = (unsigned char*)malloc(MAXDATASIZE)) == NULL)
	{
		printf("RTPpacket_t payload MEMORY ERROR\n");
	}

	if ((rtp_hdr = (RtpHeader*)malloc(sizeof(RtpHeader))) == NULL)
	{
		printf("RTP_FIXED_HEADER MEMORY ERROR\n");
	}
	printf("receive rtp audio data\n");
	rtp_hdr = (RtpHeader*)&recvbuf[0];
	//printf("�汾�� 	: %d\n", rtp_hdr->version);
	rtp_pkt->version = rtp_hdr->version;
	rtp_pkt->padding = rtp_hdr->padding;
	rtp_pkt->extension = rtp_hdr->extension;
	rtp_pkt->cc = rtp_hdr->csrc_len;
	//printf("��־λ 	: %d\n", rtp_hdr->marker);
	rtp_pkt->marker = rtp_hdr->marker;
	//printf("��������	:%d\n", rtp_hdr->payloadtype);
	rtp_pkt->pt = rtp_hdr->payloadtype;
	//printf("����   	: %d \n", rtp_hdr->seq_no);
	rtp_pkt->seq_no = rtp_hdr->seq_no;
	//printf("ʱ��� 	: %d\n", rtp_hdr->timestamp);
	rtp_pkt->timestamp = rtp_hdr->timestamp;
	//printf("֡��   	: %d\n", rtp_hdr->ssrc);
	rtp_pkt->ssrc = rtp_hdr->ssrc;

	memcpy(rtp_pkt->payload, &recvbuf[16], len - 16);
	rtp_pkt->paylen = len - 16;
	writeAdtsHeaders(adts_hdr, rtp_pkt->paylen, 1, 32000);

	fwrite(adts_hdr, 1, 7, outFile);
	fwrite_number = fwrite(rtp_pkt->payload, 1, rtp_pkt->paylen, outFile);

	printf("paylen = %d, write in filelen = %d\n", rtp_pkt->paylen, fwrite_number);
	//�ͷ���Դ
	memset(recvbuf, 0, 1500);
	free(rtp_pkt->payload);
	free(rtp_pkt);
	delete[] adts_hdr;
	//����
	return;
}

void AudioReceiver::recvData() {

}

AudioReceiver::~AudioReceiver() {
I_LOG("AudioReceiver destruct...");
}