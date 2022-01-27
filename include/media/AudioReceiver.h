/*
	* Created by Astro
	* Date : 2022.01.24
	* Descryption:
		*		���ڽ��ոö˿ڵ�aac��Ƶ��ĿǰӦ��Ҫ���������յ���rtp����Ƶ���浽����
*/

#include <iostream>
#include <string>
#include <other/loggerApi.h>
#include <net/NetManager.h>
#include <media/other/common.h>
#include <media/decoder/AudioDecoder.h>
#include <media/other/AudioUtil.hpp>

struct AudioReceiver
{
	AudioReceiver(int _port, int _alSoundFormat, CoderInfo _decoderInfo);
	~AudioReceiver();

	void setPort(int _port);
	int getPort();

	AVFrame* getData();
	void setData(char* _data);

	//������յ���Rtp����
	void processRecvRtpData();

private:
	int port = 0;
	char* data = nullptr;

	void initSocket();

  void process(char* bufIn, int len);

	inline void writeAdtsHeaders(uint8_t* header, int dataLength, int channel,
		int sample_rate);

	//network
	char recvbuf[MAXDATASIZE];
	int fd;
	int sin_size;
	struct sockaddr_in server_sockaddr, client_sockaddr;
	char sendbuf[10];

	int recv_bytes = 0; //���յ����ֽ�����recvFrom�����ķ���ֵ������

		// ��Ƶ��װ��ʽ
	AudioInfo info, outInfo;
	CodecType decoderCodecType;
	CoderInfo decoderInfo;

	//decoder
	std::unique_ptr<AuDecoder> decoder;

	AVFrame* recvFrame = nullptr;

};

