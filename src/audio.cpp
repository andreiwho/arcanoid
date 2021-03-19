#include "audio.h"
#include <stdexcept>
#include <iostream>

#include <sndfile.h>

AudioFile::AudioFile(std::string_view filePath)
{
    SF_INFO info = {};
    SNDFILE* file = sf_open(filePath.data(), SFM_READ, &info);

    frames = info.frames;
    sampleRate = info.samplerate;
    channels = info.channels;
    format = info.format;
    sections = info.sections;
    seekable = info.seekable;

    sf_close(file);
}

