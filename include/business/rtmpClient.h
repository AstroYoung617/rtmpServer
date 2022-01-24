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