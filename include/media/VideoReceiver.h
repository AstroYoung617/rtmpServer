/*
	* Created by Astro
	* Date : 2022.01.24
	* Descryption:
		*		用于接收该端口的h264视频，目前应该要做到将rtp的视频流保存到本地
*/

#include <iostream>
#include <string>
#include <other/loggerApi.h>
#include <net/NetManager.h>
#include <media/common.h>
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

	void rtp_unpackage_vd(char* bufIn, int len);

	NALU_t* AllocNALU(int buffersize);
	int port = 0;
	//test 
	char* data = nullptr;
	int recvCount = 0;

	std::mutex* mutex;
	std::condition_variable* vdcv;
};