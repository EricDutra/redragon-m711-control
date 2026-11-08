#include "i18n.h"

#include <windows.h>

namespace i18n {

namespace {

Language g_language = Language::English;

constexpr wchar_t kRegistryKey[] = L"Software\\RedragonControl";
constexpr wchar_t kRegistryValue[] = L"Language";

}

Language language() {
    return g_language;
}

void setLanguage(Language language) {
    g_language = language;
}

void loadLanguage() {
    DWORD value = 0;
    DWORD size = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, kRegistryKey, kRegistryValue, RRF_RT_REG_DWORD, nullptr, &value, &size) ==
        ERROR_SUCCESS) {
        g_language = value == 1 ? Language::Portuguese : Language::English;
    }
}

void saveLanguage() {
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRegistryKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) !=
        ERROR_SUCCESS) {
        return;
    }
    DWORD value = g_language == Language::Portuguese ? 1 : 0;
    RegSetValueExW(key, kRegistryValue, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
    RegCloseKey(key);
}

const char* tr(const char* english, const char* portuguese) {
    return g_language == Language::Portuguese ? portuguese : english;
}

}
