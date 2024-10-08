
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <string>
#include <efsw/efsw.hpp>

class UpdateListener : public efsw::FileWatchListener
{
public:
  void handleFileAction(efsw::WatchID watchid, const std::string &dir,
                        const std::string &filename, efsw::Action action,
                        std::string oldFilename) override
  {
    switch (action)
    {
    case efsw::Actions::Add:
      std::cout << "DIR (" << dir << ") FILE (" << filename << ") has event Added"
                << std::endl;
      break;
    case efsw::Actions::Delete:
      std::cout << "DIR (" << dir << ") FILE (" << filename << ") has event Delete"
                << std::endl;
      break;
    case efsw::Actions::Modified:
      std::cout << "DIR (" << dir << ") FILE (" << filename << ") has event Modified"
                << std::endl;
      break;
    case efsw::Actions::Moved:
      std::cout << "DIR (" << dir << ") FILE (" << filename << ") has event Moved from ("
                << oldFilename << ")" << std::endl;
      break;
    default:
      std::cout << "Should never happen!" << std::endl;
    }
  }
};

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
  enum
  {
    max_length = 1024
  };
  char data_[max_length];
};

class server
{
public:
  server(asio::io_context &io_context, short port)
      : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
        socket_(io_context)
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(socket_,
                           [this](std::error_code ec)
                           {
                             if (!ec)
                             {
                               std::make_shared<session>(std::move(socket_))->start();
                             }

                             do_accept();
                           });
  }

  tcp::acceptor acceptor_;
  tcp::socket socket_;
};

int main(int argc, char *argv[])
{

  using namespace std::string_literals;
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: async_tcp_echo_server <port>\n";
      return 1;
    }

    efsw::FileWatcher *fileWatcher = new efsw::FileWatcher();

    // Create the instance of your efsw::FileWatcherListener implementation
    UpdateListener *listener = new UpdateListener();

    // Add a folder to watch, and get the efsw::WatchID
    // It will watch the /tmp folder recursively ( the third parameter indicates that is recursive )
    // Reporting the files and directories changes to the instance of the listener
    efsw::WatchID watchID = fileWatcher->addWatch("C:/Users/turbo/Desktop", listener, true);

    fileWatcher->watch();

    asio::io_context io_context;

    server s(io_context, std::atoi(argv[1]));

    io_context.run();
  }
  catch (std::exception &e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}