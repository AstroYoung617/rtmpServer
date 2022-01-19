#include <iostream>
//对于C++而言的网络库，需要引入下面的头文件和lib库

//接收方实现组播接收需要加入组播，才能接收到组播消息
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "wsock32.lib")
#endif
#include "test/rtp.h"
//引入ws2ipdef来使用IP_MREQ
#include <ws2ipdef.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
//引入thread来使用this_thread进行轮询
#include <thread>

#include <WinInet.h>

//目的端口号
#define CLIENT_PORT 8888
#define SERVER_IP "127.0.0.1"
#define MULTICAST 1


int main()
{
  int on = 1;
  //加载winsock 无论是客户端还是服务器都需要
  std::cout << "this is Server" << std::endl;
  WSADATA wsaData;
  WORD socketVersion = MAKEWORD(2, 2);
  //初始化socket资源
  if (WSAStartup(socketVersion, &wsaData) != 0)
  {
    return 0;
  }
  //构造监听socket 标准udp写法
  SOCKET serSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  setsockopt(serSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
  //是否创建成功
  if (serSocket == INVALID_SOCKET)
  {
    printf("socket error !");
    return 0;
  }

  sockaddr_in serAddr;
  serAddr.sin_family = AF_INET;
  serAddr.sin_port = htons(CLIENT_PORT);
  //serAddr.sin_addr.s_addr= htonl(INADDR_ANY);
  serAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
  //把socket同端口进行绑定
  if (bind(serSocket, (sockaddr*)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
  {
    printf("bind local error !");
    closesocket(serSocket);
    return 0;
  }
  if (MULTICAST) {
    IP_MREQ mreq;
    mreq.imr_multiaddr.s_addr = inet_addr("239.255.42.42");
    mreq.imr_interface.s_addr = inet_addr(SERVER_IP);    //指定接口接收组播信息

    //setsockopt(serSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&mreq, sizeof(mreq));
    setsockopt(serSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&mreq, sizeof(mreq));
    int loop = 1;
    setsockopt(serSocket, IPPROTO_IP, IP_MULTICAST_LOOP, (const char*)&loop, sizeof(loop));

  }
    // local test p2p
    sockaddr_in remoteAddr;
    int nAddrLen = sizeof(remoteAddr);
  
  //进行轮询
  RtpPacket* rtpPacket = (RtpPacket*)malloc(5000);
  rtpHeaderInit(rtpPacket, 0, 0, 0, RTP_VESION, 97, 1, 0, 0, 0x32411);
  while (true)
  {
    char recvData[50];
    int ret = recvfrom(serSocket, recvData, 50, 0, (sockaddr*)&remoteAddr, &nAddrLen);
    if (ret > 0)
    {
      //recvData[ret] = 0x00;
      printf("接受到一个连接：%s \r\n", inet_ntoa(remoteAddr.sin_addr));
      //printf(recvData);
      for (int i = RTP_HEADER_SIZE; i < sizeof(recvData); i++) {
        std::cout << recvData[i] << " ";
      }
      std::cout << std::endl;
    }
    const char* sendData = "一个来自服务端的UDP数据包\n";
    
    rtpPacket->rtpHeader.seq = htons(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = htonl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = htonl(rtpPacket->rtpHeader.ssrc);
    //udp使用sendto
    sendto(serSocket, (const char*)rtpPacket, 3+RTP_HEADER_SIZE, 0, (sockaddr*)&remoteAddr, nAddrLen);

    rtpPacket->rtpHeader.seq = ntohs(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = ntohl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = ntohl(rtpPacket->rtpHeader.ssrc);

    rtpPacket->rtpHeader.seq++;
    rtpPacket->rtpHeader.timestamp += 500;
  }
  closesocket(serSocket);
  WSACleanup();
  return 0;
}