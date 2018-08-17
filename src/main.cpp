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
#include <AL/alext.h>

#include "wave.h"

// ============================================================================
// A helper function to check whether there's been an error with the AL queue.
// ============================================================================
static bool hasAlError()
{
  auto code = alGetError();
  auto hasError = (code != ALC_NO_ERROR);
  if (code != ALC_NO_ERROR) {
    printf("OpenAL AL error: %s\n", alGetString(code));
  }
  return hasError;
}

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

  // ==========================================================================
  // CREATE BUFFER(S)
  // Create AL buffer(s) to store audio data for the playback.
  // ==========================================================================
  ALuint buffer;
  alGenBuffers(1, &buffer);
  if (hasAlError()) {
    printf("alGenBuffers failed: Unable to set active context.\n");
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(ctx);
    alcCloseDevice(device);
    exit(EXIT_FAILURE);
  }

  // ==========================================================================
  // LOAD SOUND DATA
  // Load the actual sound data from any sound data source.
  // ==========================================================================
  auto file = wave_load("test.wav");
  if (file.riff.chunkSize == 0) {
    printf("wave_load failed: Unable to load test.wav file.\n");
    alDeleteBuffers(1, &buffer);
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(ctx);
    alcCloseDevice(device);
  }

  // ==========================================================================
  // DEFINE BUFFER DATA
  // Copy data from the sound data container into the AL buffer.
  // ==========================================================================
  auto format = AL_FORMAT_MONO8; // TODO
  auto frequency = file.fmt.sampleRate;
  auto dataSize = file.data.data.size();
  alBufferData(buffer, format, file.data.data.data(), dataSize, frequency);
  if (hasAlError()) {
    printf("alBufferData failed: Unable to set buffer data.\n");
    alDeleteBuffers(1, &buffer);
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(ctx);
    alcCloseDevice(device);
  }

  // ==========================================================================
  // CREATE SOURCE(S)
  // Create source(s) which are used to control the playback.
  // ==========================================================================
  ALuint source;
  alGenSources(1, &source);
  if (hasAlError()) {
    printf("alBufferalGenSourcesData failed: Unable to create a source.\n");
    alDeleteBuffers(1, &buffer);
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(ctx);
    alcCloseDevice(device);
  }

  // ==========================================================================
  // ASSIGN BUFFER TO SOURCE
  // Assign a buffer containing the sound data to a source.
  // ==========================================================================
  alSourcei(source, AL_BUFFER, buffer);
  if (hasAlError()) {
    printf("alBufferalGenSourcesData failed: Unable to create a source.\n");
    alDeleteSources(1, &source);
    alDeleteBuffers(1, &buffer);
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(ctx);
    alcCloseDevice(device);
  }

  alDeleteSources(1, &source);
  alDeleteBuffers(1, &buffer);
  alcMakeContextCurrent(nullptr);
  alcDestroyContext(ctx);
  alcCloseDevice(device);
  return EXIT_SUCCESS;
}
