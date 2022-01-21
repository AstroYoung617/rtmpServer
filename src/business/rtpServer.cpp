#include <iostream>
#include <business/rtpServer.h>

RtpServer::RtpServer() {
	I_LOG("RtpServer construct success!");
	AudioReceiver AudioReceiver0;
	AudioReceiver AudioReceiver1;
	AudioReceiver0.setPort(1234);
	AudioReceiver1.setPort(1244);
	AudioReceiver0.setData("hello");
	AudioReceiver1.setData(" world");
	AudioChnlVct.push_back(AudioReceiver0);
	AudioChnlVct.push_back(AudioReceiver1);
}

std::string RtpServer::getTotalData() {
	std::string totalData = "";
	char* data = "1";
	for (auto it : AudioChnlVct) {
		totalData += it.getData();
	}
	return totalData;
}

RtpServer::~RtpServer() {
	I_LOG("Rtpserver destruct...");
}