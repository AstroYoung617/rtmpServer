#include <iostream>
#include <other/loggerApi.h>
#include <business/rtmpClient.h>

int main() {
	RtmpClient rtmpClient;
	rtmpClient.printData(1);
	int port;
	//std::cout << "please input the audio port:";
	//while (std::cin >> port) {
	//	rtmpClient.setPort(1, port);
	//	getchar();
	//	break;
	//}
	std::cout << "please input the video port:";
	while (std::cin >> port) {
		rtmpClient.setPort(2, port);
		getchar();
		break;
	}
	return 0;
}