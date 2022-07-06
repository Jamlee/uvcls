#include <iostream>
#include <unistd.h> /* readlink, usleep */
#include <sys/time.h>

void uv_sleep(int msec) {
  int sec;
  int usec;

  sec = msec / 1000;
  usec = (msec % 1000) * 1000;
  if (sec > 0)
    sleep(sec);
  if (usec > 0)
    usleep(usec);
}


int main() {
    std::cout << "hello" <<std::endl;
    uv_sleep(3000);
    std::cout << "hello" <<std::endl;
    return 0;
}