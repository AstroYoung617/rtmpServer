//
// blocking_tcp_echo_client.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//asio 官方示例 client

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
    //端口写死为5000
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
      //每间隔1000ms发送stringstream文本消息
      //构建文本消息
      stringstream fmt;
      fmt << "客户端[" << id << "]: counter->" << counter++;

      //构建asio发送消息
      string msg = fmt.str();
      size_t request_length = msg.length();
      asio::write(s, asio::buffer(msg.data(), request_length));

      //构建reply文本消息，直接输出到控制台
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