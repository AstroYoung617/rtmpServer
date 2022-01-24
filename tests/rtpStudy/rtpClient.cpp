#include <stdio.h> 
#include <winsock2.h> 
#include <iostream>
#include <thread>
#pragma comment(lib,"ws2_32.lib")  
#include "test/rtp.h"


int main(int argc, char* argv[])
{
  std::cout << "this is Client" << std::endl;
  int on = 1;
  //���ط���
  WORD socketVersion = MAKEWORD(2, 2);
  WSADATA wsaData;
  //��ʼ��socket��Դ
  if (WSAStartup(socketVersion, &wsaData) != 0)
  {
    return 0;
  }
  //�����׽���
  SOCKET sclient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  setsockopt(sclient, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

  sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_port = htons(8888);
  sin.sin_addr.s_addr = inet_addr("239.255.42.42");
  int len = sizeof(sin);

  RtpPacket* rtpPacket = (RtpPacket*)malloc(5000);
  rtpHeaderInit(rtpPacket, 0, 0, 0, RTP_VESION, 97, 1, 0, 0, 0x32411);
  while (true) {
    rtpPacket->payload[0] = 0x00;
    rtpPacket->payload[1] = 0x10;
    rtpPacket->payload[2] = 0x40;
    rtpPacket->payload[3] = 0x41;
    const char* sendData = "���Կͻ��˵����ݰ�.\n";

    //����ǰ�Ĳ���
    rtpPacket->rtpHeader.seq = htons(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = htonl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = htonl(rtpPacket->rtpHeader.ssrc);

    sendto(sclient, (const char*)rtpPacket, 10 + RTP_HEADER_SIZE, 0, (sockaddr*)&sin, len);
    printRtpPacket(rtpPacket);
    //���ͺ�Ĳ���
    rtpPacket->rtpHeader.seq = ntohs(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = ntohl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = ntohl(rtpPacket->rtpHeader.ssrc);
    

    //��ʱֻ���ͽ��в���
    char recvData[50];
    int ret = recvfrom(sclient, recvData, 50, 0, (sockaddr*)&sin, &len);
    if (ret > 0)
    {
      //recvData[ret] = 0x00;
      //printf(recvData);
      for (int i = 0; i < sizeof(recvData); i++) {
        std::cout << (uint8_t)recvData[i] << " ";
      }
      std::cout << std::endl;
    }


    rtpPacket->rtpHeader.seq++;
    rtpPacket->rtpHeader.timestamp += 500;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  //�ͷ�socket
  closesocket(sclient);
  WSACleanup();
  return 0;
}
