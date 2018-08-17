// ============================================================================
// A trivial implementation to read a simple wave file from the filesystem.
//
// NOTE: THIS IMPLEMENTATION DOES NOT CONTAIN ANY KIND OF ERROR CHECKS! DO NOT
//       USE THIS IN PRODUCTION ENVIRONMENT. ALWAYS USE WITH EXTREME CAUTION!
//
// Some useful resources:
// http://soundfile.sapp.org/doc/WaveFormat/
//
// ============================================================================
#ifndef WAVE_H
#define WAVE_H

#include <cstdint>
#include <string>
#include <vector>

struct RiffHeader
{
  uint8_t  chunkId[4];
  uint32_t chunkSize;
  uint8_t  format[4];
};

struct FmtSubchunk
{
  uint8_t  subchunk1Id[4];
  uint32_t subchunk1Size;
  uint16_t audioFormat;
  uint16_t numChannels;
  uint32_t sampleRate;
  uint32_t byteRate;
  uint16_t blockAlign;
  uint16_t bitsPerSample;
};

struct DataSubchunk
{
  uint8_t              subchunk2Id[4];
  uint32_t             subchunk2Size;
  std::vector<uint8_t> data;
};

struct WaveFile
{
  RiffHeader   riff;
  FmtSubchunk  fmt;
  DataSubchunk data;
};

WaveFile wave_load(const std::string& filename);

#endif
