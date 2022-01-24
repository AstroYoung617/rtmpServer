//
// blocking_tcp_echo_client.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//asio �ٷ�ʾ�� client

#include<sys/types.h>
#include <cstdlib>
//#include <unistd.h>
#include <cstring>
#include <iostream>
#include <asio.hpp>
#include <string.h>  


using namespace std;
using std::stringstream;
using asio::ip::tcp;


enum { max_length = 1024 };

int main(int argc, char* argv[])
{
  try
  {
    const char* host = "localhost";
    //�˿�д��Ϊ5000
    const char* port = "5000";
    asio::io_context io_context;

    tcp::socket s(io_context);
    tcp::resolver resolver(io_context);
    asio::connect(s, resolver.resolve(host, port));
    int id = (int)getpid();
    int counter = 0;
    while (true)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      //ÿ���1000ms����stringstream�ı���Ϣ
      //�����ı���Ϣ
      stringstream fmt;
      fmt << "�ͻ���[" << id << "]: counter->" << counter++;

      //����asio������Ϣ
      string msg = fmt.str();
      size_t request_length = msg.length();
      asio::write(s, asio::buffer(msg.data(), request_length));

      //����reply�ı���Ϣ��ֱ�����������̨
      char reply[max_length];
      size_t reply_length = asio::read(s,
        asio::buffer(reply, strlen(msg.data())));
      std::cout << "Reply is: ";
      std::cout.write(reply, reply_length);
      std::cout << "\n";
    }
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}