//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <stdio.h>
#include <iostream>
#include <memory>
#include <utility>
#include <asio.hpp>
#include <string.h>
using namespace std;

using asio::ip::tcp;

class session
  : public std::enable_shared_from_this<session>
{
public:
  session(tcp::socket socket)
    : socket_(std::move(socket))
  {
  }

  void start()
  {
    do_read();
  }

private:
  void do_read()
  {
    auto self(shared_from_this());

    socket_.async_read_some(asio::buffer(data_, max_length),
      [this, self](std::error_code ec, std::size_t length)
      {
        if (!ec)
        {
          do_write(length);
        }
      });
  }

  void do_write(std::size_t length)
  {
    auto self(shared_from_this());

    string msg;
    for (int i = 0; i < length; i++)
      msg += data_[i];

    string addr = socket_.remote_endpoint().address().to_string();
    printf("Sender:%s Msg: %s\n", addr.data(), msg.data());
    asio::async_write(socket_, asio::buffer(data_, length),
      [this, self](std::error_code ec, std::size_t /*length*/)
      {
        if (!ec)
        {
          do_read();
        }
      });
  }

  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
};

class server
{
public:
  server(asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(
      [this](std::error_code ec, tcp::socket socket)
      {
        if (!ec)
        {
          std::make_shared<session>(std::move(socket))->start();
        }

        do_accept();
      });
  }

  tcp::acceptor acceptor_;
};

int main(int argc, char* argv[])
{
 
  try
  {
    asio::io_context io_context;

    server s(io_context, std::atoi("5000"));

    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}