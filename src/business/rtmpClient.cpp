#include <iostream>
#include <business/rtmpClient.h>


RtmpClient::RtmpClient() {
	I_LOG("RtmpClient construct success");
	netManager = std::make_shared<NetManager>();  
	netManager->setRtmpUrl(rtmpURL);
}


void RtmpClient::printData(int _type) {
	if (_type == 1) {
		I_LOG("print audio data");
	}
}

void RtmpClient::createAudioCh(int _port) {
	I_LOG("Create an new AudioChannel port:{}", _port);
	auto audioReceiver = new AudioReceiver(_port);
	AudioRecvVct.push_back(*audioReceiver);
}

void RtmpClient::createVideoCh(int _port) {
	I_LOG("Create an new VideoChannel port:{}", _port);
	auto videoReceiver = new VideoReceiver(_port, vdmtx, vdcv);
	VideoRecvVct.push_back(*videoReceiver);
	if (netManager->rtmpInit(0) == -1) {
		return;
	}

	//videoSender / audioSender init encoder
	videoSender = std::make_unique<VideoSender>(mtx, cv, netManager);
	VideoDefinition vd = VideoDefinition(640, 480);
	videoSender->initEncoder(vd, 20);

	if (netManager->rtmpInit(1) == -1) {
		return;
	}
	std::thread get_video(&RtmpClient::getVideoData, this);
	//std::this_thread::sleep_for(std::chrono::seconds(2));
	std::thread send_video(&RtmpClient::sendVideoData, this);
	threadMap["pollVd"] = std::move(get_video);
	threadMap["pushVd"] = std::move(send_video);
}

void RtmpClient::getVideoData() {
	auto videoRecv = VideoRecvVct.front();
	while (1) {
		//std::unique_lock<std::mutex> lk(*mtx);
		videoRecv.processRecvRtpData();
		//先只用一个AVFrame作为存储，将其传递给videoSender
		recvFrame = videoRecv.getData();
		//lk.unlock();
		//cv->notify_one();
	}
}

void RtmpClient::sendVideoData() {
	while (1) {
		//std::unique_lock<std::mutex> lk(*mtx);
		//cv->wait(lk);
		if (recvFrame && recvFrame->data[0])
			send2Rtmp(2);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		//lk.unlock();
	}
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
		videoSender->sendFrame(recvFrame);
		av_frame_unref(recvFrame);
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