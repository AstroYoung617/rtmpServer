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
	std::thread send_audio(&RtmpClient::sendAudioData, this);
	threadMap["pollAu"] = std::move(get_audio);
	threadMap["pushAu"] = std::move(send_audio);
}

void RtmpClient::createVideoCh(int _port) {
	I_LOG("Create an new VideoChannel port:{}", _port);
	auto videoReceiver = new VideoReceiver(_port, vdmtx, vdcv);
	VideoRecvVct.push_back(*videoReceiver);
	//auto videoReceiver1 = new VideoReceiver(_port + 2, vdmtx, vdcv);
	//VideoRecvVct.push_back(*videoReceiver1);

	std::thread get_video(&RtmpClient::getVideoData, this);
	std::thread send_video(&RtmpClient::sendVideoData, this);
	std::thread count_vfps(&RtmpClient::countFPS, this);
	threadMap["pollVd"] = std::move(get_video);
	threadMap["pushVd"] = std::move(send_video);
	threadMap["countFps"] = std::move(count_vfps);
}

void RtmpClient::countFPS(){
	while (1) {
		if (!startPush) {
			std::unique_lock<std::mutex> lk(*mtx);
			cv->wait(lk);
		}
		if (countRcvV < 25) {
			for (int i = 0; i < 25 - countRcvV; i++) {
				if (recvVdFrameDq.size() && recvVdFrameDq.front()->data[0]) {
					recvVdFrameDq.push_front(recvVdFrameDq.front());
					countSndV++;
				}
			}
		}
		E_LOG("video receive FPS = {}", countRcvV);
		E_LOG("video send FPS = {}", countSndV);
		countRcvV = 0;
		countSndV = 0;
		std::this_thread::sleep_for(std::chrono::seconds(1));
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
		if (recvFrameAu && recvFrameAu->data[0]) {
			recvAuFrameDq.push_back(recvFrameAu);
			countRcvA++;
			aucv->notify_one();
		}
		//TODO 之后将多个videoRecv的data进行拼接后传给videoSender
		//lk.unlock();
		//std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
}

void RtmpClient::sendAudioData() {
	//std::this_thread::sleep_for(std::chrono::milliseconds(1900));
	while (1) {
		if (!startPush) {
			std::unique_lock<std::mutex> lk(*mtx);
			cv->wait(lk);
		}
		std::unique_lock<std::mutex> lck(*aumtx);
		aucv->wait(lck);
		if (recvAuFrameDq.size() && recvAuFrameDq.front()->data[0])
			send2Rtmp(1);
		//std::this_thread::sleep_for(std::chrono::milliseconds(25));
		//av_usleep(25000);
		lck.unlock();
	}
}

void RtmpClient::getVideoData() {
	auto videoRecv = VideoRecvVct.front();
	while (1) {
		if (!startPush) {
			std::unique_lock<std::mutex> lk(*mtx);
			cv->wait(lk);
		}
		
		videoRecv.processRecvRtpData();
		//先只用一个AVFrame作为存储，将其传递给videoSender
		recvFrameVd = videoRecv.getData();
		if (recvFrameVd && recvFrameVd->data[0]) {
			recvVdFrameDq.push_back(recvFrameVd);
			countRcvV++;
			vdcv->notify_one();
		}
		//TODO 之后将多个videoRecv的data进行拼接后传给videoSender
		//lk.unlock();
	}
}

void RtmpClient::sendVideoData() {
	while (1) {
		if (!startPush) {
			std::unique_lock<std::mutex> lk(*mtx);
			cv->wait(lk);
		}
		//std::unique_lock<std::mutex> lck(*vdmtx);
		//vdcv->wait(lck);
		//vdcv->wait_for(lck, std::chrono::milliseconds(45));
		if (recvVdFrameDq.size() && recvVdFrameDq.front()->data[0]) {
			countSndV++;
			send2Rtmp(2);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(40));

	  //lck.unlock();

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

		//videoSender / audioSender init encoder
		videoSender = std::make_unique<VideoSender>(mtx, cv, netManager);
		VideoDefinition vd = VideoDefinition(640, 480);
		videoSender->initEncoder(vd, 20);

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
		audioSender->send(recvAuFrameDq.front()->data[0], len);
		recvAuFrameDq.pop_front();
		//av_frame_unref(recvFrameAu);
		//audioSender->send(recvFrameAu->data[0], len);
	}
	else if (_type == 2) {
		//if (videoSender->lastPts / 2 > audioSender->lastPts) {
		//	//av_usleep(1000 * (videoSender->lastPts * 2 - audioSender->lastPts));
		//	recvVdFrameDq.pop_front();
		//	return;
		//}
		videoSender->sendFrame(recvVdFrameDq.front());
		recvVdFrameDq.pop_front();
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
	else if (threadMap["countFps"].joinable())
		threadMap["countFps"].join();
}