/*
	* Created by Astro
	* Date : 2022.01.13
	* Descryption:
		*		用于管理rtpServer的动作，初始化AudioReceiver，videochannel ，
		*   将接收并解析后的aac\h264数据传递给rtmpserver
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



