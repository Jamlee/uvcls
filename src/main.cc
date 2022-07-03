// https://github.com/skypjack/uvw/blob/master/src/uvw/loop.h
// https://github.com/skypjack/uvw/blob/master/test/main.cpp

#include "loop.h"
#include "uv.h"

using namespace uvcls;

int main() {
  auto loop = Loop::getDefault();
  (*loop).TCPListen();
  return 0;
}
