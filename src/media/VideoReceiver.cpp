#include <iostream>
#include <media/VideoReceiver.h>

FILE* poutfile = NULL;

char* outputfilename = "./receive.264";

VideoReceiver::VideoReceiver(int _port, std::mutex* _mutex, std::condition_variable* _vdcv) {
	I_LOG("VideoReceiver struct success");
	port = _port;
	mutex = move(_mutex);
	vdcv = _vdcv;
	//initSocket(_port);
}

void VideoReceiver::setPort(int _port) {
	port = _port;
}

int VideoReceiver::getPort() {
	return port;
}

uint8_t* VideoReceiver::getData() {
	return data;
}

void VideoReceiver::setData(uint8_t* _data) {
	data = _data;
}

void VideoReceiver::initSocket() {
	char recvbuf[MAXDATASIZE];  //����ͷ��������� 1500
	int sockfd;
	//FILE* client_fd;
	int sin_size;
	char sendbuf[BUFFER_SIZE];
	struct sockaddr_in server_sockaddr, client_sockaddr;

	int	receive_bytes = 0;

	if (NULL == (poutfile = fopen(outputfilename, "wb+")))
	{
		printf("Error: Open input file error\n");
		getchar();
	}

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
	//���մӿͻ��˷���������
	while ((receive_bytes = recvfrom(sockfd, recvbuf, MAXDATASIZE, 0, (struct sockaddr*)&client_sockaddr, &sin_size)) > 0)
	{
		std::unique_lock<std::mutex> lck(*mutex);
		if (strncmp(recvbuf, "over", 4) == 0)

		{

			break;

		}
		poutfile = fopen(outputfilename, "ab+");
		rtp_unpackage_vd(recvbuf, receive_bytes);
		fclose(poutfile);
		lck.unlock();
		vdcv->notify_one();
	}
	strcpy(sendbuf, "success");
	sendto(sockfd, sendbuf, BUFFER_SIZE, 0, (struct sockaddr*)&client_sockaddr, sin_size);
	//fclose(client_fd);
	closesocket(sockfd);
	return;
}

void VideoReceiver::rtp_unpackage_vd(char* bufIn, int len) {
	unsigned char recvbuf[1500];
	RtpPacket* p = NULL;
	RtpHeader* rtp_hdr = NULL;
	NALU_HEADER* nalu_hdr = NULL;
	NALU_t* n = NULL;
	FU_INDICATOR* fu_ind = NULL;
	FU_HEADER* fu_hdr = NULL;
	int total_bytes = 0;                 //��ǰ������������
	static int total_recved = 0;         //һ�����������
	int fwrite_number = 0;               //�����ļ������ݳ���

	memcpy(recvbuf, bufIn, len);          //����rtp�� 
	//printf("������+ rtpͷ��   = %d\n", len);

	//
	//begin rtp_payload and rtp_header

	
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
	I_LOG("receive rtp video data, recvCount : {}", recvCount++);
	rtp_hdr = (RtpHeader*)&recvbuf[0];
	//printf("�汾�� 	: %d\n", rtp_hdr->version);
	p->version = rtp_hdr->version;
	p->padding = rtp_hdr->padding;
	p->extension = rtp_hdr->extension;
	p->cc = rtp_hdr->csrc_len;
	//printf("��־λ 	: %d\n", rtp_hdr->marker);
	p->marker = rtp_hdr->marker;
	//printf("��������	:%d\n", rtp_hdr->payloadtype);
	p->pt = rtp_hdr->payloadtype;
	//printf("����   	: %d \n", rtp_hdr->seq_no);
	p->seq_no = rtp_hdr->seq_no;
	//printf("ʱ��� 	: %d\n", rtp_hdr->timestamp);
	p->timestamp = rtp_hdr->timestamp;
	//printf("֡��   	: %d\n", rtp_hdr->ssrc);
	p->ssrc = rtp_hdr->ssrc;

	//end rtp_payload and rtp_header
	//
	//begin nal_hdr
	if (!(n = AllocNALU(800000)))          //Ϊ�ṹ��nalu_t�����Աbuf����ռ䡣����ֵΪָ��nalu_t�洢�ռ��ָ��
	{
		printf("NALU_t MMEMORY ERROR\n");
	}
	if ((nalu_hdr = (NALU_HEADER*)malloc(sizeof(NALU_HEADER))) == NULL)
	{
		printf("NALU_HEADER MEMORY ERROR\n");
	}

	nalu_hdr = (NALU_HEADER*)&recvbuf[12];                        //���紫��������ֽ��� ���������ڴ滹�Ǻ��ĵ��������෴��ֻҪƥ�������ֽ�����ĵ��������ɴ�����ȷ��
	//printf("forbidden_zero_bit: %d\n", nalu_hdr->F);              //���紫���еķ�ʽΪ��F->NRI->TYPE.. �ڴ��д洢��ʽΪ TYPE->NRI->F (��nalͷƥ��)��
	n->forbidden_bit = nalu_hdr->F << 7;                          //�ڴ��е��ֽ���
	//printf("nal_reference_idc:  %d\n", nalu_hdr->NRI);
	n->nal_reference_idc = nalu_hdr->NRI << 5;
	//printf("nal ��������:       %d\n", nalu_hdr->TYPE);
	n->nal_unit_type = nalu_hdr->TYPE;

	//end nal_hdr
	//
	//��ʼ���
	if (nalu_hdr->TYPE == 0)
	{
		printf("������д���0�޶���\n");
	}
	else if (nalu_hdr->TYPE > 0 && nalu_hdr->TYPE < 24)  //����
	{
		//printf("��ǰ��Ϊ����\n");
		putc(0x00, poutfile);
		putc(0x00, poutfile);
		putc(0x00, poutfile);
		putc(0x01, poutfile);	//д����ʼ�ֽ�0x00000001
		total_bytes += 4;
		memcpy(p->payload, &recvbuf[13], len - 13);
		p->paylen = len - 13;//���յ����ܳ���Ϊlen ��ȥ ��ͷ�ĳ���13 �õ�payload�ĳ���
		fwrite(nalu_hdr, 1, 1, poutfile);	//дNAL_HEADER
		total_bytes += 1;
		fwrite_number = fwrite(p->payload, 1, p->paylen, poutfile);	//дNAL����
		total_bytes = p->paylen;
		//printf("������ + nal= %d\n", total_bytes);
	}
	else if (nalu_hdr->TYPE == 24)                    //STAP-A   ��һʱ�����ϰ�
	{
		//printf("��ǰ��ΪSTAP-A\n");
	}
	else if (nalu_hdr->TYPE == 25)                    //STAP-B   ��һʱ�����ϰ�
	{
		//printf("��ǰ��ΪSTAP-B\n");
	}
	else if (nalu_hdr->TYPE == 26)                     //MTAP16   ���ʱ�����ϰ�
	{
		//printf("��ǰ��ΪMTAP16\n");
	}
	else if (nalu_hdr->TYPE == 27)                    //MTAP24   ���ʱ�����ϰ�
	{
		//printf("��ǰ��ΪMTAP24\n");
	}
	else if (nalu_hdr->TYPE == 28)                    //FU-A��Ƭ��������˳��ʹ���˳����ͬ
	{
		if ((fu_ind = (FU_INDICATOR*)malloc(sizeof(FU_INDICATOR))) == NULL)
		{
			//printf("FU_INDICATOR MEMORY ERROR\n");
		}
		if ((fu_hdr = (FU_HEADER*)malloc(sizeof(FU_HEADER))) == NULL)
		{
			//printf("FU_HEADER MEMORY ERROR\n");
		}

		fu_ind = (FU_INDICATOR*)&recvbuf[12];		//��Ƭ���õ���FU_INDICATOR������NALU_HEADER
		//printf("FU_INDICATOR->F     :%d\n", fu_ind->F);
		n->forbidden_bit = fu_ind->F << 7;
		//printf("FU_INDICATOR->NRI   :%d\n", fu_ind->NRI);
		n->nal_reference_idc = fu_ind->NRI << 5;
		//printf("FU_INDICATOR->TYPE  :%d\n", fu_ind->TYPE);
		n->nal_unit_type = fu_ind->TYPE;

		fu_hdr = (FU_HEADER*)&recvbuf[13];		//FU_HEADER��ֵ
		//printf("FU_HEADER->S        :%d\n", fu_hdr->S);
		//printf("FU_HEADER->E        :%d\n", fu_hdr->E);
		//printf("FU_HEADER->R        :%d\n", fu_hdr->R);
		//printf("FU_HEADER->TYPE     :%d\n", fu_hdr->TYPE);
		n->nal_unit_type = fu_hdr->TYPE;               //Ӧ�õ���FU_HEADER��TYPE

		if (rtp_hdr->marker == 1)                      //��Ƭ�����һ����
		{
			//printf("��ǰ��ΪFU-A��Ƭ�����һ����\n");
			memcpy(p->payload, &recvbuf[14], len - 14);
			p->paylen = len - 14;
			fwrite_number = fwrite(p->payload, 1, p->paylen, poutfile);	//дNAL����
			total_bytes = p->paylen;
			//printf("������ + FU = %d\n", total_bytes);
		}
		else if (rtp_hdr->marker == 0)                 //��Ƭ�� ���������һ����
		{
			if (fu_hdr->S == 1)                        //��Ƭ�ĵ�һ����
			{
				unsigned char F;
				unsigned char NRI;
				unsigned char TYPE;
				unsigned char nh;
				//printf("��ǰ��ΪFU-A��Ƭ����һ����\n");
				putc(0x00, poutfile);
				putc(0x00, poutfile);
				putc(0x00, poutfile);
				putc(0x01, poutfile);				//д��ʼ�ֽ���0x00000001
				total_bytes += 4;

				F = fu_ind->F << 7;
				NRI = fu_ind->NRI << 5;
				TYPE = fu_hdr->TYPE;                                            //Ӧ�õ���FU_HEADER��TYPE
				//nh = n->forbidden_bit|n->nal_reference_idc|n->nal_unit_type;  //�������ļ�Ҳ�ǰ� ���ֽ���洢
				nh = F | NRI | TYPE;

				putc(nh, poutfile);				//дNAL HEADER

				total_bytes += 1;
				memcpy(p->payload, &recvbuf[14], len - 14);
				p->paylen = len - 14;
				fwrite_number = fwrite(p->payload, 1, p->paylen, poutfile);	//дNAL����
				total_bytes = p->paylen;
				//printf("������ + FU_First = %d\n", total_bytes);
			}
			else                                      //������ǵ�һ����
			{
				//printf("��ǰ��ΪFU-A��Ƭ��\n");
				memcpy(p->payload, &recvbuf[14], len - 14);
				p->paylen = len - 14;
				fwrite_number = fwrite(p->payload, 1, p->paylen, poutfile);	//дNAL����
				total_bytes = p->paylen;
				//printf("������ + FU = %d\n", total_bytes);
			}
		}
	}
	else if (nalu_hdr->TYPE == 29)                //FU-B��Ƭ��������˳��ʹ���˳����ͬ
	{
		if (rtp_hdr->marker == 1)                  //��Ƭ�����һ����
		{
			//printf("��ǰ��ΪFU-B��Ƭ�����һ����\n");

		}
		else if (rtp_hdr->marker == 0)             //��Ƭ�� ���������һ����
		{
			//printf("��ǰ��ΪFU-B��Ƭ��\n");
		}
	}
	else
	{
		//printf("������д���30-31 û�ж���\n");
	}
	total_recved += total_bytes;
	//printf("total_recved = %d\n", total_recved);
	memset(recvbuf, 0, 1500);
	free(p->payload);
	free(p);
	//freeNalu
	if (n)
	{
		free(n);
	}
	//�������
	//
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

VideoReceiver::~VideoReceiver() {
	I_LOG("VideoReceiver destruct...");
}