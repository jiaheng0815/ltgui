#pragma once
#include <string>

namespace ltgui {
namespace utf8 {

inline int codePointLen(unsigned char c) {
    // Valid UTF-8 lead byte patterns (RFC 3629):
    // 0xxxxxxx           → 1 byte  (0x00–0x7F)
    // 110xxxxx           → 2 bytes (0xC2–0xDF; reject overlong 0xC0/0xC1)
    // 1110xxxx           → 3 bytes (0xE0–0xEF)
    // 11110xxx           → 4 bytes (0xF0–0xF4; reject 0xF5–0xF7, RFC 3629 limit)
    if ((c & 0x80) == 0) return 1;
    if ((c & 0xE0) == 0xC0) {
        // Reject overlong 2-byte sequences: 0xC0 and 0xC1 always encode
        // codepoints ≤ 0x7F which must use 1-byte form.
        if (c < 0xC2) return 1;
        return 2;
    }
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) {
        // RFC 3629 restricts max codepoint to U+10FFFF.
        // Bytes 0xF5–0xF7 would decode beyond that; reject them.
        if (c > 0xF4) return 1;
        return 4;
    }
    // Invalid lead bytes (0xFE, 0xFF, etc.) or continuation bytes at start:
    // treat as a single-byte raw byte so cursor navigation doesn't get stuck.
    return 1;
}

inline std::string encode(unsigned int cp) {
    // Reject surrogates (U+D800–U+DFFF) and out-of-range codepoints
    if ((cp >= 0xD800 && cp <= 0xDFFF) || cp >= 0x110000)
        return {};

    std::string out;
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
    return out;
}

inline int prevPos(const std::string& s, int pos) {
    if (pos <= 0) return 0;
    int p = pos - 1;
    while (p > 0 && (static_cast<unsigned char>(s[p]) & 0xC0) == 0x80) p--;
    return p;
}

inline int nextPos(const std::string& s, int pos) {
    int len = static_cast<int>(s.size());
    if (pos >= len) return len;
    return pos + codePointLen(static_cast<unsigned char>(s[pos]));
}

} // namespace utf8
} // namespace ltgui
