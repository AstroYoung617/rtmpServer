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
#include <media/common.h>
#include <media/decoder/videoDecoder.h>
#include <utility>
#include <mutex>

struct VideoReceiver 
{
	VideoReceiver(int _port, std::mutex* _mutex, std::condition_variable* _vdcv);
	~VideoReceiver();

	void setPort(int _port);
	int getPort();

	uint8_t* getData();
	void setData(uint8_t* _data);

	void initSocket(); 
private:
	//decode ����payload ��parser�����д���nalu header...
	AVFrame* decode(uint8_t* data, size_t len, int64_t ts);

	//���rtp parameter: ��������ݣ� �������ݳ��ȣ� ��������ݣ� �������ݵĳ��ȣ� ʱ���
	void rtp_unpackage_vd(char* bufIn, int len, uint8_t* outData, int& outLen, int64_t& timestamp);

	int saveYUVFrameToFile(AVFrame* frame, int width, int height);

	NALU_t* AllocNALU(int buffersize);
	int port = 0;
	//test 
	uint8_t* outDcData = new uint8_t[4096];
	int outDcLen = 0;
	int64_t ts = 0;
	int recvCount = 0;

	std::mutex* mutex;
	std::condition_variable* vdcv;

	Decoder* decoder = nullptr;
	AVFrame* recvFrameCache = nullptr;



};