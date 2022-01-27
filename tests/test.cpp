#include <iostream>
#include <other/loggerApi.h>
#include <business/rtmpClient.h>

int main() {
	RtmpClient rtmpClient;
	rtmpClient.printData(1);
	int port;
	int start;
	std::cout << "please input the audio port(0 to skip):";
	while (std::cin >> port) {
		rtmpClient.setPort(1, port);
		getchar();
		break;
	}
	std::cout << "please input the video port(0 to skip):";
	while (std::cin >> port) {
		rtmpClient.setPort(2, port);
		getchar();
		break;
	}
	std::cout << "---------------------start?---------------------" << std::endl;
	std::cout << "if want start, print 1, else 0 : " << std::endl;
	while (std::cin >> start) {
		rtmpClient.setStart(start);
		getchar();
		break;
	}
	return 0;
}