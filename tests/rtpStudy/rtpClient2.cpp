#include <iostream>
//对于C++而言的网络库，需要引入下面的头文件和lib库

//接收方实现组播接收需要加入组播，才能接收到组播消息
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h> 
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

int main()
{  // 0. 加载socket
  WORD socketVersion = MAKEWORD(2, 2);
  WSADATA wsaData;
  //初始化socket资源
  if (WSAStartup(socketVersion, &wsaData) != 0)
  {
    return 0;
  }
  // 1. 创建通信的套接字
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd == -1)
  {
    perror("socket");
    exit(0);
  }

  // 2. 通信的fd绑定本地的IP和端口
  //struct sockaddr_in addr;
  //addr.sin_family = AF_INET;
  //addr.sin_port = htons(8888);
  //addr.sin_addr.s_addr = INADDR_ANY;
  //int ret = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
  //if (ret == -1)
  //{
  //  perror("bind");
  //  exit(0);
  //}


  struct sockaddr_in cliaddr;
  cliaddr.sin_family = AF_INET;
  cliaddr.sin_port = htons(8888); // 客户端也需要绑定这端口
  inet_pton(AF_INET, "239.255.42.42", &cliaddr.sin_addr.s_addr);
  int len = sizeof(cliaddr);
  //inet_pton(AF_INET, "0.0.0.0", &addr.sin_addr.s_addr);
  // 加入到组播
  struct ip_mreq op;  //ip_mreq和ip_mreqn都有相同的效果， 后者是更新的版本多了一个设置级别ifindex，
                      //但是前者依然可以使用
  op.imr_interface.s_addr = INADDR_ANY; // 本地地址
  inet_pton(AF_INET, "239.255.42.42", &op.imr_multiaddr.s_addr);

  setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&op, sizeof(op));
  std::cout << "this is client" << std::endl;
  int num = 0;
  // 3. 通信
  RtpPacket* rtpPacket = (RtpPacket*)malloc(5000);
  RtpPacket* brtpPacket = (RtpPacket*)malloc(5000);
  rtpHeaderInit(rtpPacket, 0, 0, 0, RTP_VESION, 97, 1, 0, 0, 0x32411);
  while (1)
  {
    //test payload
    rtpPacket->payload[0] = 0x41;
    rtpPacket->payload[1] = 0x42;
    rtpPacket->payload[2] = 0x43;
    rtpPacket->payload[3] = 0x44;
    //发送数据
    rtpPacket->rtpHeader.seq = htons(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = htonl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = htonl(rtpPacket->rtpHeader.ssrc);
    sendto(fd, (const char*)rtpPacket, 10 + RTP_HEADER_SIZE, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
    rtpPacket->rtpHeader.seq = ntohs(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = ntohl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = ntohl(rtpPacket->rtpHeader.ssrc);
    rtpPacket->rtpHeader.seq++;
    rtpPacket->rtpHeader.timestamp += 500;
    // 接收数据
    char buf[128];
    recvfrom(fd, buf, strlen(buf) + 1, 0, (struct sockaddr*)&cliaddr, &len);
    brtpPacket = (RtpPacket*)buf;
    std::cout << "server say: ";
    for (int i = 0; i < 4; i++) {
      std::cout << brtpPacket->payload[i] << " ";
    }
    std::cout << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  closesocket(fd);
  return 0;
}