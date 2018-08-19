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

#include <vorbis/vorbisfile.h>

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
  //
  // Here we use Vorbis data from a OGG container as the sound source. The main
  // thing in here is the OggVorbis_File structure which contains the basic
  // file information (pointer to physical file/bitstream and info about that).
  //
  // OggVorbis_File:s can be opened in three different ways:
  //   1. ov_fopen............Opens the specified file with default values.
  //   2. ov_open.............Opens with the given file handle. [NON-WINDOWS!]
  //   3. ov_open_callbacks...Opens with custom manipulation routines.
  // ==========================================================================
  OggVorbis_File ovFile;
  auto result = ov_fopen("test.ogg", &ovFile);
  if (result != 0) {
    printf("ov_fopen failed: Failed to open test.ogg file.\n");
    alDeleteBuffers(1, &buffer);
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(ctx);
    alcCloseDevice(device);
    exit(EXIT_FAILURE);
  }

  // ==========================================================================
  // GET MEMORY DATA
  // Get the memory data from the sound file into memory.
  // ==========================================================================
  std::vector<char> oggBuffer;
  char ovBuffer[4096];
  auto eof = 0;
  auto currentSection = 0;
  while (!eof) {
    auto ret = ov_read(&ovFile, ovBuffer, sizeof(ovBuffer),0, 2, 1, &currentSection);
    if (ret == 0) {
      eof = 1;
    } else if (ret < 0) {
      // error
    } else {
      for (auto i = 0; i < ret; i++) {
        oggBuffer.push_back(ovBuffer[i]);
      }
    }
  }

  // ==========================================================================
  // DEFINE BUFFER DATA
  // Copy data from the sound data container into the AL buffer.
  // ==========================================================================
  auto format = AL_FORMAT_MONO16; // TODO
  auto frequency = ovFile.vi->rate;
  auto dataSize = oggBuffer.size();
  ov_clear(&ovFile);
  alBufferData(buffer, format, oggBuffer.data(), dataSize, frequency);
  if (hasAlError()) {
    printf("alBufferData failed: Unable to set buffer data.\n");
    alDeleteBuffers(1, &buffer);
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(ctx);
    alcCloseDevice(device);
    exit(EXIT_FAILURE);
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
    exit(EXIT_FAILURE);
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
    exit(EXIT_FAILURE);
  }

  // ==========================================================================
  // PLAY THE SOUND
  // Here we actually start to play the sound.
  // ==========================================================================
  alSourcePlay(source);
  printf("Playing sound test.wav\n");
  if (hasAlError()) {
    printf("alSourcePlay failed: Unable to play the specified source.\n");
    alDeleteSources(1, &source);
    alDeleteBuffers(1, &buffer);
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(ctx);
    alcCloseDevice(device);
    exit(EXIT_FAILURE);
  }

  // ==========================================================================
  ALint sourceState;
  alGetSourcei(source, AL_SOURCE_STATE, &sourceState);
  while (sourceState == AL_PLAYING) {
    // ... wait until the source has stopped playing ...
    alGetSourcei(source, AL_SOURCE_STATE, &sourceState);
  }

  alDeleteSources(1, &source);
  alDeleteBuffers(1, &buffer);
  alcMakeContextCurrent(nullptr);
  alcDestroyContext(ctx);
  alcCloseDevice(device);
  return EXIT_SUCCESS;
}
