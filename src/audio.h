#pragma once
#include <cstdint>
#include <string_view>
#include <vector>
#include <fstream>
#include <AL/al.h>

// Read wav file
std::vector<char> wavLoadFile(std::string_view fileName, ALubyte* numChannels, ALint* sampleRate, ALubyte* bitsPerSample, ALsizei* size);
