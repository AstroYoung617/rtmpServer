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