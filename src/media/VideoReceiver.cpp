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
	char recvbuf[MAXDATASIZE];  //加上头最大传输数据 1500
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
	//socket 操作
	 //加载winsock 无论是客户端还是服务器都需要
	WORD socketVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	//初始化socket资源
	if (WSAStartup(socketVersion, &wsaData) != 0)
	{
		return;
	}

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}//建立socket链接，数据报socket，IPv4协议
	printf("create socket success!\n");

	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = INADDR_ANY;//0.0.0.0不确定地址  
	server_sockaddr.sin_port = htons(port);
	memset(&(server_sockaddr.sin_zero), 0, 8);  //填充0以保持与struct sockaddr同样大小

	if (bind(sockfd, (struct sockaddr*)&server_sockaddr,
		sizeof(struct sockaddr)) < 0)
	{
		perror("ERROR on binding");
		exit(1);
	}
	printf("bind success!\n");

	sin_size = sizeof(struct sockaddr_in);
	printf("waiting for client connection...\n");
	//接收从客户端发来的数据
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
	int total_bytes = 0;                 //当前包传出的数据
	static int total_recved = 0;         //一共传输的数据
	int fwrite_number = 0;               //存入文件的数据长度

	memcpy(recvbuf, bufIn, len);          //复制rtp包 
	//printf("包长度+ rtp头：   = %d\n", len);

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
	//printf("版本号 	: %d\n", rtp_hdr->version);
	p->version = rtp_hdr->version;
	p->padding = rtp_hdr->padding;
	p->extension = rtp_hdr->extension;
	p->cc = rtp_hdr->csrc_len;
	//printf("标志位 	: %d\n", rtp_hdr->marker);
	p->marker = rtp_hdr->marker;
	//printf("负载类型	:%d\n", rtp_hdr->payloadtype);
	p->pt = rtp_hdr->payloadtype;
	//printf("包号   	: %d \n", rtp_hdr->seq_no);
	p->seq_no = rtp_hdr->seq_no;
	//printf("时间戳 	: %d\n", rtp_hdr->timestamp);
	p->timestamp = rtp_hdr->timestamp;
	//printf("帧号   	: %d\n", rtp_hdr->ssrc);
	p->ssrc = rtp_hdr->ssrc;

	//end rtp_payload and rtp_header
	//
	//begin nal_hdr
	if (!(n = AllocNALU(800000)))          //为结构体nalu_t及其成员buf分配空间。返回值为指向nalu_t存储空间的指针
	{
		printf("NALU_t MMEMORY ERROR\n");
	}
	if ((nalu_hdr = (NALU_HEADER*)malloc(sizeof(NALU_HEADER))) == NULL)
	{
		printf("NALU_HEADER MEMORY ERROR\n");
	}

	nalu_hdr = (NALU_HEADER*)&recvbuf[12];                        //网络传输过来的字节序 ，当存入内存还是和文档描述的相反，只要匹配网络字节序和文档描述即可传输正确。
	//printf("forbidden_zero_bit: %d\n", nalu_hdr->F);              //网络传输中的方式为：F->NRI->TYPE.. 内存中存储方式为 TYPE->NRI->F (和nal头匹配)。
	n->forbidden_bit = nalu_hdr->F << 7;                          //内存中的字节序。
	//printf("nal_reference_idc:  %d\n", nalu_hdr->NRI);
	n->nal_reference_idc = nalu_hdr->NRI << 5;
	//printf("nal 负载类型:       %d\n", nalu_hdr->TYPE);
	n->nal_unit_type = nalu_hdr->TYPE;

	//end nal_hdr
	//
	//开始解包
	if (nalu_hdr->TYPE == 0)
	{
		printf("这个包有错误，0无定义\n");
	}
	else if (nalu_hdr->TYPE > 0 && nalu_hdr->TYPE < 24)  //单包
	{
		//printf("当前包为单包\n");
		putc(0x00, poutfile);
		putc(0x00, poutfile);
		putc(0x00, poutfile);
		putc(0x01, poutfile);	//写进起始字节0x00000001
		total_bytes += 4;
		memcpy(p->payload, &recvbuf[13], len - 13);
		p->paylen = len - 13;//接收到的总长度为len 减去 包头的长度13 得到payload的长度
		fwrite(nalu_hdr, 1, 1, poutfile);	//写NAL_HEADER
		total_bytes += 1;
		fwrite_number = fwrite(p->payload, 1, p->paylen, poutfile);	//写NAL数据
		total_bytes = p->paylen;
		//printf("包长度 + nal= %d\n", total_bytes);
	}
	else if (nalu_hdr->TYPE == 24)                    //STAP-A   单一时间的组合包
	{
		//printf("当前包为STAP-A\n");
	}
	else if (nalu_hdr->TYPE == 25)                    //STAP-B   单一时间的组合包
	{
		//printf("当前包为STAP-B\n");
	}
	else if (nalu_hdr->TYPE == 26)                     //MTAP16   多个时间的组合包
	{
		//printf("当前包为MTAP16\n");
	}
	else if (nalu_hdr->TYPE == 27)                    //MTAP24   多个时间的组合包
	{
		//printf("当前包为MTAP24\n");
	}
	else if (nalu_hdr->TYPE == 28)                    //FU-A分片包，解码顺序和传输顺序相同
	{
		if ((fu_ind = (FU_INDICATOR*)malloc(sizeof(FU_INDICATOR))) == NULL)
		{
			//printf("FU_INDICATOR MEMORY ERROR\n");
		}
		if ((fu_hdr = (FU_HEADER*)malloc(sizeof(FU_HEADER))) == NULL)
		{
			//printf("FU_HEADER MEMORY ERROR\n");
		}

		fu_ind = (FU_INDICATOR*)&recvbuf[12];		//分片包用的是FU_INDICATOR而不是NALU_HEADER
		//printf("FU_INDICATOR->F     :%d\n", fu_ind->F);
		n->forbidden_bit = fu_ind->F << 7;
		//printf("FU_INDICATOR->NRI   :%d\n", fu_ind->NRI);
		n->nal_reference_idc = fu_ind->NRI << 5;
		//printf("FU_INDICATOR->TYPE  :%d\n", fu_ind->TYPE);
		n->nal_unit_type = fu_ind->TYPE;

		fu_hdr = (FU_HEADER*)&recvbuf[13];		//FU_HEADER赋值
		//printf("FU_HEADER->S        :%d\n", fu_hdr->S);
		//printf("FU_HEADER->E        :%d\n", fu_hdr->E);
		//printf("FU_HEADER->R        :%d\n", fu_hdr->R);
		//printf("FU_HEADER->TYPE     :%d\n", fu_hdr->TYPE);
		n->nal_unit_type = fu_hdr->TYPE;               //应用的是FU_HEADER的TYPE

		if (rtp_hdr->marker == 1)                      //分片包最后一个包
		{
			//printf("当前包为FU-A分片包最后一个包\n");
			memcpy(p->payload, &recvbuf[14], len - 14);
			p->paylen = len - 14;
			fwrite_number = fwrite(p->payload, 1, p->paylen, poutfile);	//写NAL数据
			total_bytes = p->paylen;
			//printf("包长度 + FU = %d\n", total_bytes);
		}
		else if (rtp_hdr->marker == 0)                 //分片包 但不是最后一个包
		{
			if (fu_hdr->S == 1)                        //分片的第一个包
			{
				unsigned char F;
				unsigned char NRI;
				unsigned char TYPE;
				unsigned char nh;
				//printf("当前包为FU-A分片包第一个包\n");
				putc(0x00, poutfile);
				putc(0x00, poutfile);
				putc(0x00, poutfile);
				putc(0x01, poutfile);				//写起始字节码0x00000001
				total_bytes += 4;

				F = fu_ind->F << 7;
				NRI = fu_ind->NRI << 5;
				TYPE = fu_hdr->TYPE;                                            //应用的是FU_HEADER的TYPE
				//nh = n->forbidden_bit|n->nal_reference_idc|n->nal_unit_type;  //二进制文件也是按 大字节序存储
				nh = F | NRI | TYPE;

				putc(nh, poutfile);				//写NAL HEADER

				total_bytes += 1;
				memcpy(p->payload, &recvbuf[14], len - 14);
				p->paylen = len - 14;
				fwrite_number = fwrite(p->payload, 1, p->paylen, poutfile);	//写NAL数据
				total_bytes = p->paylen;
				//printf("包长度 + FU_First = %d\n", total_bytes);
			}
			else                                      //如果不是第一个包
			{
				//printf("当前包为FU-A分片包\n");
				memcpy(p->payload, &recvbuf[14], len - 14);
				p->paylen = len - 14;
				fwrite_number = fwrite(p->payload, 1, p->paylen, poutfile);	//写NAL数据
				total_bytes = p->paylen;
				//printf("包长度 + FU = %d\n", total_bytes);
			}
		}
	}
	else if (nalu_hdr->TYPE == 29)                //FU-B分片包，解码顺序和传输顺序相同
	{
		if (rtp_hdr->marker == 1)                  //分片包最后一个包
		{
			//printf("当前包为FU-B分片包最后一个包\n");

		}
		else if (rtp_hdr->marker == 0)             //分片包 但不是最后一个包
		{
			//printf("当前包为FU-B分片包\n");
		}
	}
	else
	{
		//printf("这个包有错误，30-31 没有定义\n");
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
	//结束解包
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