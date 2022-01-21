#include <iostream>
#include <media/AudioReceiver.h>


AudioReceiver::AudioReceiver() {
	I_LOG("AudioReceiver struct success");
}

void AudioReceiver::setPort(int _port) {
	port = _port;
}

int AudioReceiver::getPort() {
	return port;
}

char* AudioReceiver::getData() {
	return data;
}

void AudioReceiver::setData(char* _data) {
	data = _data;
}

AudioReceiver::~AudioReceiver() {
I_LOG("AudioReceiver destruct...");
}