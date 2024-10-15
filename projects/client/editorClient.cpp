
#include "asio.hpp"
#include <iostream>
#include <functional>


using asio::ip::tcp;

class Client
{
public:
	Client(asio::io_context &io_context, const std::string &host, const std::string &port)
		: socket_(io_context), resolver_(io_context), strand_(io_context), running_(true)
	{
		connect(host, port);
	}

	~Client()
	{
		running_ = false;
		if (socket_.is_open())
		{
			socket_.close();
		}
	}

	void BindOnMessage(std::function<void(std::string)> callback){
		onMessage_ = callback;
	}

private:
	void connect(const std::string &host, const std::string &port)
	{
		auto endpoints = resolver_.resolve(host, port);
		asio::async_connect(socket_, endpoints,
							[this](std::error_code ec, asio::ip::tcp::endpoint)
							{
								if (!ec)
								{
									std::cout << "Connected successfully!\n";
									read_messages();
								}
								else
								{
									std::cerr << "Connection failed: " << ec.message() << "\n";
								}
							});
	}

	void read_messages()
	{
		asio::async_read_until(socket_, asio::dynamic_buffer(read_buffer_), '\n',
							   asio::bind_executor(strand_, [this](std::error_code ec, std::size_t bytes_transferred)
												   {
                if (!ec && running_) {
                    std::string message(read_buffer_.substr(0, bytes_transferred - 1));
                    std::cout << "Received: " << message << std::endl;
					if(onMessage_){
						onMessage_(message);
					}
                    read_buffer_.erase(0, bytes_transferred);
                    read_messages(); // Continue reading
                } else if (ec) {
                    std::cerr << "Read error: " << ec.message() << "\n";
                } }));
	}

	asio::ip::tcp::socket socket_;
	asio::ip::tcp::resolver resolver_;
	asio::io_context::strand strand_;
	std::string read_buffer_;
	std::atomic<bool> running_;
	std::function<void(std::string)> onMessage_;
};
