/*
	* Created by Astro
	* Date : 2022.01.24
	* Descryption:
		*		用于接收该端口的aac音频，目前应该要做到将接收到的rtp流音频保存到本地
*/

#include <iostream>
#include <string>
#include <other/loggerApi.h>
#include <net/NetManager.h>
#include <media/common.h>

struct AudioReceiver
{
	AudioReceiver(int _port);
	~AudioReceiver();

	void setPort(int _port);
	int getPort();

	char* getData();
	void setData(char* _data);
private:
	int port = 0;
	char* data = nullptr;

	void initSocket(int _port);

  void rtp_unpackage_au(char* bufIn, int len);

	inline void writeAdtsHeaders(uint8_t* header, int dataLength, int channel,
		int sample_rate);

	void recvData();
};

