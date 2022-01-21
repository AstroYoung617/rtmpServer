#include <iostream>
#include <business/rtmpClient.h>

RtmpClient::RtmpClient() {
	I_LOG("RtmpClient construct success");
	rtpServer = std::make_unique<RtpServer>();
}

void RtmpClient::printData(int _type) {
	if (_type == 1) {
		I_LOG("print audio data");
		I_LOG("{}", rtpServer->getTotalData());
	}
}

RtmpClient::~RtmpClient() {
	I_LOG("RtmpClient destruct...");
}