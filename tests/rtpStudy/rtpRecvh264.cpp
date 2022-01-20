#include <iostream>
//����C++���Ե�����⣬��Ҫ���������ͷ�ļ���lib��

//���շ�ʵ���鲥������Ҫ�����鲥�����ܽ��յ��鲥��Ϣ
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
//����ws2tcpip��ʹ��inet_pton
#include <ws2tcpip.h> 
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "wsock32.lib")
#endif
#include "test/rtp.h"
//����ws2ipdef��ʹ��IP_MREQ
#include <ws2ipdef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
//����thread��ʹ��this_thread������ѯ
#include <thread>

int main()
{
  // 0. ����socket
  WORD socketVersion = MAKEWORD(2, 2);
  WSADATA wsaData;
  //��ʼ��socket��Դ
  if (WSAStartup(socketVersion, &wsaData) != 0)
  {
    return 0;
  }
  // 1. ����ͨ�ŵ��׽���
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd == -1)
  {
    perror("socket");
    exit(0);
  }

  // 2. ͨ�ŵ�fd�󶨱��ص�IP�Ͷ˿�
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(1234);
  addr.sin_addr.s_addr = INADDR_ANY;
  int ret = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
  if (ret == -1)
  {
    perror("bind");
    exit(0);
  }
  //inet_pton(AF_INET, "0.0.0.0", &addr.sin_addr.s_addr);
  std::cout << "this is server" << std::endl;
  // 3. ͨ��
  sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_port = htons(1234);
  sin.sin_addr.s_addr = inet_addr("127.0.0.1");
  int len = sizeof(sin);

  RtpPacket* rtpPacket = (RtpPacket*)malloc(50000);
  while (1)
  {

    // ��������
    char buf[50000];
    //���������鲥�յ�����ʱ���鲥�����ѣ����鲥�л�ȡ���ݡ�
    recvfrom(fd, buf, strlen(buf) + 1, 0, (sockaddr*)&sin, &len);
    rtpPacket = (RtpPacket*)buf;
    uint8_t temp;
    std::cout << "client say: ";
    for (int i = 0; i < 4; i++) {
      std::cout << rtpPacket->payload[i] << " ";
    }
    //���ı���������
    for (int i = 0; i < 2; i++) {
      temp = rtpPacket->payload[i];
      rtpPacket->payload[i] = rtpPacket->payload[4 - i - 1];
      rtpPacket->payload[4 - i - 1] = temp;
    }
    std::cout << std::endl;

  }
  closesocket(fd);
  return 0;
}