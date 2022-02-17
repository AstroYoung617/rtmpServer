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
#include <deque>
#include <map>
#include <media/AudioReceiver.h>
#include <media/VideoReceiver.h>
#include <media/VideoSender.h>
#include <media/AudioSender.h>
#include <other/loggerApi.h>
#include <media/other/AudioUtil.hpp>
using std::string;
using std::deque;

struct RtmpClient {
	RtmpClient();
	~RtmpClient();
	void printData(int _type);					//type : 1 (audio)  2 (video)
	void setPort(int _type, int _port); //type : 1 (audio)  2 (video)
	void send2Rtmp(int _type);					//type : 1 (audio)  2 (video)  3(audio & video)
	void setStart(bool start);
	void setURL(string URL);
private:
	void createAudioCh(int _port);
	void createVideoCh(int _port);
	std::vector<AudioReceiver> AudioRecvVct;
	std::deque<VideoReceiver> VideoRecvVct;
	std::deque<VideoReceiver> VideoRecvVct1;
	std::map<int, deque<VideoReceiver>> VideoRecvMap;
	
	void initRtmp();

	//thread use to get(rtp)&send(rtmp)
	void getVideoData(int _port);
	void sendVideoData();

	void getAudioData();
	void sendAudioData();

	void sendFakeVideoData();

	//use to test yuv combine
	AVFrame* combineYUV(AVFrame* pFrame);

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
	std::shared_ptr<VideoReceiver> videoReceiver = nullptr;
	std::shared_ptr<VideoReceiver> videoReceiver1 = nullptr;

	std::unique_ptr<VideoSender> videoSender = nullptr;
	std::unique_ptr<AudioSender> audioSender = nullptr;
	std::shared_ptr<NetManager> netManager = nullptr;



	//AVFrame* recvFrameVd = nullptr;
	AVFrame* recvFrameAu = nullptr;

	std::map<int, deque<AVFrame*>> recvVdFrameMap;  //ʹ�ö˿ںͶ������Ӧ�ķ�ʽ���洢���յ���֡

	deque<AVFrame*> recvVdFrameDq;
	deque<AVFrame*> recvAuFrameDq;

	//���ݸ�videoReceiver��videoSender�Ļ���������
	std::mutex* vdmtx = new std::mutex;
	std::condition_variable* vdcv = new std::condition_variable;

	std::mutex* aumtx = new std::mutex;
	std::condition_variable* aucv = new std::condition_variable;

	//��rtmpClientʹ�õĻ���������
	std::mutex* mtx = new std::mutex;
	std::condition_variable* cv = new std::condition_variable;

	//rtmp 
	string rtmpURL = "";
	//string rtmpURL = "rtmp://live-push.bilivideo.com/live-bvc/?streamname=live_98579344_16311849&key=db3d9db495ef7a7852dcc1544a267ba7&schedule=rtmp&pflag=1";
	//string rtmpURL = "E:/testrtmp.flv";
};