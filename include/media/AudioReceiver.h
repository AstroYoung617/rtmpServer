/*
	* Created by Astro
	* Date : 2022.01.13
	* Descryption:
		*		用于接收该端口的aac音频，目前应该要做到将接收到的rtp流音频保存到本地
*/

#include <iostream>
#include <string>
#include <other/loggerApi.h>

struct AudioReceiver
{
	AudioReceiver();
	~AudioReceiver();

	void setPort(int _port);
	int getPort();

	char* getData();
	void setData(char* _data);
private:
	int port = 0;
	char* data = nullptr;
};

