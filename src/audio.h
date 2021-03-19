#pragma once
#include <cstdint>
#include <string_view>
#include <vector>
#include <fstream>
#include <AL/al.h>

// Read wav file
std::vector<char> wavLoadFile(std::string_view fileName, ALubyte* numChannels, ALint* sampleRate, ALubyte* bitsPerSample, ALsizei* size);

class AudioFile
{
private:
    size_t frames{};
    int sampleRate{};
    int channels{};
    int format{};
    int sections{};
    int seekable{};
public:
    AudioFile(std::string_view filePath);

    inline size_t getFramesCound() const
    {
        return frames;
    }

    inline int getSampleRate() const
    {
        return sampleRate;
    }

    inline int getChannels() const
    {
        return channels;
    }

    inline int getFormat() const
    {
        return format;
    }

    inline int getSections() const
    {
        return sections;
    }

    inline int isSeekable() const
    {
        return seekable;
    }
};