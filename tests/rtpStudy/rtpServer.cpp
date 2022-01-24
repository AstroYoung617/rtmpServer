#include <iostream>
//����C++���Ե�����⣬��Ҫ���������ͷ�ļ���lib��

//���շ�ʵ���鲥������Ҫ�����鲥�����ܽ��յ��鲥��Ϣ
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
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

#include <WinInet.h>

//Ŀ�Ķ˿ں�
#define CLIENT_PORT 8888
#define SERVER_IP "127.0.0.1"
#define MULTICAST 1


int main()
{
  int on = 1;
  //����winsock �����ǿͻ��˻��Ƿ���������Ҫ
  std::cout << "this is Server" << std::endl;
  WSADATA wsaData;
  WORD socketVersion = MAKEWORD(2, 2);
  //��ʼ��socket��Դ
  if (WSAStartup(socketVersion, &wsaData) != 0)
  {
    return 0;
  }
  //�������socket ��׼udpд��
  SOCKET serSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  setsockopt(serSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
  //�Ƿ񴴽��ɹ�
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
  //��socketͬ�˿ڽ��а�
  if (bind(serSocket, (sockaddr*)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
  {
    printf("bind local error !");
    closesocket(serSocket);
    return 0;
  }
  if (MULTICAST) {
    IP_MREQ mreq;
    mreq.imr_multiaddr.s_addr = inet_addr("239.255.42.42");
    mreq.imr_interface.s_addr = inet_addr(SERVER_IP);    //ָ���ӿڽ����鲥��Ϣ

    //setsockopt(serSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&mreq, sizeof(mreq));
    setsockopt(serSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&mreq, sizeof(mreq));
    int loop = 1;
    setsockopt(serSocket, IPPROTO_IP, IP_MULTICAST_LOOP, (const char*)&loop, sizeof(loop));

  }
    // local test p2p
    sockaddr_in remoteAddr;
    int nAddrLen = sizeof(remoteAddr);
  
  //������ѯ
  RtpPacket* rtpPacket = (RtpPacket*)malloc(5000);
  rtpHeaderInit(rtpPacket, 0, 0, 0, RTP_VESION, 97, 1, 0, 0, 0x32411);
  while (true)
  {
    char recvData[50];
    int ret = recvfrom(serSocket, recvData, 50, 0, (sockaddr*)&remoteAddr, &nAddrLen);
    if (ret > 0)
    {
      //recvData[ret] = 0x00;
      printf("���ܵ�һ�����ӣ�%s \r\n", inet_ntoa(remoteAddr.sin_addr));
      //printf(recvData);
      for (int i = RTP_HEADER_SIZE; i < sizeof(recvData); i++) {
        std::cout << recvData[i] << " ";
      }
      std::cout << std::endl;
    }
    const char* sendData = "һ�����Է���˵�UDP���ݰ�\n";
    
    rtpPacket->rtpHeader.seq = htons(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = htonl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = htonl(rtpPacket->rtpHeader.ssrc);
    //udpʹ��sendto
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