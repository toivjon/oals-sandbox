// ============================================================================
//
// Some useful resources:
// http://openal.org/documentation/OpenAL_Programmers_Guide.pdf
//
// A note about OpenAL naming conventions:
//    AL  --- Core-specific functions.
//    ALC --- Device-specific functions.
//
// ============================================================================

#include <cstdio>
#include <cstdlib>

#include <AL/al.h>
#include <AL/alc.h>

/*
static bool hasAlError()
{
  auto code = alGetError();
  printf("OpenAL AL error: %s\n", alGetString(code));
  return (code != AL_NO_ERROR);
}
*/

// ============================================================================
// A helper function to check whether there's been an error with the ALC queue.
// @param device The target ALC device.
// ============================================================================
static bool hasAlcError(ALCdevice* device)
{
  auto code = alcGetError(device);
  auto hasError = (code != ALC_NO_ERROR);
  if (code != ALC_NO_ERROR) {
    printf("OpenAL ALC error: %s\n", alcGetString(device, code));
  }
  return hasError;
}

// ============================================================================

int main()
{
  // ==========================================================================
  // OPEN A DEVICE
  // The first thing to do in OpenAL is to open a device (nullptr = default).
  // ==========================================================================
  auto device = alcOpenDevice(nullptr);
  if (device == nullptr || hasAlcError(device)) {
    printf("alcOpenDevice failed: Unable to open device.\n");
    exit(EXIT_FAILURE);
  }

  // ==========================================================================
  // CREATE A CONTEXT
  // The second thing is to create a context for the sound device.
  // ==========================================================================
  auto ctx = alcCreateContext(device, nullptr);
  if (ctx == nullptr || hasAlcError(device)) {
    printf("alcCreateContext failed: Unable to create device context.\n");
    alcCloseDevice(device);
    exit(EXIT_FAILURE);
  }

  // ==========================================================================
  // ACTIVATE CONTEXT
  // Define the created context as the active context.
  // ==========================================================================
  if (alcMakeContextCurrent(ctx) == ALC_FALSE || hasAlcError(device)) {
    printf("alcMakeContextCurrent failed: Unable to set active context.\n");
    alcDestroyContext(ctx);
    alcCloseDevice(device);
    exit(EXIT_FAILURE);
  }

  alcMakeContextCurrent(nullptr);
  alcDestroyContext(ctx);
  alcCloseDevice(device);
  return EXIT_SUCCESS;
}
