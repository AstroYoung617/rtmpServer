/*
	* Created by Astro
	* Date : 2022.01.13
	* Descryption:
	*		用于管理rtmpClient的动作, 初始化rtpServer, 用于接收rtpServer整合好的音视频数据，发送给rtmp服务器
	*   将接收到的音视频数据以flv的形式发送至rtmp的url上
*/
#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <media/AudioReceiver.h>
#include <media/VideoReceiver.h>
#include <media/VideoSender.h>
#include <other/loggerApi.h>
using std::string;

struct RtmpClient {
	RtmpClient();
	~RtmpClient();
	void printData(int _type);					//type : 1 (audio)  2 (video)
	void setPort(int _type, int _port); //type : 1 (audio)  2 (video)
	void send2Rtmp(int _type);					//type : 1 (audio)  2 (video)  3(audio & video)
private:
	void createAudioCh(int _port);
	void createVideoCh(int _port);
	std::vector<AudioReceiver> AudioChnlVct;
	std::vector<VideoReceiver> VideoChnlVct;

	void getVideoData();
	void sendVideoData();

	std::unordered_map<string, std::thread> threadMap = {};
	//std::unique_ptr<RtpServer> rtpServer = nullptr;
	std::unique_ptr<VideoSender> videoSender = nullptr;
};