#include <iostream>
//对于C++而言的网络库，需要引入下面的头文件和lib库

//接收方实现组播接收需要加入组播，才能接收到组播消息
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
//引入ws2tcpip来使用inet_pton
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
{
  // 0. 加载socket
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
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(8888);
  addr.sin_addr.s_addr = INADDR_ANY;
  int ret = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
  if (ret == -1)
  {
    perror("bind");
    exit(0);
  }
  //inet_pton(AF_INET, "0.0.0.0", &addr.sin_addr.s_addr);
  // 加入到组播
  struct ip_mreq op;  //ip_mreq和ip_mreqn都有相同的效果， 后者是更新的版本多了一个设置级别ifindex，
  //但是前者依然可以使用
  op.imr_interface.s_addr = INADDR_ANY; // 本地地址
  inet_pton(AF_INET, "239.255.42.42", &op.imr_multiaddr.s_addr);
  //加入组播
  setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&op, sizeof(op));
  std::cout << "this is server" << std::endl;
  // 3. 通信
  sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_port = htons(8888);
  sin.sin_addr.s_addr = inet_addr("239.255.42.42");
  int len = sizeof(sin);

  RtpPacket* rtpPacket = (RtpPacket*)malloc(5000);
  while (1)
  {

    // 接收数据
    char buf[128];
    //阻塞，当组播收到数据时被组播程序唤醒，从组播中获取数据。
    recvfrom(fd, buf, strlen(buf) + 1, 0, (sockaddr*)&sin, &len);
    rtpPacket = (RtpPacket*)buf;
    uint8_t temp;
    std::cout << "client say: ";
    for (int i = 0; i < 4; i++) {
      std::cout << rtpPacket->payload[i] << " ";
    } 
      //对文本进行逆序
    for (int i = 0; i < 2; i++) {
      temp = rtpPacket->payload[i];
      rtpPacket->payload[i] = rtpPacket->payload[4 - i - 1];
      rtpPacket->payload[4 - i - 1] = temp;
    }
    std::cout << std::endl;

    // 发送数据
    const char* sendData = "一个来自服务端的UDP数据包\n";
    
    rtpPacket->rtpHeader.seq = htons(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = htonl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = htonl(rtpPacket->rtpHeader.ssrc);
    //udp使用sendto
    sendto(fd, (const char*)rtpPacket, 10 + RTP_HEADER_SIZE, 0, (sockaddr*)&sin, len);

    //在组播的情况下recvFrom和sendto使用同一个地址进行传输
    //sendto(fd, buf, strlen(buf) + 1, 0, (sockaddr*)&sin, len);
    rtpPacket->rtpHeader.seq = ntohs(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = ntohl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = ntohl(rtpPacket->rtpHeader.ssrc);

    rtpPacket->rtpHeader.seq++;
    rtpPacket->rtpHeader.timestamp += 500;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  closesocket(fd);
  return 0;
}