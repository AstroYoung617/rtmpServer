/*
	* Created by Astro
	* Date : 2022.01.13
	* Descryption:
	*		���ڹ���rtmpClient�Ķ���, ��ʼ��rtpServer, ���ڽ���rtpServer���Ϻõ�����Ƶ���ݣ����͸�rtmp������
	*   �����յ�������Ƶ������flv����ʽ������rtmp��url��
*/
#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <business/rtpServer.h>
#include <other/loggerApi.h>

struct RtmpClient {
	RtmpClient();
	~RtmpClient();
	void printData(int _type); //type : 1 (audio)  2 (video)
private:
	std::unique_ptr<RtpServer> rtpServer = nullptr;
};