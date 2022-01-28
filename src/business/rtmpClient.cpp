#include <iostream>
#include <business/rtmpClient.h>


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


	std::thread get_video(&RtmpClient::getVideoData, this);
	std::thread send_video(&RtmpClient::sendVideoData, this);
	threadMap["pollVd"] = std::move(get_video);
	threadMap["pushVd"] = std::move(send_video);
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
		//TODO 之后将多个videoRecv的data进行拼接后传给videoSender
		//lk.unlock();
	}
}

void RtmpClient::sendAudioData() {
	while (1) {
		if (!startPush) {
			std::unique_lock<std::mutex> lk(*mtx);
			cv->wait(lk);
		}
		if (recvFrameAu && recvFrameAu->data[0])
			send2Rtmp(1);
		std::this_thread::sleep_for(std::chrono::milliseconds(25));
		//lk.unlock();
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
		//TODO 之后将多个videoRecv的data进行拼接后传给videoSender
		//lk.unlock();
		vdcv->notify_one();
	}
}

void RtmpClient::sendVideoData() {
	while (1) {
		if (!startPush) {
			std::unique_lock<std::mutex> lk(*mtx);
			cv->wait(lk);
		}
		std::unique_lock<std::mutex> lck(*vdmtx);
		vdcv->wait(lck);
		if (recvFrameVd && recvFrameVd->data[0])
			send2Rtmp(2);
		//std::this_thread::sleep_for(std::chrono::milliseconds(50));

		lck.unlock();

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
		encoderinfo.outSampleRate = 44100;
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
		uint8_t* data = new uint8_t[len + 1];
		audioSender->send(recvFrameAu->data[0], len);
	}
	else if (_type == 2) {
		videoSender->sendFrame(recvFrameVd);
		av_frame_unref(recvFrameVd);
	}
	else if (_type == 3) {
		;
	}
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