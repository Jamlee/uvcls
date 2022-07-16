#include <iostream>
#include "tcp.hpp"

int main()
{
    const std::string address = std::string{"127.0.0.1"};
    const unsigned int port = 4242;
    auto loop = uvcls::Loop::getDefault();

    auto server = std::make_shared<uvcls::TCPHandle>(loop, 0);
    server->init();
    server->noDelay(true);
    server->keepAlive(true, uvcls::TCPHandle::Time{128});
    server->on<uvcls::ErrorEvent>([](const auto &, auto &)
                                  { std::cout << "tcp error" << std::endl; });

    server->once<uvcls::ListenEvent>([&loop](const uvcls::ListenEvent &, uvcls::TCPHandle &handle)
                                     {
        auto socket = std::make_shared<uvcls::TCPHandle>(loop, 0);
        socket->init();
        socket->on<uvcls::ErrorEvent>([](const uvcls::ErrorEvent &, uvcls::TCPHandle &) { 
            std::cout << "tcp error" << std::endl;
        });
        socket->on<uvcls::CloseEvent>([&handle](const uvcls::CloseEvent &, uvcls::TCPHandle &) { handle.close(); });
        socket->on<uvcls::EndEvent>([](const uvcls::EndEvent &, uvcls::TCPHandle &sock) { sock.close(); });
        socket->on<uvcls::DataEvent>([](const uvcls::DataEvent &data, uvcls::TCPHandle &sock) {
            for(int i=0 ; i < data.length ; ++i)
            {
                std::cout << data.data[i];
            }
            std::cout << std::endl;
        });
        handle.accept(*socket);
        socket->read(); });

    server->bind(address, port);
    server->listen();
    loop->run();
}
