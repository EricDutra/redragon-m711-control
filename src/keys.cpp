#include "keys.h"

#include <windows.h>

#include "i18n.h"

namespace keys {

namespace {

constexpr KeyInfo kKeys[] = {
    {0x04, 'A', "A"}, {0x05, 'B', "B"}, {0x06, 'C', "C"}, {0x07, 'D', "D"}, {0x08, 'E', "E"}, {0x09, 'F', "F"},
    {0x0A, 'G', "G"}, {0x0B, 'H', "H"}, {0x0C, 'I', "I"}, {0x0D, 'J', "J"}, {0x0E, 'K', "K"}, {0x0F, 'L', "L"},
    {0x10, 'M', "M"}, {0x11, 'N', "N"}, {0x12, 'O', "O"}, {0x13, 'P', "P"}, {0x14, 'Q', "Q"}, {0x15, 'R', "R"},
    {0x16, 'S', "S"}, {0x17, 'T', "T"}, {0x18, 'U', "U"}, {0x19, 'V', "V"}, {0x1A, 'W', "W"}, {0x1B, 'X', "X"},
    {0x1C, 'Y', "Y"}, {0x1D, 'Z', "Z"},
    {0x1E, '1', "1"}, {0x1F, '2', "2"}, {0x20, '3', "3"}, {0x21, '4', "4"}, {0x22, '5', "5"}, {0x23, '6', "6"},
    {0x24, '7', "7"}, {0x25, '8', "8"}, {0x26, '9', "9"}, {0x27, '0', "0"},
    {0x28, VK_RETURN, "Enter"}, {0x29, VK_ESCAPE, "Esc"}, {0x2A, VK_BACK, "Backspace"}, {0x2B, VK_TAB, "Tab"},
    {0x2C, VK_SPACE, "Space", "Espaço"}, {0x2D, VK_OEM_MINUS, "-"}, {0x2E, VK_OEM_PLUS, "="}, {0x2F, VK_OEM_4, "["},
    {0x30, VK_OEM_6, "]"}, {0x31, VK_OEM_5, "\\"}, {0x33, VK_OEM_1, ";"}, {0x34, VK_OEM_7, "'"},
    {0x35, VK_OEM_3, "`"}, {0x36, VK_OEM_COMMA, ","}, {0x37, VK_OEM_PERIOD, "."}, {0x38, VK_OEM_2, "/"},
    {0x39, VK_CAPITAL, "Caps Lock"},
    {0x3A, VK_F1, "F1"}, {0x3B, VK_F2, "F2"}, {0x3C, VK_F3, "F3"}, {0x3D, VK_F4, "F4"}, {0x3E, VK_F5, "F5"},
    {0x3F, VK_F6, "F6"}, {0x40, VK_F7, "F7"}, {0x41, VK_F8, "F8"}, {0x42, VK_F9, "F9"}, {0x43, VK_F10, "F10"},
    {0x44, VK_F11, "F11"}, {0x45, VK_F12, "F12"},
    {0x46, VK_SNAPSHOT, "Print Screen"}, {0x47, VK_SCROLL, "Scroll Lock"}, {0x48, VK_PAUSE, "Pause"},
    {0x49, VK_INSERT, "Insert"}, {0x4A, VK_HOME, "Home"}, {0x4B, VK_PRIOR, "Page Up"}, {0x4C, VK_DELETE, "Delete"},
    {0x4D, VK_END, "End"}, {0x4E, VK_NEXT, "Page Down"}, {0x4F, VK_RIGHT, "Right Arrow", "Seta direita"},
    {0x50, VK_LEFT, "Left Arrow", "Seta esquerda"}, {0x51, VK_DOWN, "Down Arrow", "Seta baixo"}, {0x52, VK_UP, "Up Arrow", "Seta cima"},
    {0x53, VK_NUMLOCK, "Num Lock"}, {0x54, VK_DIVIDE, "Num /"}, {0x55, VK_MULTIPLY, "Num *"},
    {0x56, VK_SUBTRACT, "Num -"}, {0x57, VK_ADD, "Num +"}, {0x59, VK_NUMPAD1, "Num 1"}, {0x5A, VK_NUMPAD2, "Num 2"},
    {0x5B, VK_NUMPAD3, "Num 3"}, {0x5C, VK_NUMPAD4, "Num 4"}, {0x5D, VK_NUMPAD5, "Num 5"}, {0x5E, VK_NUMPAD6, "Num 6"},
    {0x5F, VK_NUMPAD7, "Num 7"}, {0x60, VK_NUMPAD8, "Num 8"}, {0x61, VK_NUMPAD9, "Num 9"}, {0x62, VK_NUMPAD0, "Num 0"},
    {0x63, VK_DECIMAL, "Num ."}, {0x64, VK_OEM_102, "\\ (ABNT)"}, {0x65, VK_APPS, "Menu"},
    {0xE0, VK_LCONTROL, "Left Ctrl", "Ctrl esq."}, {0xE1, VK_LSHIFT, "Left Shift", "Shift esq."}, {0xE2, VK_LMENU, "Left Alt", "Alt esq."},
};

constexpr KeyInfo kExtraKeys[] = {
    {0xE3, VK_LWIN, "Left Win", "Win esq."},
    {0xE4, VK_RCONTROL, "Right Ctrl", "Ctrl dir."},
    {0xE5, VK_RSHIFT, "Right Shift", "Shift dir."},
    {0xE6, VK_RMENU, "Right Alt", "Alt dir."},
    {0xE7, VK_RWIN, "Right Win", "Win dir."},
};

constexpr MediaInfo kMedia[] = {
    {0x00CD, "Play/Pause"},
    {0x00B7, "Stop", "Parar"},
    {0x00B6, "Previous Track", "Faixa anterior"},
    {0x00B5, "Next Track", "Próxima faixa"},
    {0x00E9, "Volume +"},
    {0x00EA, "Volume -"},
    {0x00E2, "Mute", "Mudo"},
    {0x0183, "Media Player", "Player de mídia"},
    {0x018A, "E-mail"},
    {0x1FFF, "Browser: Home", "Navegador: início"},
    {0x20FF, "Browser: Back", "Navegador: voltar"},
    {0x21FF, "Browser: Forward", "Navegador: avançar"},
    {0x22FF, "Browser: Stop", "Navegador: parar"},
    {0x23FF, "Browser: Refresh", "Navegador: atualizar"},
    {0x24FF, "Browser: Search", "Navegador: pesquisar"},
    {0x25FF, "Browser: Favorites", "Navegador: favoritos"},
};

}

std::span<const KeyInfo> keyboardKeys() {
    return kKeys;
}

std::span<const MediaInfo> mediaKeys() {
    return kMedia;
}

const char* label(const KeyInfo& key) {
    return key.namePt ? i18n::tr(key.name, key.namePt) : key.name;
}

const char* label(const MediaInfo& media) {
    return media.namePt ? i18n::tr(media.name, media.namePt) : media.name;
}

uint8_t hidFromVk(int vk, bool extended) {
    if (vk == VK_RETURN && extended) {
        return 0x58;
    }
    if (vk == VK_CONTROL) {
        vk = extended ? VK_RCONTROL : VK_LCONTROL;
    } else if (vk == VK_MENU) {
        vk = extended ? VK_RMENU : VK_LMENU;
    }
    for (const auto& key : kKeys) {
        if (key.vk == vk) {
            return key.hid;
        }
    }
    for (const auto& key : kExtraKeys) {
        if (key.vk == vk) {
            return key.hid;
        }
    }
    return 0;
}

const char* nameFromHid(uint8_t hid) {
    if (hid == 0x58) {
        return "Num Enter";
    }
    for (const auto& key : kKeys) {
        if (key.hid == hid) {
            return label(key);
        }
    }
    for (const auto& key : kExtraKeys) {
        if (key.hid == hid) {
            return label(key);
        }
    }
    return "?";
}

const char* nameFromMedia(uint16_t usage) {
    for (const auto& media : kMedia) {
        if (media.usage == usage) {
            return label(media);
        }
    }
    return "?";
}

const char* mouseButtonName(uint16_t mask) {
    switch (mask) {
    case 0x01: return i18n::tr("Left button", "Botão esquerdo");
    case 0x02: return i18n::tr("Right button", "Botão direito");
    case 0x04: return i18n::tr("Middle button", "Botão do meio");
    case 0x08: return i18n::tr("Back button", "Botão voltar");
    case 0x10: return i18n::tr("Forward button", "Botão avançar");
    default: return i18n::tr("Button ?", "Botão ?");
    }
}

}
