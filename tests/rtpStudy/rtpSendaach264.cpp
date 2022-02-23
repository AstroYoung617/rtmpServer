// test : 测试发送RTP音频数据，采用ffplay播放该RTP音频流，AAC封装


#include <iostream>
#include <thread>
//对于C++而言的网络库，需要引入下面的头文件和lib库
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
//引入ws2tcpip来使用inet_pton
#include <ws2tcpip.h> 
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "wsock32.lib")
//对于C而言需要引入下面的头文件
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
#include <ws2ipdef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <thread>

#define AAC_FILE    "./test.aac"
#define CLIENT_PORT 1234
#define H264_FILE_NAME  "./test.h264"
#define CLIENT_IP       "127.0.0.1"   //destination ip
#define FPS             25         

struct AdtsHeader
{
  unsigned int syncword;  //12 bit 同步字 '1111 1111 1111'，说明一个ADTS帧的开始
  unsigned int id;        //1 bit MPEG 标示符， 0 for MPEG-4，1 for MPEG-2
  unsigned int layer;     //2 bit 总是'00'
  unsigned int protectionAbsent;  //1 bit 1表示没有crc，0表示有crc
  unsigned int profile;           //1 bit 表示使用哪个级别的AAC
  unsigned int samplingFreqIndex; //4 bit 表示使用的采样频率
  unsigned int privateBit;        //1 bit
  unsigned int channelCfg; //3 bit 表示声道数
  unsigned int originalCopy;         //1 bit 
  unsigned int home;                  //1 bit 

  /*下面的为改变的参数即每一帧都不同*/
  unsigned int copyrightIdentificationBit;   //1 bit
  unsigned int copyrightIdentificationStart; //1 bit
  unsigned int aacFrameLength;               //13 bit 一个ADTS帧的长度包括ADTS头和AAC原始流
  unsigned int adtsBufferFullness;           //11 bit 0x7FF 说明是码率可变的码流

  /* number_of_raw_data_blocks_in_frame
   * 表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧
   * 所以说number_of_raw_data_blocks_in_frame == 0
   * 表示说ADTS帧中有一个AAC数据块并不是说没有。(一个AAC原始帧包含一段时间内1024个采样及相关数据)
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
  //加载winsock 无论是客户端还是服务器都需要
  WORD socketVersion = MAKEWORD(2, 2);
  WSADATA wsaData;
  //初始化socket资源
  if (WSAStartup(socketVersion, &wsaData) != 0)
  {
    return 0;
  }
  //构造监听socket
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
  //AAC数据的前面四个字节都有特殊含义，需要遵照这个格式
  rtpPacket->payload[0] = 0x00;
  rtpPacket->payload[1] = 0x10;
  rtpPacket->payload[2] = (frameSize & 0x1FE0) >> 5; //保存AAC Data的数据大小 高8位   
  rtpPacket->payload[3] = (frameSize & 0x1F) << 3; //保存AAC Data的数据大小 低5位 最多只能保存13bit

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
   * 如果采样频率是44100
   * 一般AAC每个1024个采样为一帧
   * 所以一秒就有 44100 / 1024 = 43帧
   * 时间增量就是 44100 / 43 = 1025
   * 一帧的时间为 1 / 43 = 23ms

   * 如果采样频率是32000
   * 一般AAC每个1024个采样为一帧
   * 所以一秒就有 32000 / 1024 = 31.25帧
   * 时间增量就是 32000 / 31.25 = 1024
   * 一帧的时间为 1 / 31.25 = 32ms
   */
  rtpPacket->rtpHeader.timestamp += 1024;

  return 0;
}

//写成内联函数减少开销
static inline int startCode3(char* buf)
{
  if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1)
    return 1;
  else
    return 0;
}

static inline int startCode4(char* buf)
{
  if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1)
    return 1;
  else
    return 0;
}

static char* findNextStartCode(char* buf, int len)
{
  int i;

  if (len < 3)
    return NULL;

  for (i = 0; i < len - 3; ++i)
  {
    if (startCode3(buf) || startCode4(buf))
      return buf;

    ++buf;
  }

  if (startCode3(buf))
    return buf;

  return NULL;
}

static int getFrameFromH264File(FILE* fd, char* frame, int size)
{
  int rSize, frameSize;
  char* nextStartCode;

  if (fd < 0)
    return -1;

  rSize = fread(frame, 1, size, fd);
  if (!startCode3(frame) && !startCode4(frame))
    return -1;

  nextStartCode = findNextStartCode(frame + 3, rSize - 3);
  if (!nextStartCode)
  {
    fseek(fd, 0, SEEK_SET);
    frameSize = rSize;
  }
  else
  {
    frameSize = (nextStartCode - frame);
    fseek(fd, frameSize - rSize, SEEK_CUR);
  }

  return frameSize;
}

static int rtpSendH264Frame(int socket, char* ip, int16_t port,
  struct RtpPacket* rtpPacket, uint8_t* frame, uint32_t frameSize)
{
  uint8_t naluType; // nalu第一个字节
  int sendBytes = 0;
  int ret;

  naluType = frame[0];

  if (frameSize <= RTP_MAX_PKT_SIZE) // nalu长度小于最大包场：单一NALU单元模式
  {
    /*
     *   0 1 2 3 4 5 6 7 8 9
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *  |F|NRI|  Type   | a single NAL unit ... |
     *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     */
    memcpy(rtpPacket->payload, frame, frameSize);
    ret = rtpSendPacket(socket, ip, port, rtpPacket, frameSize);
    if (ret < 0)
      return -1;

    rtpPacket->rtpHeader.seq++;
    sendBytes += ret;
    if ((naluType & 0x1F) == 7 || (naluType & 0x1F) == 8) // 如果是SPS、PPS就不需要加时间戳
      goto out;
  }
  else // nalu长度小于最大包场：分片模式
  {
    /*
     *  0                   1                   2
     *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * | FU indicator  |   FU header   |   FU payload   ...  |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     */

     /*
      *     FU Indicator
      *    0 1 2 3 4 5 6 7
      *   +-+-+-+-+-+-+-+-+
      *   |F|NRI|  Type   |
      *   +---------------+
      */

      /*
       *      FU Header
       *    0 1 2 3 4 5 6 7
       *   +-+-+-+-+-+-+-+-+
       *   |S|E|R|  Type   |
       *   +---------------+
       */

    int pktNum = frameSize / RTP_MAX_PKT_SIZE;       // 有几个完整的包
    int remainPktSize = frameSize % RTP_MAX_PKT_SIZE; // 剩余不完整包的大小
    int i, pos = 1;

    /* 发送完整的包 */
    for (i = 0; i < pktNum; i++)
    {
      rtpPacket->payload[0] = (naluType & 0x60) | 28;
      rtpPacket->payload[1] = naluType & 0x1F;

      if (i == 0) //第一包数据
        rtpPacket->payload[1] |= 0x80; // start
      else if (remainPktSize == 0 && i == pktNum - 1) //最后一包数据
        rtpPacket->payload[1] |= 0x40; // end

      memcpy(rtpPacket->payload + 2, frame + pos, RTP_MAX_PKT_SIZE);
      ret = rtpSendPacket(socket, ip, port, rtpPacket, RTP_MAX_PKT_SIZE + 2);
      if (ret < 0)
        return -1;

      rtpPacket->rtpHeader.seq++;
      sendBytes += ret;
      pos += RTP_MAX_PKT_SIZE;
    }

    /* 发送剩余的数据 */
    if (remainPktSize > 0)
    {
      rtpPacket->payload[0] = (naluType & 0x60) | 28;
      rtpPacket->payload[1] = naluType & 0x1F;
      rtpPacket->payload[1] |= 0x40; //end

      memcpy(rtpPacket->payload + 2, frame + pos, remainPktSize + 2);
      ret = rtpSendPacket(socket, ip, port, rtpPacket, remainPktSize + 2);
      if (ret < 0)
        return -1;

      rtpPacket->rtpHeader.seq++;
      sendBytes += ret;
    }
  }

out:

  return sendBytes;
}

void sendH264(int16_t _port) {
  int socket;
  FILE* fd;
  int fps = 25;
  int startCode;
  struct RtpPacket* rtpPacket;
  uint8_t* frame;
  uint32_t frameSize;
  char* fileName;
  int16_t port = _port;

  fd = fopen(H264_FILE_NAME, "rb");
  if (fd < 0)
  {
    printf("failed to open %s\n", H264_FILE_NAME);
  }

  socket = createUdpSocket();
  if (socket < 0)
  {
    printf("failed to create socket\n");
  }
  // 加入到组播
  //struct ip_mreq op;  //ip_mreq和ip_mreqn都有相同的效果， 后者是更新的版本多了一个设置级别ifindex，
  ////但是前者依然可以使用
  //op.imr_interface.s_addr = INADDR_ANY; // 本地地址
  //inet_pton(AF_INET, "127.0.0.1", &op.imr_multiaddr.s_addr);
  ////加入组播
  //setsockopt(socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&op, sizeof(op));

  rtpPacket = (struct RtpPacket*)malloc(500000);
  frame = (uint8_t*)malloc(500000);

  rtpHeaderInit(rtpPacket, 0, 0, 0, RTP_VESION, RTP_PAYLOAD_TYPE_H264, 0,
    0, 0, 0x88923423);

  while (1)
  {
    std::cout << "timestamp : " << GetTickCount64() << std::endl;
    frameSize = getFrameFromH264File(fd, (char*)frame, 500000);
    if (frameSize < 0)
    {
      printf("read err\n");
      continue;
    }

    if (startCode3((char*)frame))
      startCode = 3;
    else
      startCode = 4;

    frameSize -= startCode;
    rtpSendH264Frame(socket, CLIENT_IP, port,
      rtpPacket, frame + startCode, frameSize);
    //for (int i = 0; i < 1000; i++) {
    //  std::cout << (int)rtpPacket->payload[i] << " ";
    //}
    //std::cout << std::endl;
    //std::cout << std::endl;
    rtpPacket->rtpHeader.timestamp += 90000 / FPS;

    std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps));
  }

  free(rtpPacket);
  free(frame);
}

void sendAAC(int16_t _port) {
  FILE* fd;
  int ret;
  int socket;
  uint8_t* frame;
  struct AdtsHeader adtsHeader;
  struct RtpPacket* rtpPacket;
  fd = fopen(AAC_FILE, "rb");
  int16_t port = _port;
  if (fd == nullptr)
  {
    printf("failed to open %s\n", AAC_FILE);
  }

  socket = createUdpSocket();
  if (socket < 0)
  {
    printf("failed to create udp socket\n");
  }
  frame = (uint8_t*)malloc(5000);
  rtpPacket = (RtpPacket*)malloc(5000);

  rtpHeaderInit(rtpPacket, 0, 0, 0, RTP_VESION, RTP_PAYLOAD_TYPE_AAC, 1, 0, 0, 0x32411);

  while (1)
  {
    //读adts header
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
    //读aac data
    ret = fread(frame, 1, adtsHeader.aacFrameLength - 7, fd);
    if (ret < 0)
    {
      printf("read err\n");
      break;
    }

    rtpSendAACFrame(socket, "127.0.0.1", port,
      rtpPacket, frame, adtsHeader.aacFrameLength - 7);

    std::this_thread::sleep_for(std::chrono::milliseconds(25));
  }
  fclose(fd);
  //释放资源
  closesocket(socket);
  WSACleanup();

  free(frame);
  free(rtpPacket);
}



int main(int argc, char* argv[])
{
  int16_t portv;
  int16_t porta;

  std::cout << "please input this audio's port" << std::endl;
  while (std::cin >> porta) {
    if (getchar() == '\n')
      break;
  }
  std::cout << "port is : " << porta << std::endl;

  std::cout << "please input this video's port" << std::endl;
  while (std::cin >> portv) {
    if (getchar() == '\n')
      break;
  }
  std::cout << "port is : " << portv << std::endl;

  std::thread sendaac(sendAAC, porta);
  std::thread sendh264(sendH264, portv);
  sendaac.detach();
  sendh264.join();

  return 0;
}