#include <stdio.h> 
#include <winsock2.h> 

#pragma comment(lib,"ws2_32.lib")  

int main(int argc, char* argv[])
{
  //与客户端相同，加载winsock
  WSADATA wsaData;
  WORD sockVersion = MAKEWORD(2, 2);
  if (WSAStartup(sockVersion, &wsaData) != 0)
  {
    return 0;
  }
  //构造监听socket 与tcp的差别在第二个参数，tcp是基于stream的，udp是基于数据报的
  SOCKET serSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (serSocket == INVALID_SOCKET)
  {
    printf("socket error !");
    return 0;
  }

  sockaddr_in serAddr;
  serAddr.sin_family = AF_INET;
  serAddr.sin_port = htons(8888);
  serAddr.sin_addr.S_un.S_addr = INADDR_ANY;
  if (bind(serSocket, (sockaddr*)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
  {
    printf("bind error !");
    closesocket(serSocket);
    return 0;
  }

  sockaddr_in remoteAddr;
  int nAddrLen = sizeof(remoteAddr);
  //进行轮询
  while (true)
  {
    char recvData[255];
    int ret = recvfrom(serSocket, recvData, 255, 0, (sockaddr*)&remoteAddr, &nAddrLen);
    if (ret > 0)
    {
      recvData[ret] = 0x00;
      printf("接受到一个连接：%s \r\n", inet_ntoa(remoteAddr.sin_addr));
      printf(recvData);
    }

    const char* sendData = "一个来自服务端的UDP数据包\n";
    //udp使用sendto
    sendto(serSocket, sendData, strlen(sendData), 0, (sockaddr*)&remoteAddr, nAddrLen);

  }
  closesocket(serSocket);
  WSACleanup();
  return 0;
}