#include "i18n.h"
#include <cstdio>
#include <cstring>

namespace ltgui {

// --- Locale ---

std::string Locale::toString() const {
    std::string s = language;
    if (!country.empty()) { s += "-"; s += country; }
    if (!variant.empty()) { s += "-"; s += variant; }
    return s;
}

bool Locale::operator==(const Locale& o) const {
    return language == o.language && country == o.country && variant == o.variant;
}

Locale I18n::parse(const std::string& str) {
    Locale loc;
    size_t p1 = str.find('-');
    if (p1 == std::string::npos) {
        loc.language = str;
        return loc;
    }
    loc.language = str.substr(0, p1);
    size_t p2 = str.find('-', p1 + 1);
    if (p2 == std::string::npos) {
        loc.country = str.substr(p1 + 1);
        return loc;
    }
    loc.country = str.substr(p1 + 1, p2 - p1 - 1);
    loc.variant = str.substr(p2 + 1);
    return loc;
}

// --- PluralRules ---

int PluralRules::formIndex(const Locale& locale, int64_t n) {
    const std::string& lang = locale.language;

    // Chinese, Japanese, Korean, Vietnamese, Thai — always Other
    if (lang == "zh" || lang == "ja" || lang == "ko" || lang == "vi" || lang == "th") {
        return Other;
    }

    // English, German, Dutch, Italian, Spanish, Portuguese, etc.
    // One: n == 1, Other: everything else
    if (lang == "en" || lang == "de" || lang == "nl" || lang == "it" ||
        lang == "es" || lang == "pt" || lang == "da" || lang == "nb" ||
        lang == "nn" || lang == "sv" || lang == "fi" || lang == "el" ||
        lang == "he" || lang == "hu" || lang == "tr" || lang == "ca" ||
        lang == "no" || lang == "bg" || lang == "et" || lang == "fa" ||
        lang == "hi" || lang == "id" || lang == "ms" || lang == "sw" ||
        lang == "ta" || lang == "te" || lang == "ur" || lang == "eu" ||
        lang == "gl" || lang == "af" || lang == "bn" || lang == "gu" ||
        lang == "is" || lang == "kn" || lang == "ky" || lang == "lb" ||
        lang == "mk" || lang == "ml" || lang == "mr" || lang == "ne" ||
        lang == "pa" || lang == "si" || lang == "sq" || lang == "zu") {
        return n == 1 ? One : Other;
    }

    // French, Brazilian Portuguese — One for 0 or 1
    if (lang == "fr" || (lang == "pt" && locale.country == "BR")) {
        return (n == 0 || n == 1) ? One : Other;
    }

    // Russian, Ukrainian, Belarusian, Serbian, Croatian
    // One: n % 10 == 1 && n % 100 != 11
    // Few: n % 10 in 2..4 && n % 100 not in 12..14
    // Other: everything else
    if (lang == "ru" || lang == "uk" || lang == "be" || lang == "sr" || lang == "hr" ||
        lang == "bs" || lang == "sh") {
        int m10 = n % 10, m100 = n % 100;
        if (m10 == 1 && m100 != 11) return One;
        if (m10 >= 2 && m10 <= 4 && (m100 < 12 || m100 > 14)) return Few;
        return Other;
    }

    // Polish — One: n == 1; Few: n % 10 in 2..4 && n % 100 not in 12..14; Many: n != 1 && n % 10 in 0..1 or n%10 in 5..9 or n%100 in 12..14
    if (lang == "pl") {
        int m10 = n % 10, m100 = n % 100;
        if (n == 1) return One;
        if (m10 >= 2 && m10 <= 4 && (m100 < 12 || m100 > 14)) return Few;
        if ((n != 1 && (m10 <= 1 || m10 >= 5)) || (m100 >= 12 && m100 <= 14)) return Many;
        return Other;
    }

    // Czech, Slovak
    if (lang == "cs" || lang == "sk") {
        if (n == 1) return One;
        if (n >= 2 && n <= 4) return Few;
        return Other;
    }

    // Arabic
    if (lang == "ar") {
        int m100 = n % 100;
        if (n == 0) return Zero;
        if (n == 1) return One;
        if (n == 2) return Two;
        if (m100 >= 3 && m100 <= 10) return Few;
        if (m100 >= 11 && m100 <= 99) return Many;
        return Other;
    }

    // Irish
    if (lang == "ga") {
        if (n == 1) return One;
        if (n == 2) return Two;
        if (n >= 3 && n <= 6) return Few;
        if (n >= 7 && n <= 10) return Many;
        return Other;
    }

    // Welsh
    if (lang == "cy") {
        if (n == 0) return Zero;
        if (n == 1) return One;
        if (n == 2) return Two;
        if (n == 3) return Few;
        if (n == 6) return Many;
        return Other;
    }

    // Romanian
    if (lang == "ro") {
        if (n == 1) return One;
        if (n == 0 || (n % 100 >= 1 && n % 100 <= 19)) return Few;
        return Other;
    }

    // Lithuanian
    if (lang == "lt") {
        int m10 = n % 10, m100 = n % 100;
        if (m10 == 1 && m100 != 11) return One;
        if (m10 >= 2 && m10 <= 9 && (m100 < 11 || m100 > 19)) return Few;
        return Other;
    }

    // Latvian — Zero for 0, One for n%10==1 && n%100!=11, Other
    if (lang == "lv") {
        if (n == 0) return Zero;
        if (n % 10 == 1 && n % 100 != 11) return One;
        return Other;
    }

    // Maltese — One, Few, Many, Other
    if (lang == "mt") {
        int m100 = n % 100;
        if (n == 1) return One;
        if (n == 0 || (m100 >= 2 && m100 <= 10)) return Few;
        if (m100 >= 11 && m100 <= 19) return Many;
        return Other;
    }

    // Macedonian — One for n%10==1, Other
    if (lang == "sl") {
        int m100 = n % 100;
        if (m100 == 1) return One;
        if (m100 == 2) return Two;
        if (m100 >= 3 && m100 <= 4) return Few;
        return Other;
    }

    return Other;
}

// --- TranslationTable ---

void TranslationTable::add(const std::string& key, const std::string& value) {
    entries_[key] = value;
}

void TranslationTable::addPlural(const std::string& key,
                                  const std::string& zero, const std::string& one,
                                  const std::string& two, const std::string& few,
                                  const std::string& many, const std::string& other) {
    plurals_[0][key] = zero;
    plurals_[1][key] = one;
    plurals_[2][key] = two;
    plurals_[3][key] = few;
    plurals_[4][key] = many;
    plurals_[5][key] = other;
}

std::string TranslationTable::get(const std::string& key) const {
    auto it = entries_.find(key);
    if (it != entries_.end()) return it->second;
    return key;
}

std::string TranslationTable::getPlural(const std::string& key, int64_t n) const {
    int form = PluralRules::formIndex(I18n::instance().locale(), n);
    const auto& map = plurals_[form];
    auto it = map.find(key);
    if (it != map.end() && !it->second.empty()) return it->second;
    // Fallback to Other form
    if (form != PluralRules::Other) {
        const auto& otherMap = plurals_[PluralRules::Other];
        auto oit = otherMap.find(key);
        if (oit != otherMap.end() && !oit->second.empty()) return oit->second;
    }
    // Fallback to non-plural
    return get(key);
}

// Minimal JSON parser — avoids external dependencies.
// Supports: {"key":"value","key2":"value2"} with nested objects for plurals.
namespace {

static std::string jsonDecodeString(const std::string& s, size_t& pos) {
    if (pos >= s.size() || s[pos] != '"') return {};
    pos++; // skip opening "
    std::string result;
    while (pos < s.size()) {
        char c = s[pos];
        if (c == '"') { pos++; return result; }
        if (c == '\\' && pos + 1 < s.size()) {
            pos++;
            char esc = s[pos];
            if (esc == 'n') result += '\n';
            else if (esc == 't') result += '\t';
            else if (esc == '\\') result += '\\';
            else if (esc == '"') result += '"';
            else result += esc;
        } else {
            result += c;
        }
        pos++;
    }
    return result;
}

static void jsonSkipWhitespace(const std::string& s, size_t& pos) {
    while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' ||
           s[pos] == '\n' || s[pos] == '\r')) pos++;
}

static void jsonSkipValue(const std::string& s, size_t& pos) {
    jsonSkipWhitespace(s, pos);
    if (pos >= s.size()) return;
    if (s[pos] == '{') {
        int depth = 1; pos++;
        while (pos < s.size() && depth > 0) {
            if (s[pos] == '{') depth++;
            else if (s[pos] == '}') depth--;
            else if (s[pos] == '"') { pos++; while (pos < s.size() && s[pos] != '"') { if (s[pos] == '\\') pos++; pos++; } }
            pos++;
        }
    } else if (s[pos] == '[') {
        int depth = 1; pos++;
        while (pos < s.size() && depth > 0) {
            if (s[pos] == '[') depth++;
            else if (s[pos] == ']') depth--;
            else if (s[pos] == '"') { pos++; while (pos < s.size() && s[pos] != '"') { if (s[pos] == '\\') pos++; pos++; } }
            pos++;
        }
    } else if (s[pos] == '"') {
        jsonDecodeString(s, pos);
    } else {
        while (pos < s.size() && s[pos] != ',' && s[pos] != '}' && s[pos] != ']') pos++;
    }
}

} // namespace

bool TranslationTable::loadFromJsonString(const std::string& json) {
    size_t pos = 0;
    jsonSkipWhitespace(json, pos);
    if (pos >= json.size() || json[pos] != '{') return false;
    pos++;

    while (pos < json.size()) {
        jsonSkipWhitespace(json, pos);
        if (pos >= json.size()) break;
        if (json[pos] == '}') { pos++; break; }
        if (json[pos] == ',') { pos++; continue; }

        std::string key = jsonDecodeString(json, pos);
        if (key.empty()) break;

        jsonSkipWhitespace(json, pos);
        if (pos >= json.size() || json[pos] != ':') break;
        pos++;

        jsonSkipWhitespace(json, pos);
        if (pos >= json.size()) break;

        if (json[pos] == '{') {
            // Skip nested object values (e.g., "translations": {...})
            jsonSkipValue(json, pos);
        } else if (json[pos] == '"') {
            std::string value = jsonDecodeString(json, pos);
            add(key, value);
        } else if (json[pos] == '[') {
            pos++; // skip [
            std::string forms[6];
            int fi = 0;
            while (pos < json.size() && fi < 6) {
                jsonSkipWhitespace(json, pos);
                if (json[pos] == ']') { pos++; break; }
                if (json[pos] == ',') { pos++; continue; }
                if (json[pos] == '"') {
                    forms[fi] = jsonDecodeString(json, pos);
                    fi++;
                } else { break; }
            }
            addPlural(key, forms[0], forms[1], forms[2], forms[3], forms[4], forms[5]);
        }
    }
    return true;
}

bool TranslationTable::loadFromJsonFile(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0) { fclose(f); return false; }
    std::string content(size, '\0');
    fread(&content[0], 1, size, f);
    fclose(f);
    return loadFromJsonString(content);
}

// --- I18n ---

I18n& I18n::instance() {
    static I18n i18n;
    return i18n;
}

void I18n::setLocale(const Locale& locale) {
    if (locale_ == locale) return;
    locale_ = locale;
    onLocaleChanged.emit(locale_);
}

void I18n::addTable(const Locale& locale, TranslationTable table) {
    tables_[locale.toString()] = std::move(table);
}

void I18n::removeTable(const Locale& locale) {
    tables_.erase(locale.toString());
}

std::string I18n::tr(const std::string& key) const {
    auto it = tables_.find(locale_.toString());
    if (it != tables_.end()) return it->second.get(key);

    // Fallback: try language-only
    if (!locale_.country.empty()) {
        it = tables_.find(locale_.language);
        if (it != tables_.end()) return it->second.get(key);
    }

    return key;
}

std::string I18n::tr(const std::string& key, int64_t n) const {
    auto it = tables_.find(locale_.toString());
    if (it != tables_.end()) return it->second.getPlural(key, n);

    if (!locale_.country.empty()) {
        it = tables_.find(locale_.language);
        if (it != tables_.end()) return it->second.getPlural(key, n);
    }

    return key;
}

bool I18n::loadTableFromFile(const Locale& locale, const std::string& jsonPath) {
    TranslationTable table;
    if (!table.loadFromJsonFile(jsonPath)) return false;
    addTable(locale, std::move(table));
    return true;
}

} // namespace ltgui
