#include "wave.h"

#include <cstdio>
#include <fstream>

WaveFile wave_load(const std::string& filename)
{
  std::ifstream file(filename, std::ifstream::in | std::ifstream::binary);
  if (!file) {
    printf("Failed to find wave file: %s\n", filename.c_str());
    return {};
  } else {
    // a wave file contains three sections.
    RiffHeader riff = {};
    FmtSubchunk fmt = {};
    DataSubchunk data = {};

    // read the static header data from the provided file.
    file.read((char*)&riff, sizeof(riff));
    file.read((char*)&fmt, sizeof(fmt));

    // ... and now we read the dynamic data (i.e. the actual data).
    file.read((char*)&data.subchunk2Id, sizeof(data.subchunk2Id));
    file.read((char*)&data.subchunk2Size, sizeof(data.subchunk2Size));
    data.data.resize(data.subchunk2Size);
    file.read((char*)data.data.data(), data.subchunk2Size);

    // construct the parsed wave file content wrapper.
    return { riff, fmt, data };
  }
}
