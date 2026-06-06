#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "utf8.h"
#include <string>

using namespace ltgui;

// ============================================================
// 刁钻角度：UTF-8 编解码
// ============================================================

TEST_CASE("UTF8 edge: codePointLen") {
    SUBCASE("ASCII (1 byte)") {
        CHECK(utf8::codePointLen('A') == 1);
        CHECK(utf8::codePointLen('z') == 1);
        CHECK(utf8::codePointLen('0') == 1);
        CHECK(utf8::codePointLen(0x7F) == 1); // DEL
    }

    SUBCASE("2-byte sequence starter") {
        CHECK(utf8::codePointLen(0xC2) == 2);
        CHECK(utf8::codePointLen(0xDF) == 2);
    }

    SUBCASE("3-byte sequence starter") {
        CHECK(utf8::codePointLen(0xE0) == 3);
        CHECK(utf8::codePointLen(0xEF) == 3);
    }

    SUBCASE("4-byte sequence starter") {
        CHECK(utf8::codePointLen(0xF0) == 4);
        CHECK(utf8::codePointLen(0xF4) == 4);
    }

    SUBCASE("continuation bytes fall back to 1") {
        // Continuation bytes (10xxxxxx) are not valid start bytes
        CHECK(utf8::codePointLen(0x80) == 1);
        CHECK(utf8::codePointLen(0xBF) == 1);
    }

    SUBCASE("invalid byte values") {
        // 0xFE and 0xFF are never valid in UTF-8
        CHECK(utf8::codePointLen(0xFE) == 1);
        CHECK(utf8::codePointLen(0xFF) == 1);
    }
}

TEST_CASE("UTF8 edge: encode") {
    SUBCASE("ASCII range") {
        CHECK(utf8::encode('A') == "A");
        CHECK(utf8::encode(0x20) == " "); // space
        CHECK(utf8::encode(0x7E) == "~"); // tilde
    }

    SUBCASE("2-byte codepoint") {
        // U+00A9 = C2 A9 in UTF-8
        std::string s = utf8::encode(0xA9);
        CHECK(s.size() == 2);
        CHECK(static_cast<unsigned char>(s[0]) == 0xC2);
        CHECK(static_cast<unsigned char>(s[1]) == 0xA9);
    }

    SUBCASE("3-byte codepoint") {
        // U+4E2D (中) = E4 B8 AD
        std::string s = utf8::encode(0x4E2D);
        CHECK(s.size() == 3);
        CHECK(static_cast<unsigned char>(s[0]) == 0xE4);
        CHECK(static_cast<unsigned char>(s[1]) == 0xB8);
        CHECK(static_cast<unsigned char>(s[2]) == 0xAD);
    }

    SUBCASE("4-byte codepoint") {
        // U+1F600 (😀) = F0 9F 98 80
        std::string s = utf8::encode(0x1F600);
        CHECK(s.size() == 4);
        CHECK(static_cast<unsigned char>(s[0]) == 0xF0);
        CHECK(static_cast<unsigned char>(s[1]) == 0x9F);
        CHECK(static_cast<unsigned char>(s[2]) == 0x98);
        CHECK(static_cast<unsigned char>(s[3]) == 0x80);
    }

    SUBCASE("codepoint beyond max (0x110000) produces empty") {
        // encode() only handles cp < 0x110000, otherwise returns empty string
        std::string s = utf8::encode(0x110000);
        CHECK(s.empty());
    }

    SUBCASE("surrogate halves are rejected") {
        // encode() now explicitly rejects surrogates (D800-DFFF)
        std::string s = utf8::encode(0xD800);
        CHECK(s.empty());
        s = utf8::encode(0xDFFF);
        CHECK(s.empty());
    }
}

TEST_CASE("UTF8 edge: prevPos") {
    SUBCASE("at position 0 returns 0") {
        CHECK(utf8::prevPos("hello", 0) == 0);
    }

    SUBCASE("after ASCII char returns previous position") {
        CHECK(utf8::prevPos("abc", 2) == 1);
        CHECK(utf8::prevPos("abc", 1) == 0);
    }

    SUBCASE("within multi-byte char returns start of that char") {
        // "中" = E4 B8 AD, at position 2 (middle byte) → should go to 0
        std::string s = utf8::encode(0x4E2D); // "中"
        CHECK(utf8::prevPos(s, 2) == 0);
        CHECK(utf8::prevPos(s, 1) == 0);
    }

    SUBCASE("mixed ASCII and CJK") {
        std::string s = "A" + utf8::encode(0x4E2D) + "B"; // "A中B"
        // Positions: 0='A'(1B), 1-3='中'(3B), 4='B'(1B)
        // prevPos at pos 4 (on 'B') should go to...
        //   pos 3 is continuation byte of '中', so skip back
        //   pos 2 is continuation byte, skip back
        //   pos 1 is start of '中', skip back
        //   pos 0 is 'A'... no, prevPos starts at pos-1=3
        // Actually: prevPos(s, 4): p=3. s[3]=0xAD is continuation → p=2. s[2]=0xB8 is continuation → p=1. s[1]=0xB8... wait no.
        // s[0]='A', s[1]=0xE4, s[2]=0xB8, s[3]=0xAD, s[4]='B'
        // prevPos(s, 4): start p=3. s[3]=0xAD (10xxxxxx) → p=2. s[2]=0xB8 (10xxxxxx) → p=1. s[1]=0xE4 (1110xxxx) → stop. return 1.
        CHECK(utf8::prevPos(s, 4) == 1);
    }
}

TEST_CASE("UTF8 edge: nextPos") {
    SUBCASE("at end returns length") {
        CHECK(utf8::nextPos("abc", 3) == 3);
    }

    SUBCASE("past end returns length") {
        CHECK(utf8::nextPos("abc", 5) == 3);
    }

    SUBCASE("advances past ASCII") {
        CHECK(utf8::nextPos("abc", 0) == 1);
        CHECK(utf8::nextPos("abc", 1) == 2);
    }

    SUBCASE("advances past multi-byte") {
        std::string s = utf8::encode(0x4E2D); // 3 bytes
        CHECK(utf8::nextPos(s, 0) == 3); // skips all 3 bytes
    }

    SUBCASE("negative position returns len") {
        // nextPos guards pos < 0 → returns len (position at string end)
        CHECK(utf8::nextPos("abc", -1) == 3);
    }
}

TEST_CASE("UTF8 edge: encode roundtrip for every byte width") {
    auto roundtrip = [](unsigned int cp) {
        std::string s = utf8::encode(cp);
        if (s.empty()) return;
        // Verify first byte correctly identifies byte count
        int len = utf8::codePointLen(static_cast<unsigned char>(s[0]));
        CHECK(static_cast<size_t>(len) == s.size());
    };

    roundtrip(0x41);     // 1 byte
    roundtrip(0xA9);     // 2 bytes
    roundtrip(0x4E2D);   // 3 bytes
    roundtrip(0x1F600);  // 4 bytes
}
