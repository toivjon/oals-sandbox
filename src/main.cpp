#include <AL/alc.h>

int main()
{
  auto device = alcOpenDevice(nullptr);
  alcCloseDevice(device);
  return 0;
}
