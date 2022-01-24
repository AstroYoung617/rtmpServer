// test : 测试发送RTP视频数据，采用ffplay播放该RTP视频流，H264封装


#include <iostream>
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

#define H264_FILE_NAME  "./test.h264"
#define CLIENT_IP       "127.0.0.1"   //destination ip
#define CLIENT_PORT     1234              //destination port

#define FPS             25                


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

static int createUdpSocket()
{
  int fd;
  int on = 1;
  //加载winsock 无论是客户端还是服务器都需要
  WORD socketVersion = MAKEWORD(2, 2);
  WSADATA wsaData;
  //初始化socket资源
  if (WSAStartup(socketVersion, &wsaData) != 0)
  {
    return 0;
  }
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0)
    return -1;

  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

  return fd;
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

int main(int argc, char* argv[])
{
  int socket;
  FILE* fd;
  int fps = 25;
  int startCode;
  struct RtpPacket* rtpPacket;
  uint8_t* frame;
  uint32_t frameSize;
  char* fileName;
  int16_t port;

  std::cout << "please input this video's port" << std::endl;
  while (std::cin >> port) {
    if (getchar() == '\n')
      break;
  }
  std::cout << "port is : " << port << std::endl;

  fd = fopen(H264_FILE_NAME, "rb");
  if (fd < 0)
  {
    printf("failed to open %s\n", H264_FILE_NAME);
    return -1;
  }

  socket = createUdpSocket();
  if (socket < 0)
  {
    printf("failed to create socket\n");
    return -1;
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
    for (int i = 0; i < 1000; i++) {
      std::cout << (int)rtpPacket->payload[i] << " ";
    }
    std::cout << std::endl;
    std::cout << std::endl;
    rtpPacket->rtpHeader.timestamp += 90000 / FPS;

    std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps));
  }

  free(rtpPacket);
  free(frame);

  return 0;
}