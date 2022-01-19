// test : ���Է���RTP��Ƶ���ݣ�����ffplay���Ÿ�RTP��Ƶ����AAC��װ


#include <iostream>
//����C++���Ե�����⣬��Ҫ���������ͷ�ļ���lib��
#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "WS2_32.lib")
//����C������Ҫ���������ͷ�ļ�
#else
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#ifndef INVALID_SOCKET
#define INVALID_SOCKET	(SOCKET)(~0)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR	(-1)
#endif
#ifndef closesocket
#define closesocket(x)	close(x)
#endif
typedef int SOCKET;
#endif

#include "test/rtp.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <thread>

#define AAC_FILE    "E:/common/32000.aac"
#define CLIENT_PORT 9832

struct AdtsHeader
{
  unsigned int syncword;  //12 bit ͬ���� '1111 1111 1111'��˵��һ��ADTS֡�Ŀ�ʼ
  unsigned int id;        //1 bit MPEG ��ʾ���� 0 for MPEG-4��1 for MPEG-2
  unsigned int layer;     //2 bit ����'00'
  unsigned int protectionAbsent;  //1 bit 1��ʾû��crc��0��ʾ��crc
  unsigned int profile;           //1 bit ��ʾʹ���ĸ������AAC
  unsigned int samplingFreqIndex; //4 bit ��ʾʹ�õĲ���Ƶ��
  unsigned int privateBit;        //1 bit
  unsigned int channelCfg; //3 bit ��ʾ������
  unsigned int originalCopy;         //1 bit 
  unsigned int home;                  //1 bit 

  /*�����Ϊ�ı�Ĳ�����ÿһ֡����ͬ*/
  unsigned int copyrightIdentificationBit;   //1 bit
  unsigned int copyrightIdentificationStart; //1 bit
  unsigned int aacFrameLength;               //13 bit һ��ADTS֡�ĳ��Ȱ���ADTSͷ��AACԭʼ��
  unsigned int adtsBufferFullness;           //11 bit 0x7FF ˵�������ʿɱ������

  /* number_of_raw_data_blocks_in_frame
   * ��ʾADTS֡����number_of_raw_data_blocks_in_frame + 1��AACԭʼ֡
   * ����˵number_of_raw_data_blocks_in_frame == 0
   * ��ʾ˵ADTS֡����һ��AAC���ݿ鲢����˵û�С�(һ��AACԭʼ֡����һ��ʱ����1024���������������)
   */
  unsigned int numberOfRawDataBlockInFrame; //2 bit
};

static int parseAdtsHeader(uint8_t* in, struct AdtsHeader* res)
{
  static int frame_number = 0;
  memset(res, 0, sizeof(*res));

  if ((in[0] == 0xFF) && ((in[1] & 0xF0) == 0xF0))
  {
    res->id = ((unsigned int)in[1] & 0x08) >> 3;
    //printf("adts:id  %d\n", res->id);
    res->layer = ((unsigned int)in[1] & 0x06) >> 1;
    //printf("adts:layer  %d\n", res->layer);
    res->protectionAbsent = (unsigned int)in[1] & 0x01;
    //printf("adts:protection_absent  %d\n", res->protectionAbsent);
    res->profile = ((unsigned int)in[2] & 0xc0) >> 6;
    //printf("adts:profile  %d\n", res->profile);
    res->samplingFreqIndex = ((unsigned int)in[2] & 0x3c) >> 2;
    //printf("adts:sf_index  %d\n", res->samplingFreqIndex);
    res->privateBit = ((unsigned int)in[2] & 0x02) >> 1;
    //printf("adts:pritvate_bit  %d\n", res->privateBit);
    res->channelCfg = ((((unsigned int)in[2] & 0x01) << 2) | (((unsigned int)in[3] & 0xc0) >> 6));
    //printf("adts:channel_configuration  %d\n", res->channelCfg);
    res->originalCopy = ((unsigned int)in[3] & 0x20) >> 5;
    //printf("adts:original  %d\n", res->originalCopy);
    res->home = ((unsigned int)in[3] & 0x10) >> 4;
    //printf("adts:home  %d\n", res->home);
    res->copyrightIdentificationBit = ((unsigned int)in[3] & 0x08) >> 3;
    //printf("adts:copyright_identification_bit  %d\n", res->copyrightIdentificationBit);
    res->copyrightIdentificationStart = (unsigned int)in[3] & 0x04 >> 2;
    //printf("adts:copyright_identification_start  %d\n", res->copyrightIdentificationStart);
    res->aacFrameLength = (((((unsigned int)in[3]) & 0x03) << 11) |
      (((unsigned int)in[4] & 0xFF) << 3) |
      ((unsigned int)in[5] & 0xE0) >> 5);
    //printf("adts:aac_frame_length  %d\n", res->aacFrameLength);
    res->adtsBufferFullness = (((unsigned int)in[5] & 0x1f) << 6 |
      ((unsigned int)in[6] & 0xfc) >> 2);
    //printf("adts:adts_buffer_fullness  %d\n", res->adtsBufferFullness);
    res->numberOfRawDataBlockInFrame = ((unsigned int)in[6] & 0x03);
    //printf("adts:no_raw_data_blocks_in_frame  %d\n", res->numberOfRawDataBlockInFrame);

    return 0;
  }
  else
  {
    printf("failed to parse adts header\n");
    return -1;
  }
}

static int createUdpSocket()
{
  SOCKET fd;
  int on = 1;
  //����winsock �����ǿͻ��˻��Ƿ���������Ҫ
  WORD socketVersion = MAKEWORD(2, 2);
  WSADATA wsaData;
  //��ʼ��socket��Դ
  if (WSAStartup(socketVersion, &wsaData) != 0)
  {
    return 0;
  }
  //�������socket
  fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (fd < 0)
    return -1;

  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

  return fd;
}

static int rtpSendAACFrame(int socket, char* ip, int16_t port,
  struct RtpPacket* rtpPacket, uint8_t* frame, uint32_t frameSize)
{
  int ret;
  //AAC���ݵ�ǰ���ĸ��ֽڶ������⺬�壬��Ҫ���������ʽ
  rtpPacket->payload[0] = 0x00;
  rtpPacket->payload[1] = 0x10; 
  rtpPacket->payload[2] = (frameSize & 0x1FE0) >> 5; //����AAC Data�����ݴ�С ��8λ   
  rtpPacket->payload[3] = (frameSize & 0x1F) << 3; //����AAC Data�����ݴ�С ��5λ ���ֻ�ܱ���13bit

  memcpy(rtpPacket->payload + 4, frame, frameSize);

  ret = rtpSendPacket(socket, ip, port, rtpPacket, frameSize + 4);
  printRtpPacket(rtpPacket);
  if (ret < 0)
  {
    printf("failed to send rtp packet\n");
    return -1;
  }

  rtpPacket->rtpHeader.seq++;

  /*
   * �������Ƶ����44100
   * һ��AACÿ��1024������Ϊһ֡
   * ����һ����� 44100 / 1024 = 43֡
   * ʱ���������� 44100 / 43 = 1025
   * һ֡��ʱ��Ϊ 1 / 43 = 23ms

   * �������Ƶ����32000
   * һ��AACÿ��1024������Ϊһ֡
   * ����һ����� 32000 / 1024 = 31.25֡
   * ʱ���������� 32000 / 31.25 = 1024
   * һ֡��ʱ��Ϊ 1 / 31.25 = 32ms
   */
  rtpPacket->rtpHeader.timestamp += 1024;

  return 0;
}

int main(int argc, char* argv[])
{
  FILE* fd;
  int ret;
  int socket;
  uint8_t* frame;
  struct AdtsHeader adtsHeader;
  struct RtpPacket* rtpPacket;

  fd = fopen(AAC_FILE, "rb");
  if (fd == nullptr)
  {
    printf("failed to open %s\n", AAC_FILE);
    return -1;
  }

  socket = createUdpSocket();
  if (socket < 0)
  {
    printf("failed to create udp socket\n");
    return -1;
  }

  frame = (uint8_t*)malloc(5000);
  rtpPacket = (RtpPacket*)malloc(5000);

  rtpHeaderInit(rtpPacket, 0, 0, 0, RTP_VESION, RTP_PAYLOAD_TYPE_AAC, 1, 0, 0, 0x32411);

  while (1)
  {

    ret = fread(frame, 1, 7, fd);
    if (ret <= 0)
    {
      fseek(fd, 0, SEEK_SET);
      continue;
    }

    if (parseAdtsHeader(frame, &adtsHeader) < 0)
    {
      printf("parse err\n");
      break;
    }

    ret = fread(frame, 1, adtsHeader.aacFrameLength - 7, fd);
    if (ret < 0)
    {
      printf("read err\n");
      break;
    }

    rtpSendAACFrame(socket, "233.255.42.42", CLIENT_PORT,
      rtpPacket, frame, adtsHeader.aacFrameLength - 7);

    std::this_thread::sleep_for(std::chrono::milliseconds(25));
  }

  fclose(fd);
  //�ͷ���Դ
  closesocket(socket);
  WSACleanup();

  free(frame);
  free(rtpPacket);

  return 0;
}