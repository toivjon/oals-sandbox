// ============================================================================
//
// Some useful resources:
// http://openal.org/documentation/OpenAL_Programmers_Guide.pdf
//
// ============================================================================

#include <cstdio>
#include <cstdlib>

#include <AL/al.h>
#include <AL/alc.h>

int main()
{
  // ==========================================================================
  // OPEN A DEVICE
  // The first thing to do in OpenAL is to open a device (nullptr = default).
  // ==========================================================================
  auto device = alcOpenDevice(nullptr);
  if (device == nullptr) {
    printf("alcOpenDevice failed: Unable to open device.\n");
    exit(EXIT_FAILURE);
  }

  // ==========================================================================
  // CREATE A CONTEXT
  // The second thing is to create a context for the sound device.
  // ==========================================================================
  auto ctx = alcCreateContext(device, nullptr);
  if (ctx == nullptr) {
    printf("alcCreateContext failed: Unable to create device context.\n");
    alcCloseDevice(device);
    exit(EXIT_FAILURE);
  }

  alcDestroyContext(ctx);
  alcCloseDevice(device);
  return 0;
}
