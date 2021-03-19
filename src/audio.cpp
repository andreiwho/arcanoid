#include "audio.h"
#include <stdexcept>
#include <iostream>

#include <sndfile.h>
#include <array>

AudioFile::AudioFile(std::string_view filePath)
{
    SF_INFO info = {};
    SNDFILE* file = sf_open(filePath.data(), SFM_READ, &info);

    // Read data
    std::array<short, 4096> data;
    size_t count = 0;
    while ((count = sf_read_short(file, data.data(), data.size())) != 0)
    {
        samples.insert(samples.end(), data.begin(), data.begin() + count);
    }

    frames = info.frames;
    sampleRate = info.samplerate;
    channels = info.channels;
    format = info.format;
    sections = info.sections;
    seekable = info.seekable;

    sf_close(file);
}

