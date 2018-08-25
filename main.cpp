// ============================================================================
//
// Some useful resources:
// http://openal.org/documentation/OpenAL_Programmers_Guide.pdf
//
// A note about OpenAL naming conventions:
//    AL  --- Core-specific functions.
//    ALC --- Device-specific functions.
//
// A basic workflow for playing a sound in OpenAL Soft:
//   1. Open an audio device.
//   2. Create an audio context.
//   3. Activate the audio context.
//   4. Create audio source(s).
//   5. Select to either load or stream the audio data.
//       load.1. Load the audio data from an external source.
//       load.2. Assign the data into a audio buffer.
//       load.3. Assign the audio buffer to a audio source.
//       load.4. Play the audio source.
//     stream.1. Load the first block of data from external source.
//     stream.2. Assign the first block into an audio buffer.
//     stream.3. Queue the first audio buffer to audio source.
//     stream.4. Play the audio source.
//     stream.5. While playing, load the next data from external source.
//     stream.6. Assign the block into an audio buffer.
//     stream.7. Queue the audio buffer to audio source.
//     stream.8. Continue stream.5. to stream.8. until EOF.
//
// ============================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

#include <AL/al.h>
#include <AL/alc.h>

#include <vorbis/vorbisfile.h>

#define STREAM 1
#define CHUNK_SIZE 4096

#if STREAM == 1
#define NUM_BUFFERS 4
#else
#define NUM_BUFFERS 1
#endif

// ============================================================================

static ALCdevice*     sDevice  = nullptr;
static ALCcontext*    sContext = nullptr;
static ALuint         sSource  = 0;
static OggVorbis_File sFile    = {};
static ALuint         sBuffers[NUM_BUFFERS];

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
// Detect the AL output format from the loaded OGG Vorbis file.
// ============================================================================
static ALenum detectFormat(const OggVorbis_File& file)
{
  switch (file.vi->channels)
  {
    case 1:
      return AL_FORMAT_MONO16;
    case 2:
      return AL_FORMAT_STEREO16;
    default:
      printf("Failed to detect format: %d channels!", sFile.vi->channels);
      exit(EXIT_FAILURE);
  }
}

// ============================================================================
// A helper function to play the given OGG Vorbis as a simple sound file.
// ============================================================================
static void playFile()
{
  // ==========================================================================
  // GET MEMORY DATA
  // Get the memory data from the sound file into memory.
  // ==========================================================================
  std::vector<char> oggBuffer;
  char ovBuffer[4096];
  auto eof = 0;
  auto currentSection = 0;
  while (!eof) {
    auto ret = ov_read(&sFile, ovBuffer, sizeof(ovBuffer),0, 2, 1, &currentSection);
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
  auto format = detectFormat(sFile);
  auto frequency = sFile.vi->rate;
  auto dataSize = oggBuffer.size();
  ov_clear(&sFile);
  alBufferData(sBuffers[0], format, oggBuffer.data(), dataSize, frequency);
  if (hasAlError()) {
    printf("alBufferData failed: Unable to set buffer data.\n");
    exit(EXIT_FAILURE);
  }

  // ==========================================================================
  // ASSIGN BUFFER TO SOURCE
  // Assign a buffer containing the sound data to a source.
  // ==========================================================================
  alSourcei(sSource, AL_BUFFER, sBuffers[0]);
  if (hasAlError()) {
    printf("alBufferalGenSourcesData failed: Unable to create a source.\n");
    exit(EXIT_FAILURE);
  }

  // ==========================================================================
  // PLAY THE SOUND
  // Here we actually start to play the sound.
  // ==========================================================================
  alSourcePlay(sSource);
  printf("Playing sound test.ogg\n");
  if (hasAlError()) {
    printf("alSourcePlay failed: Unable to play the specified source.\n");
    exit(EXIT_FAILURE);
  }

  // ==========================================================================
  ALint sourceState;
  alGetSourcei(sSource, AL_SOURCE_STATE, &sourceState);
  while (sourceState == AL_PLAYING) {
    // ... wait until the source has stopped playing ...
    alGetSourcei(sSource, AL_SOURCE_STATE, &sourceState);
  }
}

// ============================================================================
// A helper function to play the given OGG Vorbis as a continouos stream.
//
// OpenAL Soft uses a buffer queueing technique to perform sound streaming. It
// allows buffers and sources to be generated in a normal way, but source will
// not be assigned with the alSourcei(...) function but instead with following.
//
//   alSourceQueueBuffers(source, n, buffers).....To add buffers to source
//   alSourceUnqueueBuffers(source, n, buffers)...To remove buffers from source
//
// Program can attach buffer(s) to a source by using the alSourceQueueBuffers
// and then call the alSourcePlay function to start playing the sound. More
// data can be then added while the source is being player. Played buffers can
// be removed (and reused!) from the source queue with alSourceUnqueueBuffers.
//
// Some important things to remember!
// # Do not use alSourcei(...) at all when using buffering (streaming).
// # All buffers attached to a source should have same audio format.
// # All buffers attached to a source should have same amount of data. If not,
//   then audio will get messed up. Read buffers to contain CHUNK_SIZE amount
//   of data, and only leave the last buffer (eof) to contain partial data.
// ============================================================================
static void playStream()
{
  // get information about the loaded sound file.
  auto format = detectFormat(sFile);
  auto frequency = sFile.vi->rate;
  auto wordSize = 2;

  // ==========================================================================
  // GET THE FIRST BLOCKS OF MEMORY DATA
  // Get the first blocks of memory data from the sound file into memory.
  // ==========================================================================
  auto index = 0;
  auto section = 0;
  auto buffersReady = false;
  for (index = 0; index < NUM_BUFFERS && !buffersReady; index++) {
    std::vector<char> data;
    data.reserve(CHUNK_SIZE);
    while (data.size() < CHUNK_SIZE && !buffersReady) {
      std::vector<char> buffer(CHUNK_SIZE);
      auto readCount = ov_read(&sFile, buffer.data(), (CHUNK_SIZE - data.size()), 0, wordSize, 1, &section);
      if (readCount <= 0) {
        buffersReady = true;
      } else {
        data.insert(data.end(), buffer.begin(), buffer.begin() + readCount);
      }
    }

    // assign the collected data from the buffer to audio buffer.
    alBufferData(sBuffers[index], format, data.data(), data.size(), frequency);
    if (hasAlError()) {
      printf("alBufferData failed: Failed to put data into audio buffer.\n");
      exit(EXIT_FAILURE);
    }
  }

  // ==========================================================================
  // QUEUE FIRST BUFFERS
  // Queue the first set of buffers.
  // ==========================================================================
  alSourceQueueBuffers(sSource, index, &sBuffers[0]);
  if (hasAlError()) {
    printf("alSourceQueueBuffers failed: Failed tou queue source buffers.\n");
    exit(EXIT_FAILURE);
  }

  // ==========================================================================
  // PLAY THE SOUND
  // Start playing the sound.
  // ==========================================================================
  alSourcePlay(sSource);
  if (hasAlError()) {
    printf("alSourcePlay failed: Failed to source playback.\n");
    exit(EXIT_FAILURE);
  }

  // ==========================================================================
  // LOOP (STREAM) UNTIL WE HAVE REACHED THE END OF THE FILE.
  // Loop the source file until we have reached the end of the file.
  // ==========================================================================
  auto eof = 0;
  auto state = (ALint) 0;
  auto freeQueues = (ALint) 0;
  while(!eof) {
    // get the current state of the source.
    alGetSourcei(sSource, AL_SOURCE_STATE, &state);
    if (hasAlError()) {
      printf("alGetSourcei AL_SOURCE_STATE failed.\n");
      exit(EXIT_FAILURE);
    }

    // get the currently free queues from the source.
    alGetSourcei(sSource, AL_BUFFERS_PROCESSED, &freeQueues);
    if (hasAlError()) {
      printf("alGetSourcei AL_BUFFERS_PROCESSED failed.\n");
      exit(EXIT_FAILURE);
    }

    while (freeQueues > 0) {
      ALuint freeBuffer;
      alSourceUnqueueBuffers(sSource, 1, &freeBuffer);
      if (hasAlError()) {
        printf("alSourceUnqueueBuffers failed: unable to unqueue buffer.\n");
        exit(EXIT_FAILURE);
      }

      freeQueues--;

      std::vector<char> data;
      data.reserve(CHUNK_SIZE);
      while (data.size() < CHUNK_SIZE && !buffersReady) {
        std::vector<char> buffer(CHUNK_SIZE);
        auto readCount = ov_read(&sFile, buffer.data(), (CHUNK_SIZE - data.size()), 0, wordSize, 1, &section);
        if (readCount <= 0) {
          buffersReady = true;
          while (CHUNK_SIZE > data.size()) {
            data.push_back(0x00);
          }
        } else {
          data.insert(data.end(), buffer.begin(), buffer.begin() + readCount);
        }
      }

      if (data.size() > 0) {
        // assign the collected data from the buffer to audio buffer.
        alBufferData(freeBuffer, format, data.data(), data.size(), frequency);
        if (hasAlError()) {
          printf("alBufferData failed: Failed to put data into audio buffer.\n");
          exit(EXIT_FAILURE);
        }

        alSourceQueueBuffers(sSource, 1, &freeBuffer);
        if (hasAlError()) {
          printf("alSourceQueueBuffers failed: Failed to queue buffer.\n");
          exit(EXIT_FAILURE);
        }
      }

    }

    if (state != AL_PLAYING && state != AL_PAUSED) {
      ALint queued;
      alGetSourcei(sSource, AL_BUFFERS_QUEUED, &queued);
      if (queued == 0) {
        printf("All buffers used!\n");
        eof = 1;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  printf("exited the while loop!\n");
}

// ============================================================================
static void cleanup()
{
  if (sDevice != nullptr) {
    if (sContext != nullptr) {
      ov_clear(&sFile);
      alDeleteSources(1, &sSource);
      alDeleteBuffers(NUM_BUFFERS, &sBuffers[0]);
      alcMakeContextCurrent(nullptr);
      alcDestroyContext(sContext);
    }
    alcCloseDevice(sDevice);
  }
}

// ============================================================================

int main()
{
  atexit(cleanup);

  // ==========================================================================
  // OPEN A DEVICE
  // The first thing to do in OpenAL is to open a device (nullptr = default).
  // ==========================================================================
  sDevice = alcOpenDevice(nullptr);
  if (sDevice == nullptr || hasAlcError(sDevice)) {
    printf("alcOpenDevice failed: Unable to open device.\n");
    exit(EXIT_FAILURE);
  }

  // ==========================================================================
  // CREATE A CONTEXT
  // The second thing is to create a context for the sound device.
  // ==========================================================================
  sContext = alcCreateContext(sDevice, nullptr);
  if (sContext == nullptr || hasAlcError(sDevice)) {
    printf("alcCreateContext failed: Unable to create device context.\n");
    exit(EXIT_FAILURE);
  }

  // ==========================================================================
  // ACTIVATE CONTEXT
  // Define the created context as the active context.
  // ==========================================================================
  if (alcMakeContextCurrent(sContext) == ALC_FALSE || hasAlcError(sDevice)) {
    printf("alcMakeContextCurrent failed: Unable to set active context.\n");
    exit(EXIT_FAILURE);
  }

  // ==========================================================================
  // CREATE SOURCE(S)
  // Create source(s) which are used to control the playback.
  // ==========================================================================
  alGenSources(1, &sSource);
  if (hasAlError()) {
    printf("alBufferalGenSourcesData failed: Unable to create a source.\n");
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
  auto result = ov_fopen("test.ogg", &sFile);
  if (result != 0) {
    printf("ov_fopen failed: Failed to open test.ogg file.\n");
    exit(EXIT_FAILURE);
  }

  // ==========================================================================
  // CREATE BUFFER(S)
  // Create AL buffer(s) to store audio data for the playback.
  // ==========================================================================
  alGenBuffers(NUM_BUFFERS, &sBuffers[0]);
  if (hasAlError()) {
    printf("alGenBuffers failed: Unable to set active context.\n");
    exit(EXIT_FAILURE);
  }

  #if STREAM == 1
    playStream();
  #else
    playFile();
  #endif

  return EXIT_SUCCESS;
}
