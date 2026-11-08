#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "hid_device.h"

namespace rdc {

constexpr uint16_t kVendorId = 0x04D9;
constexpr uint16_t kProductId = 0xFC30;
constexpr uint16_t kConfigUsagePage = 0xFFA0;

constexpr int kProfileCount = 5;
constexpr int kDpiStageCount = 5;
constexpr int kButtonCount = 8;
constexpr int kMacroCount = 20;
constexpr int kMacroBytes = 200;
constexpr int kMacroMaxEvents = kMacroBytes / 3;
constexpr int kDpiMinIndex = 1;
constexpr int kDpiMaxIndex = 113;

enum class LedEffect : uint8_t {
    Breathing,
    Rainbow,
    Static,
    Wave,
    GoWithoutTrace,
    Reactive,
    Flash,
    Off,
    Unknown,
};

struct LedSettings {
    uint8_t red = 255;
    uint8_t green = 0;
    uint8_t blue = 0;
    LedEffect effect = LedEffect::Static;
    uint8_t speed = 4;
    uint8_t brightness = 2;
    std::array<uint8_t, 7> raw{};
};

struct DpiStage {
    bool enabled = true;
    int xIndex = 10;
    int yIndex = 10;
    std::array<uint8_t, 5> raw{};
};

struct ButtonAction {
    uint8_t code = 0;
    uint8_t arg1 = 0;
    uint8_t arg2 = 0;
    uint8_t arg3 = 0;
};

enum class MacroEventType : uint8_t {
    KeyDown = 0x84,
    KeyUp = 0x04,
    MouseDown = 0x81,
    MouseUp = 0x01,
    Delay = 0x06,
};

struct MacroEvent {
    MacroEventType type = MacroEventType::Delay;
    uint16_t value = 0;
};

enum class MacroRepeat : uint8_t {
    Count = 0x00,
    WhileHeld = 0x80,
    Toggle = 0x40,
};

struct Profile {
    LedSettings led;
    uint8_t pollRaw = 1;
    uint8_t activeDpiStage = 0;
    std::array<DpiStage, kDpiStageCount> dpi{};
    std::array<ButtonAction, kButtonCount> buttons{};
};

struct MouseState {
    uint8_t activeProfile = 0;
    std::array<Profile, kProfileCount> profiles{};
};

int dpiFromIndex(int index);
int dpiIndexFromDpi(int dpi);
int pollHzFromRaw(uint8_t raw);
uint8_t pollRawFromHz(int hz);
const char* ledEffectName(LedEffect effect);
bool ledEffectHasSpeed(LedEffect effect);

class Mouse {
public:
    bool connect();
    void disconnect();
    bool isConnected() const { return device_.isOpen(); }
    uint16_t firmwareRevision() const { return device_.releaseNumber(); }
    const std::string& lastError() const { return lastError_; }

    bool readMemory(uint16_t address, uint8_t length, std::vector<uint8_t>& out);
    bool writeMemory(uint16_t address, const std::vector<uint8_t>& data);
    bool apply(uint8_t flag);
    bool applyAll();

    bool readState(MouseState& state);

    bool readActiveProfile(uint8_t& profile);
    bool writeActiveProfile(uint8_t profile);

    bool readLed(int profile, LedSettings& led);
    bool writeLed(int profile, const LedSettings& led);

    bool readPollRates(std::array<uint8_t, kProfileCount>& raw);
    bool writePollRate(int profile, uint8_t raw);

    bool readDpi(int profile, uint8_t& activeStage, std::array<DpiStage, kDpiStageCount>& stages);
    bool writeDpi(int profile, uint8_t activeStage, const std::array<DpiStage, kDpiStageCount>& stages);

    bool readButtons(int profile, std::array<ButtonAction, kButtonCount>& buttons);
    bool writeButton(int profile, int button, const ButtonAction& action);
    bool writeButtons(int profile, const std::array<ButtonAction, kButtonCount>& buttons);

    bool readMacro(int index, std::vector<MacroEvent>& events);
    bool writeMacro(int index, const std::vector<MacroEvent>& events);

private:
    bool transfer(uint8_t reportId, size_t reportLength, const std::vector<uint8_t>& request, std::vector<uint8_t>* response);
    bool fail(const std::string& message);

    HidDevice device_;
    std::string lastError_;
};

}
