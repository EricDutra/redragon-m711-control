#pragma once

namespace i18n {

enum class Language { English, Portuguese };

Language language();
void setLanguage(Language language);
void loadLanguage();
void saveLanguage();
const char* tr(const char* english, const char* portuguese);

}
