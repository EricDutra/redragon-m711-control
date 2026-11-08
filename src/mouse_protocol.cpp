#include "mouse_protocol.h"

#include <windows.h>

#include <algorithm>

namespace rdc {

namespace {

constexpr uint8_t kCmdWrite = 0xF3;
constexpr uint8_t kCmdRead = 0xF2;
constexpr uint8_t kCmdApply = 0xF1;

constexpr uint8_t kReportShort = 0x02;
constexpr uint8_t kReportLong = 0x03;
constexpr uint8_t kReportMacro = 0x04;
constexpr size_t kShortLength = 16;
constexpr size_t kLongLength = 64;
constexpr size_t kMacroLength = 1024;
constexpr size_t kHeaderLength = 8;
constexpr size_t kShortPayload = kShortLength - kHeaderLength;
constexpr size_t kLongPayload = kLongLength - kHeaderLength;

constexpr uint16_t kAddrActiveProfile = 0x002C;
constexpr uint16_t kAddrPollRate = 0x0032;
constexpr uint16_t kAddrLedPrefix = 0x0446;
constexpr uint16_t kAddrLed = 0x0449;
constexpr uint16_t kAddrMacro = 0x0478;
constexpr std::array<uint16_t, kProfileCount> kProfileBase = {0x0040, 0x0100, 0x01B0, 0x0260, 0x0310};

constexpr uint8_t kApplyProfile = 0x01;
constexpr uint8_t kApplyDpi = 0x02;
constexpr uint8_t kApplyLed = 0x04;
constexpr uint8_t kApplyPoll = 0x08;
constexpr uint8_t kApplyDpiActive = 0x10;

constexpr std::array<uint8_t, 114> kDpiCode = {
    0x00, 0x04, 0x06, 0x09, 0x0B, 0x0D, 0x0F, 0x12, 0x14, 0x16, 0x18, 0x1B, 0x1D, 0x1F, 0x21, 0x24, 0x26, 0x28, 0x2B,
    0x2D, 0x2F, 0x31, 0x34, 0x36, 0x38, 0x3A, 0x3D, 0x3F, 0x41, 0x43, 0x46, 0x48, 0x4A, 0x4D, 0x4F, 0x51, 0x53, 0x56,
    0x58, 0x5A, 0x5C, 0x5F, 0x61, 0x63, 0x66, 0x68, 0x6A, 0x6C, 0x6F, 0x71, 0x73, 0x75, 0x78, 0x7A, 0x7C, 0x7F, 0x81,
    0x83, 0x85, 0x87, 0x8A, 0x8C, 0x48, 0x4A, 0x4D, 0x4F, 0x51, 0x53, 0x56, 0x58, 0x5A, 0x5C, 0x5F, 0x61, 0x63, 0x66,
    0x68, 0x6A, 0x6C, 0x6F, 0x71, 0x73, 0x75, 0x78, 0x7A, 0x7C, 0x7F, 0x81, 0x83, 0x85, 0x87, 0x8A, 0x8C, 0x5F, 0x61,
    0x63, 0x66, 0x68, 0x6A, 0x6C, 0x6F, 0x71, 0x73, 0x75, 0x78, 0x7A, 0x7C, 0x7F, 0x81, 0x83, 0x85, 0x87, 0x8A, 0x8C,
};

uint8_t dpiFlag(int index) {
    if (index < 0x3E) {
        return 0;
    }
    if (index < 0x5D) {
        return 1;
    }
    return 4;
}

int dpiIndexFromCode(uint8_t code, uint8_t flag) {
    for (int i = kDpiMinIndex; i <= kDpiMaxIndex; ++i) {
        if (kDpiCode[i] == code && dpiFlag(i) == flag) {
            return i;
        }
    }
    return 0;
}

struct LedEncoding {
    LedEffect effect;
    uint8_t mode;
    uint8_t variant;
};

constexpr std::array<LedEncoding, 8> kLedEncodings = {{
    {LedEffect::Breathing, 0x01, 0x04},
    {LedEffect::Rainbow, 0x01, 0x08},
    {LedEffect::Static, 0x01, 0x02},
    {LedEffect::Wave, 0x02, 0x00},
    {LedEffect::GoWithoutTrace, 0x06, 0x00},
    {LedEffect::Reactive, 0x07, 0x00},
    {LedEffect::Flash, 0x01, 0x10},
    {LedEffect::Off, 0x00, 0x00},
}};

void pause(DWORD ms) {
    Sleep(ms);
}

}

int dpiFromIndex(int index) {
    return (index + 1) * 100;
}

int dpiIndexFromDpi(int dpi) {
    return std::clamp(dpi / 100 - 1, kDpiMinIndex, kDpiMaxIndex);
}

int pollHzFromRaw(uint8_t raw) {
    switch (raw) {
    case 1: return 1000;
    case 2: return 500;
    case 4: return 250;
    case 8: return 125;
    default: return 0;
    }
}

uint8_t pollRawFromHz(int hz) {
    switch (hz) {
    case 1000: return 1;
    case 500: return 2;
    case 250: return 4;
    default: return 8;
    }
}

const char* ledEffectName(LedEffect effect) {
    switch (effect) {
    case LedEffect::Breathing: return "Respiração";
    case LedEffect::Rainbow: return "Arco-íris";
    case LedEffect::Static: return "Fixo";
    case LedEffect::Wave: return "Onda";
    case LedEffect::GoWithoutTrace: return "Rastro";
    case LedEffect::Reactive: return "Reativo";
    case LedEffect::Flash: return "Piscar";
    case LedEffect::Off: return "Desligado";
    default: return "Desconhecido";
    }
}

bool ledEffectHasSpeed(LedEffect effect) {
    return effect != LedEffect::Static && effect != LedEffect::Off;
}

bool Mouse::connect() {
    disconnect();
    auto path = HidDevice::findPath(kVendorId, kProductId, kConfigUsagePage);
    if (!path) {
        return fail("Mouse não encontrado (04D9:FC30, interface 0xFFA0).");
    }
    if (!device_.open(*path)) {
        return fail("Não foi possível abrir a interface de configuração do mouse.");
    }
    lastError_.clear();
    return true;
}

void Mouse::disconnect() {
    device_.close();
}

bool Mouse::fail(const std::string& message) {
    lastError_ = message;
    return false;
}

bool Mouse::transfer(uint8_t reportId, size_t reportLength, const std::vector<uint8_t>& request,
                     std::vector<uint8_t>* response) {
    std::vector<uint8_t> buffer(reportLength, 0);
    std::copy(request.begin(), request.end(), buffer.begin());
    buffer[0] = reportId;
    if (!device_.setFeature(buffer.data(), buffer.size())) {
        return fail("Falha ao enviar feature report.");
    }
    pause(10);
    if (!response) {
        return true;
    }
    std::fill(buffer.begin(), buffer.end(), 0);
    buffer[0] = reportId;
    buffer[1] = kCmdRead;
    if (!device_.getFeature(buffer.data(), buffer.size())) {
        return fail("Falha ao ler feature report.");
    }
    *response = std::move(buffer);
    return true;
}

bool Mouse::readMemory(uint16_t address, uint8_t length, std::vector<uint8_t>& out) {
    out.clear();
    size_t remaining = length;
    uint16_t cursor = address;
    while (remaining > 0) {
        auto chunk = static_cast<uint8_t>(std::min(remaining, kLongPayload));
        std::vector<uint8_t> request = {kReportLong, kCmdRead, static_cast<uint8_t>(cursor & 0xFF),
                                        static_cast<uint8_t>(cursor >> 8), chunk};
        std::vector<uint8_t> response;
        if (!transfer(kReportLong, kLongLength, request, &response)) {
            return false;
        }
        out.insert(out.end(), response.begin() + kHeaderLength, response.begin() + kHeaderLength + chunk);
        cursor = static_cast<uint16_t>(cursor + chunk);
        remaining -= chunk;
    }
    return true;
}

bool Mouse::writeMemory(uint16_t address, const std::vector<uint8_t>& data) {
    size_t offset = 0;
    while (offset < data.size()) {
        size_t remaining = data.size() - offset;
        bool shortReport = remaining <= kShortPayload;
        size_t chunk = shortReport ? remaining : std::min(remaining, kLongPayload);
        auto cursor = static_cast<uint16_t>(address + offset);
        std::vector<uint8_t> request = {0, kCmdWrite, static_cast<uint8_t>(cursor & 0xFF),
                                        static_cast<uint8_t>(cursor >> 8), static_cast<uint8_t>(chunk), 0, 0, 0};
        request.insert(request.end(), data.begin() + offset, data.begin() + offset + chunk);
        bool ok = shortReport ? transfer(kReportShort, kShortLength, request, nullptr)
                              : transfer(kReportLong, kLongLength, request, nullptr);
        if (!ok) {
            return false;
        }
        offset += chunk;
    }
    return true;
}

bool Mouse::apply(uint8_t flag) {
    std::vector<uint8_t> request = {0, kCmdApply, 0x02, flag};
    return transfer(kReportShort, kShortLength, request, nullptr);
}

bool Mouse::applyAll() {
    for (uint8_t flag : {kApplyLed, kApplyProfile, kApplyDpi, kApplyPoll, kApplyDpiActive}) {
        if (!apply(flag)) {
            return false;
        }
    }
    return true;
}

bool Mouse::readActiveProfile(uint8_t& profile) {
    std::vector<uint8_t> data;
    if (!readMemory(kAddrActiveProfile, 1, data)) {
        return false;
    }
    profile = data[0];
    return true;
}

bool Mouse::writeActiveProfile(uint8_t profile) {
    if (profile >= kProfileCount) {
        return fail("Perfil inválido.");
    }
    return writeMemory(kAddrActiveProfile, {profile, 0}) && apply(kApplyProfile);
}

bool Mouse::readLed(int profile, LedSettings& led) {
    std::vector<uint8_t> data;
    if (!readMemory(static_cast<uint16_t>(kAddrLed + profile * 8), 7, data)) {
        return false;
    }
    std::copy(data.begin(), data.end(), led.raw.begin());
    led.red = data[0];
    led.green = data[1];
    led.blue = data[2];
    led.effect = LedEffect::Unknown;
    for (const auto& encoding : kLedEncodings) {
        if (encoding.mode == data[3] && encoding.variant == data[5]) {
            led.effect = encoding.effect;
            break;
        }
    }
    led.speed = data[4] <= 9 ? static_cast<uint8_t>(9 - data[4]) : 4;
    led.brightness = data[6];
    return true;
}

bool Mouse::writeLed(int profile, const LedSettings& led) {
    auto encoding = std::find_if(kLedEncodings.begin(), kLedEncodings.end(),
                                 [&](const LedEncoding& e) { return e.effect == led.effect; });
    if (encoding == kLedEncodings.end()) {
        return fail("Efeito de LED inválido.");
    }
    uint8_t speedByte = ledEffectHasSpeed(led.effect) ? static_cast<uint8_t>(9 - std::clamp<int>(led.speed, 1, 8)) : 0;
    auto brightness = static_cast<uint8_t>(std::clamp<int>(led.brightness, 1, 3));
    std::vector<uint8_t> data = {led.red, led.green, led.blue, encoding->mode, speedByte, encoding->variant, brightness};
    return writeMemory(kAddrLedPrefix, {0, 0}) &&
           writeMemory(static_cast<uint16_t>(kAddrLed + profile * 8), data) && apply(kApplyLed);
}

bool Mouse::readPollRates(std::array<uint8_t, kProfileCount>& raw) {
    std::vector<uint8_t> data;
    if (!readMemory(kAddrPollRate, kProfileCount * 2, data)) {
        return false;
    }
    for (int i = 0; i < kProfileCount; ++i) {
        raw[i] = data[i * 2];
    }
    return true;
}

bool Mouse::writePollRate(int profile, uint8_t raw) {
    std::vector<uint8_t> data;
    if (!readMemory(kAddrPollRate, kProfileCount * 2, data)) {
        return false;
    }
    data[profile * 2] = raw;
    std::vector<uint8_t> first(data.begin(), data.begin() + 6);
    std::vector<uint8_t> second(data.begin() + 6, data.end());
    return writeMemory(kAddrPollRate, first) && writeMemory(kAddrPollRate + 6, second) && apply(kApplyPoll);
}

bool Mouse::readDpi(int profile, uint8_t& activeStage, std::array<DpiStage, kDpiStageCount>& stages) {
    std::vector<uint8_t> data;
    uint16_t base = kProfileBase[profile];
    if (!readMemory(static_cast<uint16_t>(base + 2), 2 + kDpiStageCount * 6, data)) {
        return false;
    }
    activeStage = data[0];
    for (int i = 0; i < kDpiStageCount; ++i) {
        const uint8_t* entry = data.data() + 2 + i * 6;
        DpiStage& stage = stages[i];
        std::copy(entry, entry + 5, stage.raw.begin());
        stage.enabled = entry[0] != 0;
        stage.xIndex = dpiIndexFromCode(entry[1], entry[2]);
        stage.yIndex = dpiIndexFromCode(entry[3], entry[4]);
    }
    return true;
}

bool Mouse::writeDpi(int profile, uint8_t activeStage, const std::array<DpiStage, kDpiStageCount>& stages) {
    uint16_t base = kProfileBase[profile];
    for (int i = 0; i < kDpiStageCount; ++i) {
        const DpiStage& stage = stages[i];
        int x = std::clamp(stage.xIndex, kDpiMinIndex, kDpiMaxIndex);
        int y = std::clamp(stage.yIndex, kDpiMinIndex, kDpiMaxIndex);
        std::vector<uint8_t> entry = {static_cast<uint8_t>(stage.enabled ? 1 : 0), kDpiCode[x], dpiFlag(x), kDpiCode[y],
                                      dpiFlag(y)};
        if (!writeMemory(static_cast<uint16_t>(base + 4 + i * 6), entry)) {
            return false;
        }
    }
    if (!writeMemory(static_cast<uint16_t>(base + 2), {activeStage})) {
        return false;
    }
    return apply(kApplyDpi) && apply(kApplyDpiActive);
}

bool Mouse::readButtons(int profile, std::array<ButtonAction, kButtonCount>& buttons) {
    std::vector<uint8_t> data;
    if (!readMemory(static_cast<uint16_t>(kProfileBase[profile] + 0x42), kButtonCount * 4, data)) {
        return false;
    }
    for (int i = 0; i < kButtonCount; ++i) {
        buttons[i] = {data[i * 4], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3]};
    }
    return true;
}

bool Mouse::writeButton(int profile, int button, const ButtonAction& action) {
    if (button < 0 || button >= kButtonCount) {
        return fail("Botão inválido.");
    }
    std::vector<uint8_t> data = {action.code, action.arg1, action.arg2, action.arg3};
    return writeMemory(static_cast<uint16_t>(kProfileBase[profile] + 0x42 + button * 4), data) && applyAll();
}

bool Mouse::writeButtons(int profile, const std::array<ButtonAction, kButtonCount>& buttons) {
    std::vector<uint8_t> data;
    for (const auto& action : buttons) {
        data.insert(data.end(), {action.code, action.arg1, action.arg2, action.arg3});
    }
    return writeMemory(static_cast<uint16_t>(kProfileBase[profile] + 0x42), data) && applyAll();
}

bool Mouse::readMacro(int index, std::vector<MacroEvent>& events) {
    events.clear();
    std::vector<uint8_t> data;
    if (!readMemory(static_cast<uint16_t>(kAddrMacro + index * kMacroBytes), kMacroBytes, data)) {
        return false;
    }
    for (int i = 0; i + 2 < kMacroBytes; i += 3) {
        uint8_t type = data[i];
        if (type != 0x84 && type != 0x04 && type != 0x81 && type != 0x01 && type != 0x06) {
            break;
        }
        events.push_back({static_cast<MacroEventType>(type), static_cast<uint16_t>(data[i + 1] | (data[i + 2] << 8))});
    }
    return true;
}

bool Mouse::writeMacro(int index, const std::vector<MacroEvent>& events) {
    if (index < 0 || index >= kMacroCount) {
        return fail("Índice de macro inválido.");
    }
    if (static_cast<int>(events.size()) > kMacroMaxEvents) {
        return fail("Macro grande demais (máximo de 66 eventos).");
    }
    auto address = static_cast<uint16_t>(kAddrMacro + index * kMacroBytes);
    std::vector<uint8_t> request = {0, kCmdWrite, static_cast<uint8_t>(address & 0xFF), static_cast<uint8_t>(address >> 8),
                                    kMacroBytes, 0, 0, 0};
    for (const auto& event : events) {
        request.push_back(static_cast<uint8_t>(event.type));
        request.push_back(static_cast<uint8_t>(event.value & 0xFF));
        request.push_back(static_cast<uint8_t>(event.value >> 8));
    }
    request.resize(kHeaderLength + kMacroBytes, 0);
    if (!transfer(kReportMacro, kMacroLength, request, nullptr)) {
        return false;
    }
    pause(50);
    return true;
}

bool Mouse::readState(MouseState& state) {
    if (!readActiveProfile(state.activeProfile)) {
        return false;
    }
    std::array<uint8_t, kProfileCount> polls{};
    if (!readPollRates(polls)) {
        return false;
    }
    for (int p = 0; p < kProfileCount; ++p) {
        Profile& profile = state.profiles[p];
        profile.pollRaw = polls[p];
        if (!readLed(p, profile.led) || !readDpi(p, profile.activeDpiStage, profile.dpi) ||
            !readButtons(p, profile.buttons)) {
            return false;
        }
    }
    return true;
}

}
