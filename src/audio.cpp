#include "audio.h"
#include <stdexcept>
#include <iostream>

int convertToInt(char* buffer, size_t size)
{
    int a = 0;
    if (std::endian::native == std::endian::little)
    {
        std::memcpy(&a, buffer, size);
    }
    else
    {
        for (size_t i = 0; i < size; ++i)
        {
            reinterpret_cast<char*>(&a)[3 - i] = buffer[i];
        }
    }
    return a;
}

bool wavLoadHeader(std::ifstream& file, ALubyte* numChannels, ALint* sampleRate, ALubyte* bitsPerSample, ALsizei* size)
{
    char buffer[4];
    if (!file.read(buffer, 4))
    {
        throw std::runtime_error("Failed to read RIFF");
    }

    // RIFF
    if (std::strncmp(buffer, "RIFF", 4) != 0)
    {
        throw std::runtime_error("File is not a valid WAV file. Header doesn't contain RIFF");
    }

    // Read the size of the file
    if (!file.read(buffer, 4))
    {
        throw std::runtime_error("Failed to read size of file");
    }

    // WAVE
    if (!file.read(buffer, 4))
    {
        throw std::runtime_error("Failed to read WAVE");
    }

    if (std::strncmp(buffer, "WAVE", 4) != 0)
    {
        throw std::runtime_error("File is not a valid WAVE file (it does not contain WAVE)");
    }

    // fmt/0
    if (!file.read(buffer, 4))
    {
        throw std::runtime_error("Failed to read fmt/0");
    }

    // always 16, the size of the fmt data chunk
    if (!file.read(buffer, 4))
    {
        throw std::runtime_error("Could not read 16");
    }

    // PCM should be 1?
    if (!file.read(buffer, 2))
    {
        throw std::runtime_error("Could  not read PCM");
    }

    // Number of channels
    if (!file.read(buffer, 2))
    {
        throw std::runtime_error("Failed to read number of channels");
    }
    *numChannels = convertToInt(buffer, 2);

    // Sample rate
    if (!file.read(buffer, 4))
    {
        throw std::runtime_error("Failed to read file sample rate");
    }
    *sampleRate = convertToInt(buffer, 4);

    // (sampleRate * bitsPerSample * channels) / 8
    if (!file.read(buffer, 4))
    {
        throw std::runtime_error("Failed to read (sampleRate * bitsPerSample * channels) / 8");
    }

    // ?? dafaq
    if (!file.read(buffer, 2))
    {
        throw std::runtime_error("Failed to read dafaq");
    }

    // Bits per sample
    if (!file.read(buffer, 2))
    {
        throw std::runtime_error("Failed to read bits per sample");
    }
    *bitsPerSample = convertToInt(buffer, 2);

    // Data chunk header "data"
    if (!file.read(buffer, 4))
    {
        throw std::runtime_error("Failed to read chank header 'data'");
    }

    if (std::strncmp(buffer, "data", 4) != 0)
    {
        std::cout << buffer << std::endl;
        throw std::runtime_error("The file is not a valid WAV file. It doesn't contain 'data' tag");
    }

    // Size of data
    if (!file.read(buffer, 4))
    {
        throw std::runtime_error("Failed to read data size");
    }
    *size = convertToInt(buffer, 4);

    // Cannot be at the end of file
    if (file.eof())
    {
        throw std::runtime_error("Reached EOF");
    }

    if (file.fail())
    {
        throw std::runtime_error("Failed state set on file");
    }

    return true;
}

std::vector<char> wavLoadFile(std::string_view fileName, ALubyte* numChannels, ALint* sampleRate, ALubyte* bitsPerSample, ALsizei* size)
{
    std::ifstream in(fileName.data(), std::ios::binary);
    if (!in.is_open())
    {
        throw std::runtime_error("Failed to read WAV file");
    }

    if (!wavLoadHeader(in, numChannels, sampleRate, bitsPerSample, size))
    {
        throw std::runtime_error("Failed to read WAV header");
    }

    std::vector<char> data;
    data.reserve(*size);
    in.read(data.data(), *size);

    return data;
}
