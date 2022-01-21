/*
	* Created by Astro
	* Date : 2022.01.13
	* Descryption:
		*		用于接收该端口的h264视频
*/

#include <iostream>
#include <string>
#include <other/loggerApi.h>

struct VideoReceiver 
{
	VideoReceiver();
	~VideoReceiver();

	void setPort(int _port);
	int getPort();

	char* getData();
	void setData(char* _data);
private:
	int port = 0;
	char* data = nullptr;
};