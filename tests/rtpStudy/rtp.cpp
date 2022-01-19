//对于C++而言的网络库，需要引入下面的头文件和lib库
#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "WS2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <iostream>
#include "test/rtp.h"

void rtpHeaderInit(struct RtpPacket* rtpPacket, uint8_t csrcLen, uint8_t extension,
  uint8_t padding, uint8_t version, uint8_t payloadType, uint8_t marker,
  uint16_t seq, uint32_t timestamp, uint32_t ssrc)
{
  rtpPacket->rtpHeader.csrcLen = csrcLen;
  rtpPacket->rtpHeader.extension = extension;
  rtpPacket->rtpHeader.padding = padding;
  rtpPacket->rtpHeader.version = version;
  rtpPacket->rtpHeader.payloadType = payloadType;
  rtpPacket->rtpHeader.marker = marker;
  rtpPacket->rtpHeader.seq = seq;
  rtpPacket->rtpHeader.timestamp = timestamp;
  rtpPacket->rtpHeader.ssrc = ssrc;
}

int rtpSendPacket(int socket, char* ip, int16_t port, struct RtpPacket* rtpPacket, uint32_t dataSize)
{
  struct sockaddr_in addr;
  int ret;

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(ip);

  rtpPacket->rtpHeader.seq = htons(rtpPacket->rtpHeader.seq);
  rtpPacket->rtpHeader.timestamp = htonl(rtpPacket->rtpHeader.timestamp);
  rtpPacket->rtpHeader.ssrc = htonl(rtpPacket->rtpHeader.ssrc);

  //sendto 就是使用udp进行socket传输的标志
  ret = sendto(socket, (const char*)rtpPacket, dataSize + RTP_HEADER_SIZE, 0,
    (struct sockaddr*)&addr, sizeof(addr));

  rtpPacket->rtpHeader.seq = ntohs(rtpPacket->rtpHeader.seq);
  rtpPacket->rtpHeader.timestamp = ntohl(rtpPacket->rtpHeader.timestamp);
  rtpPacket->rtpHeader.ssrc = ntohl(rtpPacket->rtpHeader.ssrc);

  return ret;
}

void printRtpPacket(RtpPacket* _packet) {
  std::cout << "crsclen: " << _packet->rtpHeader.csrcLen << std::endl;
  std::cout << "extension: " << _packet->rtpHeader.extension << std::endl;
  std::cout << "padding: " << _packet->rtpHeader.padding << std::endl;
  std::cout << "version: " << _packet->rtpHeader.version << std::endl;
  std::cout << "payloadType: " << _packet->rtpHeader.payloadType << std::endl;
  std::cout << "marker: " << _packet->rtpHeader.marker << std::endl;
  std::cout << "seq: " << _packet->rtpHeader.seq << std::endl;
  std::cout << "timestamp: " << _packet->rtpHeader.timestamp << std::endl;
  std::cout << "ssrc: " << _packet->rtpHeader.ssrc << std::endl;
  std::cout << std::endl;
  std::cout << std::endl;
}
