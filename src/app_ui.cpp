#include "app_ui.h"

#include <algorithm>
#include <cstdio>
#include <iterator>

#include "i18n.h"
#include "imgui.h"
#include "keys.h"

using i18n::tr;

namespace {

struct Preset {
    uint8_t code;
    uint8_t arg1;
    const char* name;
    const char* namePt;
    bool verified;
};

constexpr Preset kPresets[] = {
    {0x81, 0x00, "Left click", "Clique esquerdo", true},
    {0x82, 0x00, "Right click", "Clique direito", true},
    {0x83, 0x00, "Middle click", "Clique do meio", true},
    {0x85, 0x00, "Forward (side)", "Avançar (lateral)", true},
    {0x84, 0x00, "Back (side)", "Voltar (lateral)", true},
    {0x8A, 0x00, "DPI +", "DPI +", true},
    {0x89, 0x00, "DPI -", "DPI -", true},
    {0x88, 0x00, "DPI cycle", "DPI: ciclo", false},
    {0x97, 0x00, "Profile +", "Perfil +", false},
    {0x98, 0x00, "Profile -", "Perfil -", false},
    {0x8D, 0x00, "Profile cycle", "Perfil: ciclo", false},
    {0x8B, 0x00, "Polling rate +", "Polling rate +", false},
    {0x8C, 0x00, "Polling rate -", "Polling rate -", false},
    {0x94, 0x00, "Scroll up", "Rolar para cima", false},
    {0x95, 0x00, "Scroll down", "Rolar para baixo", false},
    {0x9B, 0x04, "Button 8 factory function", "Função de fábrica do botão 8", true},
    {0x00, 0x00, "Disabled", "Desativado", true},
};

enum ButtonKind { KindPreset, KindKeyboard, KindMedia, KindMacro, KindRaw, KindCount };
const char* kindName(int kind) {
    switch (kind) {
    case KindPreset: return tr("Function", "Função");
    case KindKeyboard: return tr("Key / shortcut", "Tecla / atalho");
    case KindMedia: return tr("Media", "Mídia");
    case KindMacro: return "Macro";
    default: return tr("Custom (hex)", "Personalizado (hex)");
    }
}

constexpr uint8_t kCodeKeyboard = 0x8F;
constexpr uint8_t kCodeMedia = 0x8E;
constexpr uint8_t kCodeMacro = 0x91;

const char* buttonLabel(int index) {
    switch (index) {
    case 0: return tr("Button 1 (left)", "Botão 1 (esquerdo)");
    case 1: return tr("Button 2 (right)", "Botão 2 (direito)");
    case 2: return tr("Button 3 (wheel)", "Botão 3 (roda)");
    case 3: return tr("Button 4 (side front)", "Botão 4 (lateral frente)");
    case 4: return tr("Button 5 (side rear)", "Botão 5 (lateral trás)");
    case 5: return tr("Button 6 (DPI +)", "Botão 6 (DPI +)");
    case 6: return tr("Button 7 (DPI -)", "Botão 7 (DPI -)");
    default: return tr("Button 8", "Botão 8");
    }
}

constexpr rdc::LedEffect kEffects[] = {
    rdc::LedEffect::Static, rdc::LedEffect::Breathing, rdc::LedEffect::Flash, rdc::LedEffect::Reactive,
    rdc::LedEffect::Rainbow, rdc::LedEffect::Wave, rdc::LedEffect::GoWithoutTrace, rdc::LedEffect::Off,
};

constexpr int kPollRates[] = {125, 250, 500, 1000};

constexpr rdc::MacroRepeat kRepeatModes[] = {rdc::MacroRepeat::Count, rdc::MacroRepeat::WhileHeld,
                                             rdc::MacroRepeat::Toggle};

const Preset* findPreset(const rdc::ButtonAction& action) {
    for (const auto& preset : kPresets) {
        if (preset.code == action.code && (preset.code != 0x9B || preset.arg1 == action.arg1)) {
            return &preset;
        }
    }
    return nullptr;
}

ButtonKind kindOf(const rdc::ButtonAction& action) {
    if (action.code == kCodeKeyboard) {
        return KindKeyboard;
    }
    if (action.code == kCodeMedia) {
        return KindMedia;
    }
    if (action.code == kCodeMacro) {
        return KindMacro;
    }
    if (findPreset(action)) {
        return KindPreset;
    }
    return KindRaw;
}

rdc::ButtonAction defaultAction(ButtonKind kind) {
    switch (kind) {
    case KindKeyboard: return {kCodeKeyboard, 0, 0x04, 0};
    case KindMedia: return {kCodeMedia, 0x01, 0xCD, 0x00};
    case KindMacro: return {kCodeMacro, 0x00, 0x01, 0x00};
    default: return {0x81, 0, 0, 0};
    }
}

std::string presetName(const Preset& preset) {
    return std::string(tr(preset.name, preset.namePt)) + (preset.verified ? "" : " *");
}

bool hasLeftClick(const std::array<rdc::ButtonAction, rdc::kButtonCount>& buttons) {
    return std::any_of(buttons.begin(), buttons.end(), [](const rdc::ButtonAction& a) { return a.code == 0x81; });
}

std::string describeEvent(const rdc::MacroEvent& event) {
    char buffer[96];
    switch (event.type) {
    case rdc::MacroEventType::KeyDown:
        std::snprintf(buffer, sizeof(buffer), "%-10s %s", tr("Press", "Pressionar"), keys::nameFromHid(static_cast<uint8_t>(event.value)));
        break;
    case rdc::MacroEventType::KeyUp:
        std::snprintf(buffer, sizeof(buffer), "%-10s %s", tr("Release", "Soltar"), keys::nameFromHid(static_cast<uint8_t>(event.value)));
        break;
    case rdc::MacroEventType::MouseDown:
        std::snprintf(buffer, sizeof(buffer), "%-10s %s", tr("Press", "Pressionar"), keys::mouseButtonName(event.value));
        break;
    case rdc::MacroEventType::MouseUp:
        std::snprintf(buffer, sizeof(buffer), "%-10s %s", tr("Release", "Soltar"), keys::mouseButtonName(event.value));
        break;
    case rdc::MacroEventType::Delay:
        std::snprintf(buffer, sizeof(buffer), "%-10s %d ms", tr("Wait", "Esperar"), event.value * 10);
        break;
    }
    return buffer;
}

void helpMarker(const char* text) {
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

}

void App::start() {
    connect();
}

void App::connect() {
    loaded_ = false;
    if (!mouse_.connect()) {
        setStatus(mouse_.lastError(), true);
        return;
    }
    reload();
}

void App::reload() {
    if (!mouse_.readState(state_)) {
        setStatus(std::string(tr("Error reading the mouse: ", "Erro ao ler o mouse: ")) + mouse_.lastError(), true);
        loaded_ = false;
        return;
    }
    loaded_ = true;
    profile_ = std::clamp<int>(state_.activeProfile, 0, rdc::kProfileCount - 1);
    macroLoaded_ = false;
    setStatus(tr("Settings read from the mouse.", "Configuração lida do mouse."), false);
}

void App::setStatus(const std::string& message, bool error) {
    status_ = message;
    statusError_ = error;
}

void App::reportResult(bool ok, const char* success) {
    if (ok) {
        setStatus(success, false);
    } else {
        setStatus(std::string(tr("Failed: ", "Falha: ")) + mouse_.lastError(), true);
    }
}

void App::draw() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("main", nullptr, flags);

    drawHeader();
    ImGui::Separator();

    if (loaded_) {
        if (ImGui::BeginTabBar("tabs")) {
            if (ImGui::BeginTabItem(tr("Lighting###lighting", "Iluminação###lighting"))) {
                drawLightingTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("DPI###dpi")) {
                drawDpiTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(tr("Buttons###buttons", "Botões###buttons"))) {
                drawButtonsTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Polling rate###poll")) {
                drawPollTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Macros###macros")) {
                drawMacroTab();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    } else {
        ImGui::Spacing();
        ImGui::TextWrapped("%s", tr("Connect the Redragon M711 mouse (04D9:FC30) and click \"Connect\".",
                                    "Conecte o mouse Redragon M711 (04D9:FC30) e clique em \"Conectar\"."));
    }

    float statusHeight = ImGui::GetFrameHeightWithSpacing();
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - statusHeight - ImGui::GetStyle().WindowPadding.y);
    ImGui::Separator();
    ImVec4 color = statusError_ ? ImVec4(1.0f, 0.45f, 0.4f, 1.0f) : ImVec4(0.55f, 0.85f, 0.55f, 1.0f);
    ImGui::TextColored(color, "%s", status_.c_str());

    ImGui::End();
}

void App::drawHeader() {
    if (mouse_.isConnected()) {
        ImGui::Text("Redragon M711 (2710-H)  |  firmware 0x%04X", mouse_.firmwareRevision());
    } else {
        ImGui::TextDisabled("%s", tr("Mouse disconnected", "Mouse desconectado"));
    }
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 330.0f);
    drawLanguageSelector();
    ImGui::SameLine();
    const char* primary = mouse_.isConnected() ? tr("Reload###primary", "Recarregar###primary")
                                               : tr("Connect###primary", "Conectar###primary");
    if (ImGui::Button(primary, ImVec2(100, 0))) {
        if (mouse_.isConnected()) {
            reload();
        } else {
            connect();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("Reconnect###reconnect", "Reconectar###reconnect"), ImVec2(100, 0))) {
        connect();
    }

    if (!loaded_) {
        return;
    }

    ImGui::TextUnformatted(tr("Editing profile:", "Perfil em edição:"));
    for (int p = 0; p < rdc::kProfileCount; ++p) {
        ImGui::SameLine();
        char label[32];
        std::snprintf(label, sizeof(label), "%d%s", p + 1, p == state_.activeProfile ? " *" : "");
        bool selected = profile_ == p;
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }
        if (ImGui::Button(label, ImVec2(44, 0))) {
            profile_ = p;
        }
        if (selected) {
            ImGui::PopStyleColor();
        }
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(profile_ == state_.activeProfile);
    if (ImGui::Button(tr("Activate this profile on the mouse###activate", "Ativar este perfil no mouse###activate"))) {
        bool ok = mouse_.writeActiveProfile(static_cast<uint8_t>(profile_));
        if (ok) {
            state_.activeProfile = static_cast<uint8_t>(profile_);
        }
        reportResult(ok, tr("Profile activated.", "Perfil ativado."));
    }
    ImGui::EndDisabled();
    helpMarker(tr("* marks the profile active on the mouse. Each profile stores its own lighting, DPI, buttons and polling rate.",
                   "* indica o perfil ativo no mouse. Cada perfil guarda LED, DPI, botões e polling rate próprios."));
}

void App::drawLanguageSelector() {
    const char* names[] = {"English", "Português"};
    int current = i18n::language() == i18n::Language::Portuguese ? 1 : 0;
    ImGui::SetNextItemWidth(110);
    if (ImGui::Combo("##language", &current, names, IM_ARRAYSIZE(names))) {
        i18n::setLanguage(current == 1 ? i18n::Language::Portuguese : i18n::Language::English);
        i18n::saveLanguage();
    }
}

void App::drawLightingTab() {
    rdc::LedSettings& led = state_.profiles[profile_].led;
    ImGui::Spacing();
    ImGui::PushItemWidth(300);

    if (ImGui::BeginCombo(tr("Effect###effect", "Efeito###effect"), rdc::ledEffectName(led.effect))) {
        for (auto effect : kEffects) {
            if (ImGui::Selectable(rdc::ledEffectName(effect), effect == led.effect)) {
                led.effect = effect;
            }
        }
        ImGui::EndCombo();
    }

    float color[3] = {led.red / 255.0f, led.green / 255.0f, led.blue / 255.0f};
    bool usesColor = led.effect == rdc::LedEffect::Static || led.effect == rdc::LedEffect::Breathing ||
                     led.effect == rdc::LedEffect::Flash || led.effect == rdc::LedEffect::Reactive ||
                     led.effect == rdc::LedEffect::GoWithoutTrace;
    ImGui::BeginDisabled(!usesColor);
    ImGui::SetNextItemWidth(240);
    if (ImGui::ColorPicker3(tr("Color###color", "Cor###color"), color, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_DisplayRGB)) {
        led.red = static_cast<uint8_t>(color[0] * 255.0f + 0.5f);
        led.green = static_cast<uint8_t>(color[1] * 255.0f + 0.5f);
        led.blue = static_cast<uint8_t>(color[2] * 255.0f + 0.5f);
    }
    ImGui::EndDisabled();

    int speed = std::clamp<int>(led.speed, 1, 8);
    ImGui::BeginDisabled(!rdc::ledEffectHasSpeed(led.effect));
    if (ImGui::SliderInt(tr("Speed###speed", "Velocidade###speed"), &speed, 1, 8)) {
        led.speed = static_cast<uint8_t>(speed);
    }
    ImGui::EndDisabled();

    int brightness = std::clamp<int>(led.brightness, 1, 3);
    ImGui::BeginDisabled(led.effect == rdc::LedEffect::Off);
    if (ImGui::SliderInt(tr("Brightness###brightness", "Brilho###brightness"), &brightness, 1, 3)) {
        led.brightness = static_cast<uint8_t>(brightness);
    }
    ImGui::EndDisabled();

    ImGui::PopItemWidth();
    ImGui::Spacing();
    if (ImGui::Button(tr("Apply lighting###applyLed", "Aplicar iluminação###applyLed"), ImVec2(180, 0))) {
        if (led.effect == rdc::LedEffect::Unknown) {
            led.effect = rdc::LedEffect::Static;
        }
        reportResult(mouse_.writeLed(profile_, led), tr("Lighting applied.", "Iluminação aplicada."));
    }
}

void App::drawDpiTab() {
    rdc::Profile& profile = state_.profiles[profile_];
    ImGui::Spacing();
    ImGui::TextWrapped("%s", tr("Up to 6200 DPI the sensor works natively; above that the firmware uses interpolation.",
                                "Até 6200 DPI o sensor trabalha nativamente; acima disso o firmware usa interpolação."));
    ImGui::Spacing();

    if (ImGui::BeginTable("dpi", 4, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn(tr("Stage", "Estágio"));
        ImGui::TableSetupColumn(tr("Active", "Ativo"));
        ImGui::TableSetupColumn(tr("Enabled", "Habilitado"));
        ImGui::TableSetupColumn("DPI", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        int active = profile.activeDpiStage;
        for (int i = 0; i < rdc::kDpiStageCount; ++i) {
            rdc::DpiStage& stage = profile.dpi[i];
            ImGui::PushID(i);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%d", i + 1);
            ImGui::TableNextColumn();
            if (ImGui::RadioButton("##active", &active, i)) {
                profile.activeDpiStage = static_cast<uint8_t>(i);
            }
            ImGui::TableNextColumn();
            ImGui::Checkbox("##enabled", &stage.enabled);
            ImGui::TableNextColumn();
            int dpi = rdc::dpiFromIndex(std::clamp(stage.xIndex, rdc::kDpiMinIndex, rdc::kDpiMaxIndex));
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderInt("##dpi", &dpi, rdc::dpiFromIndex(rdc::kDpiMinIndex),
                                 rdc::dpiFromIndex(rdc::kDpiMaxIndex), "%d DPI")) {
                dpi = (dpi + 50) / 100 * 100;
                stage.xIndex = rdc::dpiIndexFromDpi(dpi);
                stage.yIndex = stage.xIndex;
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();
    if (ImGui::Button(tr("Apply DPI###applyDpi", "Aplicar DPI###applyDpi"), ImVec2(180, 0))) {
        if (!profile.dpi[profile.activeDpiStage].enabled) {
            setStatus(tr("The active stage must be enabled.", "O estágio ativo precisa estar habilitado."), true);
        } else {
            reportResult(mouse_.writeDpi(profile_, profile.activeDpiStage, profile.dpi), tr("DPI applied.", "DPI aplicado."));
        }
    }
}

void App::drawButtonsTab() {
    rdc::Profile& profile = state_.profiles[profile_];
    ImGui::Spacing();
    if (ImGui::BeginTable("buttons", 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn(tr("Button", "Botão"));
        ImGui::TableSetupColumn(tr("Type", "Tipo"));
        ImGui::TableSetupColumn(tr("Action", "Ação"), ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        for (int i = 0; i < rdc::kButtonCount; ++i) {
            ImGui::PushID(i);
            drawButtonEditor(i, profile.buttons[i]);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::TextDisabled("%s", tr("Functions marked with * were inferred from the original software but are not tested yet.",
                                  "Funções marcadas com * foram inferidas do software original, mas ainda não testadas."));
    ImGui::Spacing();
    if (ImGui::Button(tr("Apply buttons###applyButtons", "Aplicar botões###applyButtons"), ImVec2(180, 0))) {
        if (!hasLeftClick(profile.buttons)) {
            setStatus(tr("At least one button must remain as left click.", "Pelo menos um botão precisa continuar como clique esquerdo."), true);
        } else {
            reportResult(mouse_.writeButtons(profile_, profile.buttons), tr("Buttons applied.", "Botões aplicados."));
        }
    }
}

void App::drawButtonEditor(int index, rdc::ButtonAction& action) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(buttonLabel(index));

    ImGui::TableNextColumn();
    ButtonKind kind = kindOf(action);
    ImGui::SetNextItemWidth(170);
    if (ImGui::BeginCombo("##kind", kindName(kind))) {
        for (int k = 0; k < KindCount; ++k) {
            if (ImGui::Selectable(kindName(k), k == kind) && k != kind) {
                if (k != KindRaw) {
                    action = defaultAction(static_cast<ButtonKind>(k));
                }
            }
        }
        ImGui::EndCombo();
    }

    ImGui::TableNextColumn();
    switch (kind) {
    case KindPreset: {
        const Preset* current = findPreset(action);
        std::string currentName = current ? presetName(*current) : "?";
        ImGui::SetNextItemWidth(260);
        if (ImGui::BeginCombo("##preset", currentName.c_str())) {
            for (const auto& preset : kPresets) {
                std::string name = presetName(preset);
                if (ImGui::Selectable(name.c_str(), &preset == current)) {
                    action = {preset.code, preset.arg1, 0, 0};
                }
            }
            ImGui::EndCombo();
        }
        break;
    }
    case KindKeyboard: {
        bool ctrl = action.arg1 & 0x01;
        bool shift = action.arg1 & 0x02;
        bool alt = action.arg1 & 0x04;
        bool win = action.arg1 & 0x08;
        ImGui::Checkbox("Ctrl", &ctrl);
        ImGui::SameLine();
        ImGui::Checkbox("Shift", &shift);
        ImGui::SameLine();
        ImGui::Checkbox("Alt", &alt);
        ImGui::SameLine();
        ImGui::Checkbox("Win", &win);
        action.arg1 = static_cast<uint8_t>((ctrl ? 0x01 : 0) | (shift ? 0x02 : 0) | (alt ? 0x04 : 0) | (win ? 0x08 : 0));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(140);
        const char* modifierOnly = tr("(modifier only)", "(só modificador)");
        if (ImGui::BeginCombo("##key", action.arg2 ? keys::nameFromHid(action.arg2) : modifierOnly)) {
            if (ImGui::Selectable(modifierOnly, action.arg2 == 0)) {
                action.arg2 = 0;
            }
            for (const auto& key : keys::keyboardKeys()) {
                if (ImGui::Selectable(keys::label(key), key.hid == action.arg2)) {
                    action.arg2 = key.hid;
                }
            }
            ImGui::EndCombo();
        }
        action.arg3 = 0;
        break;
    }
    case KindMedia: {
        uint16_t usage = static_cast<uint16_t>(action.arg2 | (action.arg3 << 8));
        ImGui::SetNextItemWidth(260);
        if (ImGui::BeginCombo("##media", keys::nameFromMedia(usage))) {
            for (const auto& media : keys::mediaKeys()) {
                if (ImGui::Selectable(keys::label(media), media.usage == usage)) {
                    action = {kCodeMedia, 0x01, static_cast<uint8_t>(media.usage & 0xFF),
                              static_cast<uint8_t>(media.usage >> 8)};
                }
            }
            ImGui::EndCombo();
        }
        break;
    }
    case KindMacro: {
        int slot = action.arg1 & 0x3F;
        int modeIndex = (action.arg1 & 0x80) ? 1 : (action.arg1 & 0x40) ? 2 : 0;
        int count = action.arg2 | (action.arg3 << 8);
        ImGui::SetNextItemWidth(110);
        char slotLabel[32];
        std::snprintf(slotLabel, sizeof(slotLabel), "Macro %d", slot + 1);
        if (ImGui::BeginCombo("##slot", slotLabel)) {
            for (int m = 0; m < rdc::kMacroCount; ++m) {
                char label[32];
                std::snprintf(label, sizeof(label), "Macro %d", m + 1);
                if (ImGui::Selectable(label, m == slot)) {
                    slot = m;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(220);
        const char* repeatNames[] = {tr("Repeat N times", "Repetir N vezes"),
                                     tr("Repeat while held", "Repetir enquanto pressionado"),
                                     tr("Toggle (on/off)", "Alternar (liga/desliga)")};
        ImGui::Combo("##mode", &modeIndex, repeatNames, IM_ARRAYSIZE(repeatNames));
        if (modeIndex == 0) {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(90);
            if (count < 1 || count > 0xFFFE) {
                count = 1;
            }
            ImGui::InputInt(tr("times###times", "vezes###times"), &count);
            count = std::clamp(count, 1, 0xFFFE);
        } else {
            count = 0xFFFF;
        }
        action.code = kCodeMacro;
        action.arg1 = static_cast<uint8_t>(slot | static_cast<uint8_t>(kRepeatModes[modeIndex]));
        action.arg2 = static_cast<uint8_t>(count & 0xFF);
        action.arg3 = static_cast<uint8_t>(count >> 8);
        break;
    }
    default: {
        uint8_t* bytes[4] = {&action.code, &action.arg1, &action.arg2, &action.arg3};
        for (int b = 0; b < 4; ++b) {
            ImGui::PushID(b);
            ImGui::SetNextItemWidth(40);
            ImGui::InputScalar("##hex", ImGuiDataType_U8, bytes[b], nullptr, nullptr, "%02X",
                               ImGuiInputTextFlags_CharsHexadecimal);
            ImGui::PopID();
            if (b < 3) {
                ImGui::SameLine();
            }
        }
        break;
    }
    }
}

void App::drawPollTab() {
    rdc::Profile& profile = state_.profiles[profile_];
    ImGui::Spacing();
    int current = rdc::pollHzFromRaw(profile.pollRaw);
    ImGui::TextUnformatted(tr("Report rate (USB):", "Taxa de atualização (USB):"));
    for (int hz : kPollRates) {
        char label[16];
        std::snprintf(label, sizeof(label), "%d Hz", hz);
        if (ImGui::RadioButton(label, current == hz)) {
            profile.pollRaw = rdc::pollRawFromHz(hz);
        }
        ImGui::SameLine();
    }
    ImGui::NewLine();
    ImGui::Spacing();
    if (ImGui::Button(tr("Apply polling rate###applyPoll", "Aplicar polling rate###applyPoll"), ImVec2(180, 0))) {
        reportResult(mouse_.writePollRate(profile_, profile.pollRaw), tr("Polling rate applied.", "Polling rate aplicado."));
    }
}

void App::drawMacroTab() {
    ImGui::Spacing();
    ImGui::TextWrapped(tr("Macros are stored in the mouse memory (%d slots, up to %d events each) and work without this "
                          "program running. To use a macro, assign it to a button in the Buttons tab.",
                          "As macros ficam gravadas na memória do mouse (%d posições, até %d eventos cada) e funcionam "
                          "sem este programa aberto. Para usar uma macro, associe-a a um botão na aba Botões."),
                       rdc::kMacroCount, rdc::kMacroMaxEvents);
    ImGui::Spacing();

    ImGui::SetNextItemWidth(120);
    char slotLabel[32];
    std::snprintf(slotLabel, sizeof(slotLabel), "Macro %d", macroIndex_ + 1);
    ImGui::BeginDisabled(recording_);
    if (ImGui::BeginCombo("##macroSlot", slotLabel)) {
        for (int m = 0; m < rdc::kMacroCount; ++m) {
            char label[32];
            std::snprintf(label, sizeof(label), "Macro %d", m + 1);
            if (ImGui::Selectable(label, m == macroIndex_)) {
                macroIndex_ = m;
                macroLoaded_ = false;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("Read from mouse###readMacro", "Ler do mouse###readMacro")) || !macroLoaded_) {
        macroLoaded_ = true;
        if (mouse_.readMacro(macroIndex_, macroEvents_)) {
            setStatus(tr("Macro read from the mouse.", "Macro lida do mouse."), false);
        } else {
            setStatus(std::string(tr("Failed to read macro: ", "Falha ao ler macro: ")) + mouse_.lastError(), true);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("Write to mouse###writeMacro", "Gravar no mouse###writeMacro"))) {
        reportResult(mouse_.writeMacro(macroIndex_, macroEvents_), tr("Macro written to the mouse.", "Macro gravada no mouse."));
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("Clear###clearMacro", "Limpar###clearMacro"))) {
        macroEvents_.clear();
    }
    ImGui::EndDisabled();

    ImGui::Separator();

    if (!recording_) {
        if (ImGui::Button(tr("Record keys###record", "Gravar teclas###record"), ImVec2(130, 0))) {
            macroEvents_.clear();
            recording_ = true;
            lastRecordTime_ = std::chrono::steady_clock::now();
            setStatus(tr("Recording... press the keys. Click \"Stop\" to finish.",
                          "Gravando... pressione as teclas. Clique em \"Parar\" para terminar."),
                      false);
        }
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.15f, 0.15f, 1.0f));
        if (ImGui::Button(tr("Stop###stop", "Parar###stop"), ImVec2(130, 0))) {
            recording_ = false;
            setStatus(tr("Recording finished.", "Gravação encerrada."), false);
        }
        ImGui::PopStyleColor();
    }
    ImGui::SameLine();
    ImGui::Checkbox(tr("Record delays between keys###delays", "Gravar tempos entre teclas###delays"), &recordDelays_);

    ImGui::BeginDisabled(recording_);
    ImGui::SetNextItemWidth(150);
    const auto keyList = keys::keyboardKeys();
    newKeyIndex_ = std::clamp(newKeyIndex_, 0, static_cast<int>(keyList.size()) - 1);
    if (ImGui::BeginCombo("##newKey", keys::label(keyList[newKeyIndex_]))) {
        for (int k = 0; k < static_cast<int>(keyList.size()); ++k) {
            if (ImGui::Selectable(keys::label(keyList[k]), k == newKeyIndex_)) {
                newKeyIndex_ = k;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("+ Key###addKey", "+ Tecla###addKey"))) {
        uint8_t hid = keyList[newKeyIndex_].hid;
        macroEvents_.push_back({rdc::MacroEventType::KeyDown, hid});
        macroEvents_.push_back({rdc::MacroEventType::Delay, static_cast<uint16_t>(std::max(1, newDelayMs_ / 10))});
        macroEvents_.push_back({rdc::MacroEventType::KeyUp, hid});
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("+ Left click###addLeft", "+ Clique esq.###addLeft"))) {
        macroEvents_.push_back({rdc::MacroEventType::MouseDown, 0x01});
        macroEvents_.push_back({rdc::MacroEventType::Delay, static_cast<uint16_t>(std::max(1, newDelayMs_ / 10))});
        macroEvents_.push_back({rdc::MacroEventType::MouseUp, 0x01});
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("+ Right click###addRight", "+ Clique dir.###addRight"))) {
        macroEvents_.push_back({rdc::MacroEventType::MouseDown, 0x02});
        macroEvents_.push_back({rdc::MacroEventType::Delay, static_cast<uint16_t>(std::max(1, newDelayMs_ / 10))});
        macroEvents_.push_back({rdc::MacroEventType::MouseUp, 0x02});
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::InputInt("ms", &newDelayMs_, 10, 100);
    newDelayMs_ = std::clamp(newDelayMs_, 10, 655350);
    ImGui::SameLine();
    if (ImGui::Button(tr("+ Delay###addDelay", "+ Espera###addDelay"))) {
        macroEvents_.push_back({rdc::MacroEventType::Delay, static_cast<uint16_t>(newDelayMs_ / 10)});
    }
    ImGui::EndDisabled();

    int count = static_cast<int>(macroEvents_.size());
    ImVec4 countColor = count > rdc::kMacroMaxEvents ? ImVec4(1.0f, 0.45f, 0.4f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
    ImGui::TextColored(countColor, tr("%d / %d events", "%d / %d eventos"), count, rdc::kMacroMaxEvents);

    drawMacroEvents();
}

void App::drawMacroEvents() {
    ImVec2 size(0, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() * 1.5f);
    if (!ImGui::BeginChild("events", size, ImGuiChildFlags_Borders)) {
        ImGui::EndChild();
        return;
    }
    int removeIndex = -1;
    int moveFrom = -1;
    int moveTo = -1;
    for (int i = 0; i < static_cast<int>(macroEvents_.size()); ++i) {
        ImGui::PushID(i);
        rdc::MacroEvent& event = macroEvents_[i];
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%3d", i + 1);
        ImGui::SameLine();
        if (event.type == rdc::MacroEventType::Delay) {
            ImGui::TextUnformatted(tr("Wait", "Esperar"));
            ImGui::SameLine();
            int ms = event.value * 10;
            ImGui::SetNextItemWidth(110);
            if (ImGui::InputInt("ms##delay", &ms, 10, 100)) {
                event.value = static_cast<uint16_t>(std::clamp(ms, 10, 655350) / 10);
            }
        } else {
            ImGui::TextUnformatted(describeEvent(event).c_str());
        }
        ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - 140.0f);
        if (ImGui::ArrowButton("##up", ImGuiDir_Up) && i > 0) {
            moveFrom = i;
            moveTo = i - 1;
        }
        ImGui::SameLine();
        if (ImGui::ArrowButton("##down", ImGuiDir_Down) && i + 1 < static_cast<int>(macroEvents_.size())) {
            moveFrom = i;
            moveTo = i + 1;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton(tr("Remove###remove", "Remover###remove"))) {
            removeIndex = i;
        }
        ImGui::PopID();
    }
    if (recording_) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();

    if (moveFrom >= 0) {
        std::swap(macroEvents_[moveFrom], macroEvents_[moveTo]);
    }
    if (removeIndex >= 0) {
        macroEvents_.erase(macroEvents_.begin() + removeIndex);
    }
}

void App::recordEvent(rdc::MacroEventType type, uint16_t value) {
    auto now = std::chrono::steady_clock::now();
    if (recordDelays_ && !macroEvents_.empty()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRecordTime_).count();
        auto units = std::clamp<long long>(elapsed / 10, 1, 0xFFFF);
        macroEvents_.push_back({rdc::MacroEventType::Delay, static_cast<uint16_t>(units)});
    }
    lastRecordTime_ = now;
    macroEvents_.push_back({type, value});
}

void App::onKey(int vk, bool down, bool extended) {
    if (!recording_) {
        return;
    }
    uint8_t hid = keys::hidFromVk(vk, extended);
    if (hid == 0) {
        return;
    }
    recordEvent(down ? rdc::MacroEventType::KeyDown : rdc::MacroEventType::KeyUp, hid);
}
