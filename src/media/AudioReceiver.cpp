#include <iostream>
#include <media/AudioReceiver.h>
FILE* outFile = fopen("E:/common/rtmpServer.pcm", "wb");

AudioReceiver::AudioReceiver(int _port, int _alSoundFormat, CoderInfo _decoderInfo) {
	I_LOG("AudioReceiver struct success");
	port = _port;
	initSocket();
	this->decoderInfo = _decoderInfo;
	// init netEq & audioInfo
	AudioInfo in;
	in.sample_rate = decoderInfo.inSampleRate;
	in.channels = decoderInfo.inChannels;
	in.sample_fmt = (AVSampleFormat)(decoderInfo.inFormate);
	in.channel_layout = av_get_default_channel_layout(decoderInfo.inChannels);
	if (decoderInfo.cdtype == CodecType::PCMA) {
		in.pcmaTimeSeg = 30;
	}
	AudioInfo out;
	out.sample_rate = decoderInfo.outSampleRate;
	out.channels = decoderInfo.outChannels;
	out.sample_fmt = (AVSampleFormat)(decoderInfo.outFormate);
	out.channel_layout = av_get_default_channel_layout(decoderInfo.outChannels);

	decoderCodecType = decoderInfo.cdtype;

	info.setInfo(decoderInfo.inSampleRate, (AVSampleFormat)decoderInfo.inFormate,
		decoderInfo.inChannels);
	outInfo.setInfo(decoderInfo.outSampleRate, (AVSampleFormat)decoderInfo.outFormate,
		decoderInfo.outChannels);
	decoder = std::make_unique<AuDecoder>();
	decoder->init(in, out, decoderCodecType, 0);
}

void AudioReceiver::setPort(int _port) {
	port = _port;
}

int AudioReceiver::getPort() {
	return port;
}

AVFrame* AudioReceiver::getData() {
	return recvFrame;
}

void AudioReceiver::setData(char* _data) {
	data = _data;
}

void AudioReceiver::initSocket() {
	//socket 操作
	 //加载winsock 无论是客户端还是服务器都需要
	WORD socketVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	//初始化socket资源
	if (WSAStartup(socketVersion, &wsaData) != 0)
	{
		return;
	}

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}//建立socket链接，数据报socket，IPv4协议
	printf("create socket success!\n");

	server_sockaddr.sin_family = AF_INET;
	server_sockaddr.sin_addr.s_addr = INADDR_ANY;//0.0.0.0不确定地址  
	server_sockaddr.sin_port = htons(port);
	memset(&(server_sockaddr.sin_zero), 0, 8);  //填充0以保持与struct sockaddr同样大小

	//将地址和socket进行绑定
	if (bind(fd, (struct sockaddr*)&server_sockaddr,
		sizeof(struct sockaddr)) < 0)
	{
		perror("ERROR on binding");
		exit(1);
	}
	printf("bind success!\n");

	//地址长度
	sin_size = sizeof(struct sockaddr_in);
	printf("waiting for client connection...\n");
	return;
}

void AudioReceiver::processRecvRtpData() {
	//接收从客户端发来的数据
	RtpPacket* p = NULL;
	RtpHeader* rtp_hdr = NULL;

	if ((recv_bytes = recvfrom(fd, recvbuf, MAXDATASIZE, 0, (struct sockaddr*)&client_sockaddr, &sin_size)) > 0)
	{
		if (strncmp(recvbuf, "over", 4) == 0)
			return;
		process(recvbuf, recv_bytes);
	}
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

void AudioReceiver::process(char* bufIn, int len) {
	unsigned char recvbuf[1500];
	RtpPacket* rtp_pkt = NULL;
	RtpHeader* rtp_hdr = NULL;
	uint8_t* adts_hdr = new uint8_t[7];
	memcpy(recvbuf, bufIn, len);
	std::cout << "总长度 = " << len << std::endl;

	rtp_pkt = (RtpPacket*)&recvbuf[0];
	//分配rtppacket的内存空间
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
	////将主机数转换成网络字节序
	rtp_hdr = (RtpHeader*)&recvbuf[0];
	rtp_hdr->seq_no = htons(rtp_hdr->seq_no);
	rtp_hdr->timestamp = htonl(rtp_hdr->timestamp);
	rtp_hdr->ssrc = htonl(rtp_hdr->ssrc);

	rtp_hdr = (RtpHeader*)&recvbuf[0];
	//printf("版本号 	: %d\n", rtp_hdr->version);
	rtp_pkt->version = rtp_hdr->version;
	rtp_pkt->padding = rtp_hdr->padding;
	rtp_pkt->extension = rtp_hdr->extension;
	rtp_pkt->cc = rtp_hdr->csrc_len;
	//printf("标志位 	: %d\n", rtp_hdr->marker);
	rtp_pkt->marker = rtp_hdr->marker;
	//printf("负载类型	:%d\n", rtp_hdr->payloadtype);
	rtp_pkt->pt = rtp_hdr->payloadtype;
	//printf("包号   	: %d \n", rtp_hdr->seq_no);
	rtp_pkt->seq_no = rtp_hdr->seq_no;
	printf("时间戳 	: %d\n", rtp_hdr->timestamp);
	rtp_pkt->timestamp = rtp_hdr->timestamp;
	//printf("帧号   	: %d\n", rtp_hdr->ssrc);
	rtp_pkt->ssrc = rtp_hdr->ssrc;

	memcpy(rtp_pkt->payload, &recvbuf[16], len - 16);
	rtp_pkt->paylen = len - 16;
	writeAdtsHeaders(adts_hdr, rtp_pkt->paylen, 1, 32000);
	//构造payload
	uint8_t* payload = new uint8_t[7 + rtp_pkt->paylen];
	for (int i = 0; i < 7; i++) {
			payload[i] = adts_hdr[i];
	}
	for (int i = 0; i < rtp_pkt->paylen; i++) {
		payload[i + 7] = rtp_pkt->payload[i];
	}
	//TODO decode
	int frame_size;
	uint8_t* adts_buff;
	//如果是aac格式
	frame_size = 0;
	adts_buff = new uint8_t[3000];
	std::pair<int&, uint8_t*> demuxAudioFrame(frame_size, adts_buff);
	//解封装
	decoder->demux(payload, demuxAudioFrame);
	decoder->addPacket(demuxAudioFrame.second, demuxAudioFrame.first, rtp_pkt->timestamp, rtp_pkt->seq_no);
	//decoder->addPacket(payload, rtp_pkt->paylen + 7, rtp_pkt->timestamp, rtp_pkt->seq_no);
	recvFrame = decoder->getFrame();
	if (recvFrame && recvFrame->data[0]) {
		len = recvFrame->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(recvFrame->format)) * recvFrame->channels;
		uint8_t* data = new uint8_t[len + 1];
		fwrite(recvFrame->data[0], 1, len, outFile);
	}
	//释放资源
	memset(recvbuf, 0, 1500);
	free(rtp_pkt->payload);
	free(rtp_pkt);
	//结束
	return;
}


AudioReceiver::~AudioReceiver() {
I_LOG("AudioReceiver destruct...");
}