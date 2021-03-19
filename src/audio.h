#pragma once
#include <cstdint>
#include <string_view>
#include <vector>
#include <fstream>

// Read wav file
class AudioFile
{
private:
    size_t frames{};
    int sampleRate{};
    int channels{};
    int format{};
    int sections{};
    int seekable{};

    std::vector<short> samples;
public:
    AudioFile(std::string_view filePath);

    inline size_t getFramesCount() const
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

    inline size_t getSize() const
    {
        return samples.size() * sizeof(decltype(samples)::value_type);
    }

    inline const short* getData() const
    {
        return samples.data();
    }
};