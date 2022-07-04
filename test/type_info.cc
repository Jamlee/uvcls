#include "gtest/gtest.h"
#include "type_info.hpp"
#include <iostream>

class HelloWorld {};

TEST(type_info, type) {
  auto id = uvcls::type<HelloWorld>();
  // std::cout << id << std::endl;
  // std::cout << __PRETTY_FUNCTION__ << std::endl;
  EXPECT_EQ(id, 3269991986);
}