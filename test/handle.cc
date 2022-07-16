#include <type_traits>
#include <iostream>
#include "gtest/gtest.h"
#include "idle.hpp"
#include "request.hpp"
#include "stream.hpp"
#include "tcp.hpp"

TEST(Idle, Run) {
    auto loop = uvcls::Loop::getDefault();
    auto idle = std::make_shared<uvcls::IdleHandle>(loop->shared_from_this());
    idle->on<uvcls::IdleEvent>([](const auto &, auto &hndl) {
        std::cout << "idle event" << std::endl;
        hndl.stop();
        hndl.close();
    });
    idle->on<uvcls::CloseEvent>([](const auto &, auto &hndl) {
        std::cout << "close event" << std::endl;
    });
    
    idle->init();
    idle->start();
    loop->run();
}

TEST(Request, Demo) {
    auto loop = uvcls::Loop::getDefault();
    auto shutdown = std::make_shared<uvcls::ConnectReq>(loop->shared_from_this());
    loop->run();
}

TEST(Stream, Demo) {
    auto loop = uvcls::Loop::getDefault();
    auto tcp = std::make_shared<uvcls::TCPHandle>(loop->shared_from_this(), 0);
    loop->run();
}

TEST(TCP, Functionalities) {
    auto loop = uvcls::Loop::getDefault();
    auto tcp = std::make_shared<uvcls::TCPHandle>(loop->shared_from_this(), 0);
    tcp->init();
    tcp->noDelay(true);
    tcp->keepAlive(true, uvcls::TCPHandle::Time{128});
    tcp->simultaneousAccepts();
    tcp->close();
    loop->run();
}
