#include <iostream>
//����C++���Ե�����⣬��Ҫ���������ͷ�ļ���lib��

//���շ�ʵ���鲥������Ҫ�����鲥�����ܽ��յ��鲥��Ϣ
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
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
{  // 0. ����socket
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
  cliaddr.sin_port = htons(8888); // �ͻ���Ҳ��Ҫ����˿�
  inet_pton(AF_INET, "239.255.42.42", &cliaddr.sin_addr.s_addr);
  int len = sizeof(cliaddr);
  //inet_pton(AF_INET, "0.0.0.0", &addr.sin_addr.s_addr);
  // ���뵽�鲥
  struct ip_mreq op;  //ip_mreq��ip_mreqn������ͬ��Ч���� �����Ǹ��µİ汾����һ�����ü���ifindex��
                      //����ǰ����Ȼ����ʹ��
  op.imr_interface.s_addr = INADDR_ANY; // ���ص�ַ
  inet_pton(AF_INET, "239.255.42.42", &op.imr_multiaddr.s_addr);

  setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&op, sizeof(op));
  std::cout << "this is client" << std::endl;
  int num = 0;
  // 3. ͨ��
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
    //��������
    rtpPacket->rtpHeader.seq = htons(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = htonl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = htonl(rtpPacket->rtpHeader.ssrc);
    sendto(fd, (const char*)rtpPacket, 10 + RTP_HEADER_SIZE, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
    rtpPacket->rtpHeader.seq = ntohs(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = ntohl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = ntohl(rtpPacket->rtpHeader.ssrc);
    rtpPacket->rtpHeader.seq++;
    rtpPacket->rtpHeader.timestamp += 500;
    // ��������
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