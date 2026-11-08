#include <windows.h>

#include <cstdio>
#include <string>
#include <vector>

#include "mouse_protocol.h"

namespace {

void printHex(uint16_t address, const std::vector<uint8_t>& data) {
    for (size_t i = 0; i < data.size(); i += 16) {
        std::printf("%04X:", static_cast<unsigned>(address + i));
        for (size_t j = i; j < i + 16 && j < data.size(); ++j) {
            std::printf(" %02X", data[j]);
        }
        std::printf("\n");
    }
}

int dumpState(rdc::Mouse& mouse) {
    rdc::MouseState state;
    if (!mouse.readState(state)) {
        std::printf("Erro: %s\n", mouse.lastError().c_str());
        return 1;
    }
    std::printf("Perfil ativo: %d\n", state.activeProfile + 1);
    for (int p = 0; p < rdc::kProfileCount; ++p) {
        const rdc::Profile& profile = state.profiles[p];
        std::printf("\n== Perfil %d ==\n", p + 1);
        std::printf("Polling raw=%u (%d Hz?)\n", profile.pollRaw, rdc::pollHzFromRaw(profile.pollRaw));
        const auto& led = profile.led;
        std::printf("LED raw:");
        for (uint8_t b : led.raw) {
            std::printf(" %02X", b);
        }
        std::printf("  -> %s cor=#%02X%02X%02X vel=%u brilho=%u\n", rdc::ledEffectName(led.effect), led.red, led.green,
                    led.blue, led.speed, led.brightness);
        std::printf("DPI ativo: estágio %u\n", profile.activeDpiStage + 1);
        for (int i = 0; i < rdc::kDpiStageCount; ++i) {
            const auto& stage = profile.dpi[i];
            std::printf("  estágio %d raw: %02X %02X %02X %02X %02X -> %s X=%d Y=%d\n", i + 1, stage.raw[0], stage.raw[1],
                        stage.raw[2], stage.raw[3], stage.raw[4], stage.enabled ? "on " : "off",
                        rdc::dpiFromIndex(stage.xIndex), rdc::dpiFromIndex(stage.yIndex));
        }
        for (int b = 0; b < rdc::kButtonCount; ++b) {
            const auto& action = profile.buttons[b];
            std::printf("  botão %d: %02X %02X %02X %02X\n", b + 1, action.code, action.arg1, action.arg2, action.arg3);
        }
    }
    return 0;
}

int dumpRaw(rdc::Mouse& mouse, uint16_t address, int length) {
    std::vector<uint8_t> data;
    std::vector<uint8_t> all;
    for (int offset = 0; offset < length; offset += 32) {
        int chunk = length - offset < 32 ? length - offset : 32;
        if (!mouse.readMemory(static_cast<uint16_t>(address + offset), static_cast<uint8_t>(chunk), data)) {
            std::printf("Erro: %s\n", mouse.lastError().c_str());
            return 1;
        }
        all.insert(all.end(), data.begin(), data.end());
    }
    printHex(address, all);
    return 0;
}

}

int main(int argc, char** argv) {
    SetConsoleOutputCP(CP_UTF8);
    rdc::Mouse mouse;
    if (!mouse.connect()) {
        std::printf("Erro: %s\n", mouse.lastError().c_str());
        return 1;
    }
    std::printf("Conectado. Revisão do firmware: 0x%04X\n", mouse.firmwareRevision());

    std::string command = argc > 1 ? argv[1] : "state";
    if (command == "state") {
        return dumpState(mouse);
    }
    if (command == "raw" && argc > 3) {
        auto address = static_cast<uint16_t>(std::stoul(argv[2], nullptr, 16));
        int length = std::stoi(argv[3]);
        return dumpRaw(mouse, address, length);
    }
    if (command == "macro" && argc > 2) {
        int index = std::stoi(argv[2]);
        std::vector<rdc::MacroEvent> events;
        if (!mouse.readMacro(index, events)) {
            std::printf("Erro: %s\n", mouse.lastError().c_str());
            return 1;
        }
        for (const auto& event : events) {
            std::printf("%02X %u\n", static_cast<unsigned>(event.type), event.value);
        }
        return 0;
    }
    std::printf("Uso: redragon-cli [state | raw <endereço-hex> <tamanho> | macro <índice>]\n");
    return 1;
}
