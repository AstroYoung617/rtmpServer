/*
	* Created by Astro
	* Date : 2022.01.13
	* Descryption:
		*		���ڽ��ոö˿ڵ�h264��Ƶ
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