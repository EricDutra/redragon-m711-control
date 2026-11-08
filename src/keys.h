#pragma once

#include <cstdint>
#include <span>

namespace keys {

struct KeyInfo {
    uint8_t hid;
    int vk;
    const char* name;
    const char* namePt = nullptr;
};

struct MediaInfo {
    uint16_t usage;
    const char* name;
    const char* namePt = nullptr;
};

std::span<const KeyInfo> keyboardKeys();
std::span<const MediaInfo> mediaKeys();

const char* label(const KeyInfo& key);
const char* label(const MediaInfo& media);

uint8_t hidFromVk(int vk, bool extended);
const char* nameFromHid(uint8_t hid);
const char* nameFromMedia(uint16_t usage);
const char* mouseButtonName(uint16_t mask);

}
