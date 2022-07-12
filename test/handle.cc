#include <type_traits>
#include <iostream>
#include "gtest/gtest.h"
#include "handle.hpp"
#include "idle.hpp"

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

TEST(TCP, Functionalities) {
    auto loop = uvcls::Loop::getDefault();
    auto handle = loop->resource<uvw::TCPHandle>();

    ASSERT_TRUE(handle->noDelay(true));
    ASSERT_TRUE(handle->keepAlive(true, uvw::TCPHandle::Time{128}));
    ASSERT_TRUE(handle->simultaneousAccepts());

    handle->close();
    loop->run();
}
