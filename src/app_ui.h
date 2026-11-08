#pragma once

#include <chrono>
#include <string>
#include <vector>

#include "mouse_protocol.h"

class App {
public:
    void start();
    void draw();
    void onKey(int vk, bool down, bool extended);
    bool isRecording() const { return recording_; }

private:
    void connect();
    void reload();
    void setStatus(const std::string& message, bool error);
    void reportResult(bool ok, const char* success);

    void drawHeader();
    void drawLanguageSelector();
    void drawLightingTab();
    void drawDpiTab();
    void drawButtonsTab();
    void drawPollTab();
    void drawMacroTab();
    void drawButtonEditor(int index, rdc::ButtonAction& action);
    void drawMacroEvents();
    void recordEvent(rdc::MacroEventType type, uint16_t value);

    rdc::Mouse mouse_;
    rdc::MouseState state_;
    bool loaded_ = false;
    int profile_ = 0;

    std::string status_;
    bool statusError_ = false;

    int macroIndex_ = 0;
    std::vector<rdc::MacroEvent> macroEvents_;
    bool macroLoaded_ = false;
    bool recording_ = false;
    bool recordDelays_ = true;
    std::chrono::steady_clock::time_point lastRecordTime_;
    int newDelayMs_ = 50;
    int newKeyIndex_ = 0;
};
