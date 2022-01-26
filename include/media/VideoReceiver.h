/*
	* Created by Astro
	* Date : 2022.01.24
	* Descryption:
		*		用于接收该端口的h264视频，目前应该要做到将rtp的视频流保存到本地
*/

#include <iostream>
#include <string>
#include <other/loggerApi.h>
#include <net/NetManager.h>
#include <media/common.h>
#include <media/decoder/videoDecoder.h>
#include <utility>
#include <mutex>

struct VideoReceiver 
{
	VideoReceiver(int _port, std::mutex* _mutex, std::condition_variable* _vdcv);
	~VideoReceiver();

	//设置及获取videoReceiver接收的端口
	void setPort(int _port);
	int getPort();

	//获取解码后的数据，暂时未放入deque
	AVFrame* getData();

	// NOT USE
	void setData(uint8_t* _data);

	//处理接收到的Rtp数据
	void processRecvRtpData();

private:
	void initSocket();

	void closeSocket();
	//decode 输入payload 由parser来进行处理nalu header...
	void decode(uint8_t* data, size_t len, int64_t ts);

	//解包rtp parameter: 输入的数据， 输入数据长度
	void process(char* bufIn, int len);

	//保存解码后的yuv到文件
	int saveYUVFrameToFile(AVFrame* frame, int width, int height);

	NALU_t* AllocNALU(int buffersize);
	int port = 0;

	std::mutex* mutex;
	std::condition_variable* vdcv;

	Decoder* decoder = nullptr;
	AVFrame* recvFrameCache = nullptr;
	AVFrame* recvFrame = nullptr;

	//network 
	char recvbuf[MAXDATASIZE];  //加上头最大传输数据 1500
	int sockfd;
	//FILE* client_fd;
	int sin_size;
	char sendbuf[BUFFER_SIZE];
	struct sockaddr_in server_sockaddr, client_sockaddr;
	AVFrame* frame;
	int	receive_bytes = 0;

};