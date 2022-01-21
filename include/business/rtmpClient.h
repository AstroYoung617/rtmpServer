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
#include <business/rtpServer.h>
#include <other/loggerApi.h>

struct RtmpClient {
	RtmpClient();
	~RtmpClient();
	void printData(int _type); //type : 1 (audio)  2 (video)
private:
	std::unique_ptr<RtpServer> rtpServer = nullptr;
};