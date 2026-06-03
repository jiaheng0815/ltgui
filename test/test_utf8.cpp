#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "utf8.h"
#include <string>

using namespace ltgui::utf8;

TEST_CASE("codePointLen") {
    SUBCASE("ASCII 1-byte") {
        CHECK(codePointLen('A') == 1);
        CHECK(codePointLen(0x7F) == 1);
    }
    SUBCASE("2-byte lead") {
        CHECK(codePointLen(0xC2) == 2);
        CHECK(codePointLen(0xDF) == 2);
    }
    SUBCASE("3-byte lead") {
        CHECK(codePointLen(0xE0) == 3);
        CHECK(codePointLen(0xEF) == 3);
    }
    SUBCASE("4-byte lead") {
        CHECK(codePointLen(0xF0) == 4);
        CHECK(codePointLen(0xF7) == 4);
    }
    SUBCASE("continuation byte falls back to 1") {
        CHECK(codePointLen(0x80) == 1);
        CHECK(codePointLen(0xBF) == 1);
    }
}

TEST_CASE("encode") {
    SUBCASE("ASCII") {
        CHECK(encode('A') == "A");
        CHECK(encode(0x7F) == "\x7F");
    }
    SUBCASE("2-byte boundary") {
        CHECK(encode(0x80).size() == 2);
        CHECK(encode(0x7FF).size() == 2);
    }
    SUBCASE("3-byte") {
        CHECK(encode(0x800).size() == 3);
        CHECK(encode(0x4E2D).size() == 3);  // U+4E2D
        CHECK(encode(0xFFFF).size() == 3);
    }
    SUBCASE("4-byte") {
        CHECK(encode(0x10000).size() == 4);
        CHECK(encode(0x1F600).size() == 4);  // U+1F600
    }
    SUBCASE("empty for code point >= 0x110000") {
        CHECK(encode(0x110000).empty());
    }
}

TEST_CASE("prevPos") {
    SUBCASE("empty string") {
        CHECK(prevPos("", 0) == 0);
    }
    SUBCASE("at position 0") {
        CHECK(prevPos("hello", 0) == 0);
    }
    SUBCASE("single byte char") {
        std::string s = "abc";
        CHECK(prevPos(s, 2) == 1);
        CHECK(prevPos(s, 1) == 0);
    }
    SUBCASE("multi-byte char") {
        std::string s = "A" + encode(0x4E2D) + "B";  // A + 中 + B
        int pos2 = 1 + 3;  // after the 3-byte char
        CHECK(prevPos(s, pos2) == 1);
    }
    SUBCASE("continuation bytes") {
        std::string s = encode(0x1F600);  // 4-byte emoji
        CHECK(prevPos(s, 3) == 0);
        CHECK(prevPos(s, 2) == 0);
        CHECK(prevPos(s, 1) == 0);
    }
}

TEST_CASE("nextPos") {
    SUBCASE("empty string") {
        CHECK(nextPos("", 0) == 0);
    }
    SUBCASE("at end") {
        std::string s = "hi";
        CHECK(nextPos(s, 2) == 2);
    }
    SUBCASE("past end") {
        std::string s = "hi";
        CHECK(nextPos(s, 5) == 2);
    }
    SUBCASE("single byte") {
        CHECK(nextPos("abc", 1) == 2);
    }
    SUBCASE("multi-byte") {
        std::string s = encode(0x4E2D) + "X";  // 3-byte + 1-byte
        CHECK(nextPos(s, 0) == 3);
        CHECK(nextPos(s, 3) == 4);
    }
}

TEST_CASE("encode/decode round-trip") {
    auto roundtrip = [](unsigned int cp) {
        std::string enc = encode(cp);
        if (enc.empty()) return;
        // decode first code point by checking byte sequence
        unsigned char b0 = static_cast<unsigned char>(enc[0]);
        unsigned int decoded = 0;
        if ((b0 & 0x80) == 0) {
            decoded = b0;
        } else if ((b0 & 0xE0) == 0xC0) {
            decoded = (b0 & 0x1F) << 6;
            decoded |= (static_cast<unsigned char>(enc[1]) & 0x3F);
        } else if ((b0 & 0xF0) == 0xE0) {
            decoded = (b0 & 0x0F) << 12;
            decoded |= (static_cast<unsigned char>(enc[1]) & 0x3F) << 6;
            decoded |= (static_cast<unsigned char>(enc[2]) & 0x3F);
        } else if ((b0 & 0xF8) == 0xF0) {
            decoded = (b0 & 0x07) << 18;
            decoded |= (static_cast<unsigned char>(enc[1]) & 0x3F) << 12;
            decoded |= (static_cast<unsigned char>(enc[2]) & 0x3F) << 6;
            decoded |= (static_cast<unsigned char>(enc[3]) & 0x3F);
        }
        CHECK(decoded == cp);
    };

    SUBCASE("ASCII") { roundtrip('A'); roundtrip('z'); }
    SUBCASE("2-byte") { roundtrip(0xA9); roundtrip(0x3A9); }
    SUBCASE("3-byte") { roundtrip(0x4E2D); roundtrip(0x2603); }
    SUBCASE("4-byte") { roundtrip(0x1F600); roundtrip(0x1F98A); }
}
