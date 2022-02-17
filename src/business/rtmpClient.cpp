#include <iostream>
#include <business/rtmpClient.h>
#define FRAMEWIDTH 640
#define FRAMEHEIGTH 480

RtmpClient::RtmpClient() {
	I_LOG("RtmpClient construct success");
	netManager = std::make_shared<NetManager>();  
}


void RtmpClient::printData(int _type) {
	if (_type == 1) {
		I_LOG("print audio data");
	}
}

void RtmpClient::createAudioCh(int _port) {
	I_LOG("Create an new AudioChannel port:{}", _port);
	//decoder info
	CoderInfo decoderinfo;
	decoderinfo.outChannels = 1;
	decoderinfo.outSampleRate = 32000;
	decoderinfo.outFormate = MyAVSampleFormat::AV_SAMPLE_FMT_S16;

	decoderinfo.inChannels = 1;
	decoderinfo.inSampleRate = 32000;
	decoderinfo.inFormate = MyAVSampleFormat::AV_SAMPLE_FMT_FLTP;
	decoderinfo.cdtype = CodecType::AAC;
	decoderinfo.muxType = MuxType::ADTS;
	audioReceiver = std::make_unique<AudioReceiver>(_port, 0x1101, decoderinfo);
	//AudioRecvVct.push_back(*audioReceiver);

	//encoder info
	std::thread get_audio(&RtmpClient::getAudioData, this);
	threadMap["pollAu"] = std::move(get_audio);
}

void RtmpClient::createVideoCh(int _port) {
	I_LOG("Create an new VideoChannel port:{}", _port);
	if (!videoReceiver) {
		videoReceiver = std::make_shared<VideoReceiver>(_port, vdmtx, vdcv);
		VideoRecvVct.push_back(*videoReceiver);
		VideoRecvMap.insert(std::make_pair(_port, VideoRecvVct));
		auto vdDq = std::deque<AVFrame*>();
		recvVdFrameMap.insert({ _port, vdDq });
		std::thread get_video(&RtmpClient::getVideoData, this, _port);
		//发送黑色的frame测试音频是否有问题
		//std::thread send_video(&RtmpClient::sendFakeVideoData, this);
		threadMap["pollVd" + std::to_string(_port)] = std::move(get_video);
	}
	else {
		videoReceiver1 = std::make_shared<VideoReceiver>(_port, vdmtx, vdcv);
		VideoRecvVct1.push_back(*videoReceiver);
		VideoRecvMap.insert(std::make_pair(_port, VideoRecvVct1));
		auto vdDq = std::deque<AVFrame*>();
		recvVdFrameMap.insert({ _port, vdDq });
		std::thread get_video1(&RtmpClient::getVideoData, this, _port);
		////发送黑色的frame测试音频是否有问题
		////std::thread send_video(&RtmpClient::sendFakeVideoData, this);
		threadMap["pollVd" + std::to_string(_port)] = std::move(get_video1);
	}
}

void RtmpClient::getAudioData() {
	//auto audioRecv = AudioRecvVct.front();
	//while (1) {
	//	audioRecv.processRecvRtpData();
	//	//先只用一个AVFrame作为存储，将其传递给videoSender
	//	recvFrameAu = audioRecv.getData();
	//	//TODO 之后将多个videoRecv的data进行拼接后传给videoSender
	//}
	// 
	while (1) {
		if (!startPush) {
			std::unique_lock<std::mutex> lk(*mtx);
			cv->wait(lk);
		}
		audioReceiver->processRecvRtpData();
		//先只用一个AVFrame作为存储，将其传递给videoSender
		recvFrameAu = audioReceiver->getData();
		if (recvFrameAu && recvFrameAu->data[0])
			recvAuFrameDq.push_back(recvFrameAu);
		//TODO 之后将多个videoRecv的data进行拼接后传给videoSender
		//lk.unlock();
		aucv->notify_one();
	}
}

void RtmpClient::sendAudioData() {
	while (1) {
		if (!startPush) {
			std::unique_lock<std::mutex> lk(*mtx);
			cv->wait(lk);
		}
		std::unique_lock<std::mutex> lck(*aumtx);
		//aucv->wait(lck);
		//if (audioSender->lastPts > videoSender->lastPts)
		//	std::this_thread::sleep_for(std::chrono::milliseconds(audioSender->lastPts - videoSender->lastPts));
		if (recvAuFrameDq.size() && recvAuFrameDq.front()->data[0])
			send2Rtmp(1);
		//std::this_thread::sleep_for(std::chrono::milliseconds(20));
		lck.unlock();
	}
}
 void RtmpClient::getVideoData(int _port) {
	//auto videoRecv = VideoRecvMap.find(_port)->second.front();
	while (1) {
		if (!startPush) {
			std::unique_lock<std::mutex> lk(*mtx);
			cv->wait(lk);
		}
		//std::unique_lock<std::mutex> lk(*vdmtx);
		VideoRecvMap.find(_port)->second.front().processRecvRtpData();
		//videoReceiver->processRecvRtpData();
		auto recvFrameVd = VideoRecvMap.find(_port)->second.front().getData();
		//auto recvFrameVd = videoReceiver->getData();
		if (recvFrameVd && recvFrameVd->data[0])
			recvVdFrameMap.find(_port)->second.push_back(recvFrameVd);
			//recvVdFrameDq.push_back(recvFrameVd);
		//TODO 之后将多个videoRecv的data进行拼接后传给videoSender
		//lk.unlock();
		//vdcv->notify_one();
	}
}

void RtmpClient::sendVideoData() {
	while (1) {
		if (!startPush) {
			std::unique_lock<std::mutex> lk(*mtx);
			cv->wait(lk);
		}
		std::unique_lock<std::mutex> lck(*vdmtx);
		//vdcv->wait(lck);
		//vdcv->wait_for(lck, std::chrono::milliseconds(45));
		//std::this_thread::sleep_for(std::chrono::milliseconds(20));
		if (videoSender->lastPts > audioSender->lastPts)
			std::this_thread::sleep_for(std::chrono::milliseconds(videoSender->lastPts - audioSender->lastPts));
		//if (recvVdFrameDq.size() && recvVdFrameDq.front()->data[0]) 
		if (recvVdFrameMap.find(2222)->second.size()&& recvVdFrameMap.find(2222)->second.front()->data[0])
			send2Rtmp(2);
		//std::this_thread::sleep_for(std::chrono::milliseconds(10));
	  lck.unlock();
	}
}

void RtmpClient::sendFakeVideoData() {
	while (1) {
		if (!startPush) {
			std::unique_lock<std::mutex> lk(*mtx);
			cv->wait(lk);
		}
		//vdcv->wait_for(lck, std::chrono::milliseconds(45));
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		AVFrame* pDstFrame = av_frame_alloc();
		int nDstSize = avpicture_get_size(AV_PIX_FMT_YUV420P, FRAMEWIDTH, FRAMEHEIGTH);
		uint8_t* dstbuf = new uint8_t[nDstSize];
		avpicture_fill((AVPicture*)pDstFrame, dstbuf, AV_PIX_FMT_YUV420P, FRAMEWIDTH, FRAMEHEIGTH);

		pDstFrame->width = FRAMEWIDTH;
		pDstFrame->height = FRAMEHEIGTH;
		pDstFrame->format = AV_PIX_FMT_YUV420P;

		//将预先分配的AVFrame图像背景数据设置为黑色背景  
		memset(pDstFrame->data[0], 0, FRAMEWIDTH * FRAMEHEIGTH);
		memset(pDstFrame->data[1], 0x80, FRAMEWIDTH * FRAMEHEIGTH / 4);
		memset(pDstFrame->data[2], 0x80, FRAMEWIDTH * FRAMEHEIGTH / 4);
		if (videoSender->lastPts > audioSender->lastPts)
			std::this_thread::sleep_for(std::chrono::milliseconds(videoSender->lastPts - audioSender->lastPts));
		videoSender->sendFrame(pDstFrame);
		av_frame_free(&pDstFrame);
		delete[] dstbuf;

	}
}

void RtmpClient::setStart(bool _start) {
	if (_start) {
		if (netManager->rtmpInit(0) == -1) {
			return;
		}
		CoderInfo encoderinfo;
		encoderinfo.inChannels = 1;
		encoderinfo.inSampleRate = 32000;
		encoderinfo.inFormate = MyAVSampleFormat::AV_SAMPLE_FMT_S16;

		encoderinfo.outChannels = 1;
		encoderinfo.outSampleRate = 32000;
		encoderinfo.outFormate = MyAVSampleFormat::AV_SAMPLE_FMT_FLTP;
		encoderinfo.cdtype = CodecType::AAC;
		encoderinfo.muxType = MuxType::ADTS;
		int buffSize = 1024 * 2;

		audioSender = std::make_unique<AudioSender>(netManager);
		audioSender->initAudioEncoder(encoderinfo);

		//create audioSend thread
		std::thread send_audio(&RtmpClient::sendAudioData, this);
		threadMap["pushAu"] = std::move(send_audio);

		//videoSender / audioSender init encoder
		videoSender = std::make_unique<VideoSender>(mtx, cv, netManager);
		VideoDefinition vd = VideoDefinition(1280, 720);
		videoSender->initEncoder(vd, 19);

		//create videoSend thread
		std::thread send_video(&RtmpClient::sendVideoData, this);
		threadMap["pushVd"] = std::move(send_video);
		if (netManager->rtmpInit(1) == -1) {
			return;
		}
		cv->notify_all();
		startPush = _start;
	}
	else
		return;
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
		int len = recvFrameAu->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(recvFrameAu->format)) * recvFrameAu->channels;
		//uint8_t* data = new uint8_t[len + 1];
		audioSender->send(recvAuFrameDq.front()->data[0], len);
		recvAuFrameDq.pop_front();
		//av_frame_unref(recvFrameAu);
		//audioSender->send(recvFrameAu->data[0], len);
	}
	else if (_type == 2) {
		videoSender->sendFrame(recvVdFrameMap.find(2222)->second.front());
		//videoSender->sendFrame(recvVdFrameDq.front());
		//recvVdFrameDq.pop_front();
		//videoSender->sendFrame(combineYUV(recvFrameVd));
		//av_frame_unref(recvFrameVd);
	}
	else if (_type == 3) {
		;
	}
}

AVFrame* RtmpClient::combineYUV(AVFrame* pFrameYUV) {
	AVFrame* pDstFrame = av_frame_alloc();
	int nDstSize = avpicture_get_size(AV_PIX_FMT_YUV420P, FRAMEWIDTH * 2, FRAMEHEIGTH);
	uint8_t* dstbuf = new uint8_t[nDstSize];
	avpicture_fill((AVPicture*)pDstFrame, dstbuf, AV_PIX_FMT_YUV420P, FRAMEWIDTH * 2, FRAMEHEIGTH);

	pDstFrame->width = FRAMEWIDTH * 2;
	pDstFrame->height = FRAMEHEIGTH;
	pDstFrame->format = AV_PIX_FMT_YUV420P;

	//将预先分配的AVFrame图像背景数据设置为黑色背景  
	memset(pDstFrame->data[0], 0, FRAMEWIDTH * FRAMEHEIGTH * 2);
	memset(pDstFrame->data[1], 0x80, FRAMEWIDTH * FRAMEHEIGTH / 2);
	memset(pDstFrame->data[2], 0x80, FRAMEWIDTH * FRAMEHEIGTH / 2);
	
	if (pFrameYUV)
	{
		int nYIndex = 0;
		int nUVIndex = 0;

		for (int i = 0; i < FRAMEHEIGTH; i++)
		{
			//Y  
			memcpy(pDstFrame->data[0] + i * FRAMEWIDTH * 2, pFrameYUV->data[0] + nYIndex * FRAMEWIDTH, FRAMEWIDTH);
			memcpy(pDstFrame->data[0] + FRAMEWIDTH + i * FRAMEWIDTH * 2, pFrameYUV->data[0] + nYIndex * FRAMEWIDTH, FRAMEWIDTH);

			nYIndex++;
		}

		for (int i = 0; i < FRAMEHEIGTH / 4; i++)
		{
			//U
			memcpy(pDstFrame->data[1] + i * FRAMEWIDTH * 2, pFrameYUV->data[1] + nUVIndex * FRAMEWIDTH, FRAMEWIDTH);
			memcpy(pDstFrame->data[1] + FRAMEWIDTH + i * FRAMEWIDTH * 2, pFrameYUV->data[1] + nUVIndex * FRAMEWIDTH, FRAMEWIDTH);

			//V  
			memcpy(pDstFrame->data[2] + i * FRAMEWIDTH * 2, pFrameYUV->data[2] + nUVIndex * FRAMEWIDTH, FRAMEWIDTH);
			memcpy(pDstFrame->data[2] + FRAMEWIDTH + i * FRAMEWIDTH * 2, pFrameYUV->data[2] + nUVIndex * FRAMEWIDTH, FRAMEWIDTH);

			nUVIndex++;
		}
	}
	return pDstFrame;
}

void RtmpClient::setURL(string URL) {
	rtmpURL = URL;
	netManager->setRtmpUrl(rtmpURL);
}

RtmpClient::~RtmpClient() {
	I_LOG("RtmpClient destruct...");
	if (threadMap["pushVd"].joinable())
		threadMap["pushVd"].join();
	else if (threadMap["pollVd"].joinable())
		threadMap["pollVd"].join();
}