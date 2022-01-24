
#define ASIO_STANDALONE	
#define _WIN32_WINNT  0x0601 



#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>
#include <ctime>
#include "asio.hpp"


using namespace std;

//port is 2300
asio::ip::udp::endpoint ep(asio::ip::address::from_string("127.0.0.1"), 2300);


class noncopyable {
protected:
	constexpr noncopyable() = default;
	~noncopyable() = default;
	noncopyable(const noncopyable&) = delete;
	noncopyable& operator= (const noncopyable&) = delete;
};



class talk_to_svr : public enable_shared_from_this<talk_to_svr>, noncopyable
{

public:
	talk_to_svr(asio::io_service& io_context, const std::string& message)
		: sock_(io_context, asio::ip::udp::endpoint(asio::ip::udp::v4(), 0)), started_(true), message_(message)
	{
		
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		start();
	}


	typedef shared_ptr<talk_to_svr> ptr;

	void start() {
		do_write(message_);
	}

	bool started() { return started_; }
private:
	void on_read(const error_code& err, size_t bytes) {
		if (!err) {
			std::string copy(read_buffer_, bytes);
			std::cout << "server echoed us " << copy << std::endl;
			//<< (copy == message_ ? "OK" : "FAIL") << std::endl;
		}
		start();
	}
	void on_write(const error_code& err, size_t bytes) {
		printf("client write result:%d, bytes:%d \n", err.value(), bytes);
		do_read();
	}
	void do_read() {
		sock_.async_receive_from(asio::buffer(read_buffer_), sender_ep,
			bind(&talk_to_svr::on_read,
				this,
				std::placeholders::_1,
				std::placeholders::_2));
	}
	void do_write(const std::string& msg) {
		std::copy(msg.begin(), msg.end(), write_buffer_);
		sock_.async_send_to(asio::buffer(write_buffer_, msg.size()), ep,
			bind(&talk_to_svr::on_write,
				this,
				std::placeholders::_1,
				std::placeholders::_2));
	}
	void Timer1Sec() {
		start();
		std::cout << "Timer1Sec." << std::endl;
	}

private:
	asio::ip::udp::socket sock_;
	asio::ip::udp::endpoint sender_ep;
	enum { max_msg = 1024 };
	char read_buffer_[max_msg];
	char write_buffer_[max_msg];
	bool started_;
	std::string message_;
};





int main(int argc, char* argv[])
{

	asio::io_service io_service;
	talk_to_svr client(io_service, "hello");

	io_service.run();

	system("pause");
}