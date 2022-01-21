/*
	* Created by Astro
	* Date : 2022.01.13
	* Descryption:
		*		���ڹ���rtpServer�Ķ�������ʼ��AudioReceiver��videochannel ��
		*   �����ղ��������aac\h264���ݴ��ݸ�rtmpserver
*/
#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <media/VideoReceiver.h>
#include <media/AudioReceiver.h>
#include <memory>
#include <other/loggerApi.h>
std::string;

struct RtpServer {
	RtpServer();
	~RtpServer();
	std::string getTotalData();

private:
	std::vector<AudioReceiver> AudioChnlVct;
	std::vector<VideoReceiver> VideoChnlVct;
};



