#include <iostream>
#include <business/rtmpClient.h>

std::mutex* vdmtx = new std::mutex;
std::condition_variable* vdcv = new std::condition_variable;

RtmpClient::RtmpClient() {
	I_LOG("RtmpClient construct success");
	videoSender = std::make_unique<VideoSender>(vdmtx, vdcv);
}

void RtmpClient::printData(int _type) {
	if (_type == 1) {
		I_LOG("print audio data");
	}
}

void RtmpClient::createAudioCh(int _port) {
	I_LOG("Create an new AudioChannel port:{}", _port);
	auto audioReceiver = new AudioReceiver(_port);
	AudioChnlVct.push_back(*audioReceiver);
}

void RtmpClient::createVideoCh(int _port) {
	I_LOG("Create an new VideoChannel port:{}", _port);
	auto videoReceiver = new VideoReceiver(_port, vdmtx, vdcv);
	VideoChnlVct.push_back(*videoReceiver);
	std::thread get_video(&RtmpClient::getVideoData, this);
	//std::this_thread::sleep_for(std::chrono::seconds(2));
	//std::thread send_video(&RtmpClient::sendVideoData, this);
	threadMap["pollVd"] = std::move(get_video);
	//threadMap["pushVd"] = std::move(send_video);
}

void RtmpClient::getVideoData() {
	auto videoChnl = VideoChnlVct.front();
	videoChnl.initSocket();
}

void RtmpClient::sendVideoData() {
	videoSender->sendRtmpData();
}

void RtmpClient::setPort(int _type, int _port) {
	//如果port为0，则认为没有该通道
	if (!_port) {
		if (_type == 1) {
			I_LOG("no audio channel");
		}
		else if (_type == 2) {
			I_LOG("no video channel");
		}
	}
	else {
		if (_type == 1) {
			createAudioCh(_port);
		}
		else if (_type == 2) {
			createVideoCh(_port);
		}
	}
}

void RtmpClient::send2Rtmp(int _type) {
	if (_type == 1) {
		;
	}
	else if (_type == 2) {
		videoSender->sendRtmpData();
	}
	else if (_type == 3) {
		;
	}
}

RtmpClient::~RtmpClient() {
	I_LOG("RtmpClient destruct...");
	if (threadMap["pushVd"].joinable())
		threadMap["pushVd"].join();
	else if (threadMap["pollVd"].joinable())
		threadMap["pollVd"].join();
}