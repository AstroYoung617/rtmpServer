/*
	* Created by Astro
	* Date : 2022.01.13
	* Descryption:
	*		���ڹ���rtmpClient�Ķ���, ��ʼ��rtpServer, ���ڽ�������Ƶ���ݣ����͸�rtmp������
	*   �����յ�������Ƶ������flv����ʽ������rtmp��url��
*/
#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <media/AudioReceiver.h>
#include <media/VideoReceiver.h>
#include <media/VideoSender.h>
#include <media/AudioSender.h>
#include <other/loggerApi.h>
#include <media/other/AudioUtil.hpp>
using std::string;

struct RtmpClient {
	RtmpClient();
	~RtmpClient();
	void printData(int _type);					//type : 1 (audio)  2 (video)
	void setPort(int _type, int _port); //type : 1 (audio)  2 (video)
	void send2Rtmp(int _type);					//type : 1 (audio)  2 (video)  3(audio & video)
	void setStart(bool start);
	void setRtmpURL(string _rtmpURL);
private:
	void createAudioCh(int _port);
	void createVideoCh(int _port);
	std::vector<AudioReceiver> AudioRecvVct;
	std::vector<VideoReceiver> VideoRecvVct;
	
	void initRtmp();

	//thread use to get(rtp)&send(rtmp)
	void getVideoData();
	void sendVideoData();

	void getAudioData();
	void sendAudioData();
	//use to manage the threads 
	std::unordered_map<string, std::thread> threadMap = {};

	bool startPush = false;
	/*
	videoReceiver automatic generate
	after maybe modify the unique_ptr videoSender to shared_ptr
	for send data to different rtmpURL.
	*/
	//���audioReceiver����һ�����⣬��д��unique
	std::unique_ptr<AudioReceiver> audioReceiver = nullptr;

	std::unique_ptr<VideoSender> videoSender = nullptr;
	std::unique_ptr<AudioSender> audioSender = nullptr;
	std::shared_ptr<NetManager> netManager = nullptr;



	AVFrame* recvFrameVd = nullptr;
	AVPacket* recvPacketVd = nullptr;
	AVFrame* recvFrameAu = nullptr;

	//���ݸ�videoReceiver��videoSender�Ļ���������
	std::mutex* vdmtx = new std::mutex;
	std::condition_variable* vdcv = new std::condition_variable;

	//���ݸ�audioReceiver��audioSender�Ļ���������
	std::mutex* aumtx = new std::mutex;
	std::condition_variable* aucv = new std::condition_variable;

	//��rtmpClientʹ�õĻ���������
	std::mutex* mtx = new std::mutex;
	std::condition_variable* cv = new std::condition_variable;

	//rtmp 
	string rtmpURL = "rtmp://live-push.bilivideo.com/live-bvc/?streamname=live_98579344_16311849&key=41a6b8b86a64eaeccb3efe3679940c43&schedule=rtmp&pflag=1";
	//string rtmpURL = "E:/testrtmp.flv";
};