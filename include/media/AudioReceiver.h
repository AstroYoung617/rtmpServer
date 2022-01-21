/*
	* Created by Astro
	* Date : 2022.01.13
	* Descryption:
		*		���ڽ��ոö˿ڵ�aac��Ƶ��ĿǰӦ��Ҫ���������յ���rtp����Ƶ���浽����
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

