#include <iostream>
#include <media/VideoReceiver.h>


VideoReceiver::VideoReceiver() {
	I_LOG("VideoReceiver struct success");
}

void VideoReceiver::setPort(int _port) {
	port = _port;
}

int VideoReceiver::getPort() {
	return port;
}

char* VideoReceiver::getData() {
	return data;
}

void VideoReceiver::setData(char* _data) {
	data = _data;
}

VideoReceiver::~VideoReceiver() {
	I_LOG("VideoReceiver destruct...");
}