#include <type_traits>
#include <iostream>
#include "gtest/gtest.h"
#include "idle.hpp"
#include "request.hpp"
#include "stream.hpp"

TEST(Idle, Run) {
    auto loop = uvcls::Loop::getDefault();
    auto idle = std::make_shared<uvcls::IdleHandle>(loop);
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
    auto tcp = std::make_shared<uvcls::Request>(loop);
    loop->run();
}

TEST(Stream, Demo) {
    auto loop = uvcls::Loop::getDefault();
    auto tcp = std::make_shared<uvcls::StreamHandle>(loop);
    loop->run();
}

// TEST(TCP, Functionalities) {
//     auto loop = uvcls::Loop::getDefault();
//     auto tcp = std::make_shared<uvcls::TCPHandle>(loop, 0);
//     ASSERT_TRUE(tcp->noDelay(true));
//     ASSERT_TRUE(tcp->keepAlive(true, uvcls::TCPHandle::Time{128}));
//     ASSERT_TRUE(tcp->simultaneousAccepts());
//     tcp->close();
//     loop->run();
// }
