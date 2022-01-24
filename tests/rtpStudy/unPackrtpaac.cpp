#include <iostream>
//����C++���Ե�����⣬��Ҫ���������ͷ�ļ���lib��

//���շ�ʵ���鲥������Ҫ�����鲥�����ܽ��յ��鲥��Ϣ
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
//����ws2tcpip��ʹ��inet_pton
#include <ws2tcpip.h> 
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "wsock32.lib")
#endif
//����ws2ipdef��ʹ��IP_MREQ
#include <ws2ipdef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
//����thread��ʹ��this_thread������ѯ
#include <thread>

#define ADTS_HEADER_LENGTH 7
#define AAC_FILE "./recvAAC.aac"
#define MAXDATASIZE 1500
#define PORT 1234
FILE* outFile = nullptr;

typedef struct
{
	unsigned char version;          	//!< Version, 2 bits, MUST be 0x2
	unsigned char padding;			 	//!< Padding bit, Padding MUST NOT be used
	unsigned char extension;			//!< Extension, MUST be zero
	unsigned char cc;       	   		//!< CSRC count, normally 0 in the absence of RTP mixers 		
	unsigned char marker;			   	//!< Marker bit
	unsigned char pt;			   		//!< 7 bits, Payload Type, dynamically established
	unsigned int seq_no;			   	//!< RTP sequence number, incremented by one for each sent packet 
	unsigned int timestamp;	       //!< timestamp, 27 MHz for H.264
	unsigned int ssrc;			   //!< Synchronization Source, chosen randomly
	unsigned char* payload;      //!< the payload including payload headers
	unsigned int paylen;		   //!< length of payload in bytes
} RtpPacket;

typedef struct
{
	/*  0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|V=2|P|X|  CC   |M|     PT      |       sequence number         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                           timestamp                           |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|           synchronization source (SSRC) identifier            |
	+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
	|            contributing source (CSRC) identifiers             |
	|                             ....                              |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		*/
		//intel ��cpu ��intelΪС���ֽ��򣨵Ͷ˴浽�׵�ַ�� ��������Ϊ����ֽ��򣨸߶˴浽�͵�ַ��
		/*intel ��cpu �� �߶�->csrc_len:4 -> extension:1-> padding:1 -> version:2 ->�Ͷ�
		 ���ڴ��д洢 ��
		 ��->4001���ڴ��ַ��version:2
				 4002���ڴ��ַ��padding:1
			 4003���ڴ��ַ��extension:1
		 ��->4004���ڴ��ַ��csrc_len:4
			 ���紫����� �� �߶�->version:2->padding:1->extension:1->csrc_len:4->�Ͷ�  (Ϊ��ȷ���ĵ�������ʽ)
		 ��������ڴ� ��
		 ��->4001���ڴ��ַ��version:2
				 4002���ڴ��ַ��padding:1
				 4003���ڴ��ַ��extension:1
		 ��->4004���ڴ��ַ��csrc_len:4
		 �����ڴ���� ���߶�->csrc_len:4 -> extension:1-> padding:1 -> version:2 ->�Ͷ� ��
		 ����
		 unsigned char csrc_len:4;        // expect 0
		 unsigned char extension:1;       // expect 1
		 unsigned char padding:1;         // expect 0
		 unsigned char version:2;         // expect 2
		*/
		/* byte 0 */
	unsigned char csrc_len : 4;        /* expect 0 */
	unsigned char extension : 1;       /* expect 1, see RTP_OP below */
	unsigned char padding : 1;         /* expect 0 */
	unsigned char version : 2;         /* expect 2 */
 /* byte 1 */
	unsigned char payloadtype : 7;     /* RTP_PAYLOAD_RTSP */
	unsigned char marker : 1;          /* expect 1 */
 /* bytes 2,3 */
	unsigned int seq_no;
	/* bytes 4-7 */
	unsigned int timestamp;
	/* bytes 8-11 */
	unsigned int ssrc;              /* stream number is used here. */
} RtpHeader;

struct AdtsHeader
{
	unsigned int syncword;  //12 bit ͬ���� '1111 1111 1111'��˵��һ��ADTS֡�Ŀ�ʼ
	unsigned int id;        //1 bit MPEG ��ʾ���� 0 for MPEG-4��1 for MPEG-2
	unsigned int layer;     //2 bit ����'00'
	unsigned int protectionAbsent;  //1 bit 1��ʾû��crc��0��ʾ��crc
	unsigned int profile;           //1 bit ��ʾʹ���ĸ������AAC
	unsigned int samplingFreqIndex; //4 bit ��ʾʹ�õĲ���Ƶ��
	unsigned int privateBit;        //1 bit
	unsigned int channelCfg; //3 bit ��ʾ������
	unsigned int originalCopy;         //1 bit 
	unsigned int home;                  //1 bit 

	/*�����Ϊ�ı�Ĳ�����ÿһ֡����ͬ*/
	unsigned int copyrightIdentificationBit;   //1 bit
	unsigned int copyrightIdentificationStart; //1 bit
	unsigned int aacFrameLength;               //13 bit һ��ADTS֡�ĳ��Ȱ���ADTSͷ��AACԭʼ��
	unsigned int adtsBufferFullness;           //11 bit 0x7FF ˵�������ʿɱ������

	/* number_of_raw_data_blocks_in_frame
	 * ��ʾADTS֡����number_of_raw_data_blocks_in_frame + 1��AACԭʼ֡
	 * ����˵number_of_raw_data_blocks_in_frame == 0
	 * ��ʾ˵ADTS֡����һ��AAC���ݿ鲢����˵û�С�(һ��AACԭʼ֡����һ��ʱ����1024���������������)
	 */
	unsigned int numberOfRawDataBlockInFrame; //2 bit
};

//дadts header��aac����ǰ
inline void writeAdtsHeaders(uint8_t* header, int dataLength, int channel,
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

int openAACFile(char* aacFile) {
	if (NULL == (outFile = fopen(aacFile, "wb"))) {
		std::cout << "open aacFile error!" << std::endl;
		getchar();
	}
	return 1;
}

void rtp_unpackage(char* bufIn, int len) {
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
	rtp_hdr = (RtpHeader*)&recvbuf[0];
	printf("�汾�� 	: %d\n", rtp_hdr->version);
	rtp_pkt->version = rtp_hdr->version;
	rtp_pkt->padding = rtp_hdr->padding;
	rtp_pkt->extension = rtp_hdr->extension;
	rtp_pkt->cc = rtp_hdr->csrc_len;
	printf("��־λ 	: %d\n", rtp_hdr->marker);
	rtp_pkt->marker = rtp_hdr->marker;
	printf("��������	:%d\n", rtp_hdr->payloadtype);
	rtp_pkt->pt = rtp_hdr->payloadtype;
	printf("����   	: %d \n", rtp_hdr->seq_no);
	rtp_pkt->seq_no = rtp_hdr->seq_no;
	printf("ʱ��� 	: %d\n", rtp_hdr->timestamp);
	rtp_pkt->timestamp = rtp_hdr->timestamp;
	printf("֡��   	: %d\n", rtp_hdr->ssrc);
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

int main() {
	char recvbuf[MAXDATASIZE];
  int fd;
	int sin_size;
	struct sockaddr_in server_sockaddr, client_sockaddr;
	char sendbuf[10];

	int recv_bytes = 0; //���յ����ֽ�����recvFrom�����ķ���ֵ������

	openAACFile(AAC_FILE);
	//socket ����
	 //����winsock �����ǿͻ��˻��Ƿ���������Ҫ
	WORD socketVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	//��ʼ��socket��Դ
	if (WSAStartup(socketVersion, &wsaData) != 0)
	{
		return 0;
	}

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}//����socket���ӣ����ݱ�socket��IPv4Э��
	printf("create socket success!\n");

	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = INADDR_ANY;//0.0.0.0��ȷ����ַ  
	server_sockaddr.sin_port = htons(PORT);
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
		rtp_unpackage(recvbuf, recv_bytes);
		fclose(outFile);
	}
	strcpy(sendbuf, "success");
	sendto(fd, sendbuf, 10, 0, (struct sockaddr*)&client_sockaddr, sin_size);
	closesocket(fd);
	return 0;
}


