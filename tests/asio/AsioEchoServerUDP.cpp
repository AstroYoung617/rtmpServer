#define ASIO_STANDALONE	
#define _WIN32_WINNT  0x0601 



#include <cstdlib>
#include <iostream>


#include "asio.hpp"

using asio::ip::udp;
using namespace std;

class server
{
public:
	server(asio::io_context& io_context, short port)
		: socket_(io_context, udp::endpoint(udp::v4(), port))
	{
		socket_.async_receive_from(
			asio::buffer(data_, max_length), sender_endpoint_,
			bind(&server::handle_receive_from, this,
				placeholders::_1,
				placeholders::_2));
	}

	void handle_receive_from(const asio::error_code& error,
		size_t bytes_recvd)
	{
		if (!error && bytes_recvd > 0)
		{
			printf("received:%d, content:[%s]\n", bytes_recvd, data_);

			socket_.async_send_to(
				asio::buffer(data_, bytes_recvd), sender_endpoint_,
				bind(&server::handle_send_to, this,
					placeholders::_1,
					placeholders::_2));
		}
		else
		{
			socket_.async_receive_from(
				asio::buffer(data_, max_length), sender_endpoint_,// this->handle_receive_from);
				bind(&server::handle_receive_from, this,
					placeholders::_1,
					placeholders::_2));
		}
	}

	void handle_send_to(const asio::error_code& /*error*/,
		size_t /*bytes_sent*/)
	{
		socket_.async_receive_from(
			asio::buffer(data_, max_length), sender_endpoint_,
			bind(&server::handle_receive_from, this,
				placeholders::_1,
				placeholders::_2));
	}

private:
	udp::socket socket_;
	udp::endpoint sender_endpoint_;
	enum { max_length = 1024 };
	char data_[max_length];
};

int main(int argc, char* argv[])
{
	try
	{
		if (argc > 2)
		{
			std::cerr << "Usage: async_udp_echo_server <port>\n";
			return 1;
		}

		asio::io_context io_context;

		using namespace std; // For atoi.
		server s(io_context, atoi("2300"));

		io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;

}
