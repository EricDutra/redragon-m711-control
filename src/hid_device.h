#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

class HidDevice {
public:
    HidDevice() = default;
    ~HidDevice();
    HidDevice(const HidDevice&) = delete;
    HidDevice& operator=(const HidDevice&) = delete;

    static std::optional<std::wstring> findPath(uint16_t vendorId, uint16_t productId, uint16_t usagePage);

    bool open(const std::wstring& path);
    void close();
    bool isOpen() const;

    bool setFeature(const uint8_t* data, size_t length);
    bool getFeature(uint8_t* data, size_t length);

    uint16_t releaseNumber() const { return releaseNumber_; }

private:
    void* handle_ = nullptr;
    uint16_t releaseNumber_ = 0;
    size_t featureLength_ = 0;
};
