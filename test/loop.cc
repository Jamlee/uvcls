#include <type_traits>
#include <iostream>
#include "gtest/gtest.h"
#include "loop.hpp"

// ErrorEvent 事件用于封装 uv 的 error 事件
TEST(Loop, Run) {
    auto loop = uvcls::Loop::getDefault();
    loop->run();
}