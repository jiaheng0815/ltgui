#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "i18n.h"
#include "signal.h"
#include <string>

using namespace ltgui;

TEST_CASE("Locale parsing") {
    SUBCASE("simple language") {
        Locale loc = I18n::parse("zh");
        CHECK(loc.language == "zh");
        CHECK(loc.country.empty());
        CHECK(loc.variant.empty());
    }
    SUBCASE("language-country") {
        Locale loc = I18n::parse("zh-CN");
        CHECK(loc.language == "zh");
        CHECK(loc.country == "CN");
        CHECK(loc.variant.empty());
    }
    SUBCASE("language-country-variant") {
        Locale loc = I18n::parse("zh-Hant-TW");
        CHECK(loc.language == "zh");
        CHECK(loc.country == "Hant");
        CHECK(loc.variant == "TW");
    }
    SUBCASE("toString") {
        Locale loc{"en", "US", ""};
        CHECK(loc.toString() == "en-US");
    }
}

TEST_CASE("Plural rules") {
    SUBCASE("Chinese — always Other") {
        Locale zh{"zh", "CN", ""};
        CHECK(PluralRules::formIndex(zh, 0) == 5);
        CHECK(PluralRules::formIndex(zh, 1) == 5);
        CHECK(PluralRules::formIndex(zh, 100) == 5);
    }
    SUBCASE("English — One for 1, Other for rest") {
        Locale en{"en", "US", ""};
        CHECK(PluralRules::formIndex(en, 0) == 5);
        CHECK(PluralRules::formIndex(en, 1) == 1);
        CHECK(PluralRules::formIndex(en, 2) == 5);
        CHECK(PluralRules::formIndex(en, 100) == 5);
    }
    SUBCASE("Russian — One/Few/Other") {
        Locale ru{"ru", "RU", ""};
        CHECK(PluralRules::formIndex(ru, 1) == 1);   // 1 = One
        CHECK(PluralRules::formIndex(ru, 21) == 1);  // 21 = One
        CHECK(PluralRules::formIndex(ru, 2) == 3);   // 2 = Few
        CHECK(PluralRules::formIndex(ru, 5) == 5);   // 5 = Other
        CHECK(PluralRules::formIndex(ru, 11) == 5);  // 11 = Other
    }
    SUBCASE("Arabic — all 6 forms") {
        Locale ar{"ar", "SA", ""};
        CHECK(PluralRules::formIndex(ar, 0) == 0);   // Zero
        CHECK(PluralRules::formIndex(ar, 1) == 1);   // One
        CHECK(PluralRules::formIndex(ar, 2) == 2);   // Two
        CHECK(PluralRules::formIndex(ar, 5) == 3);   // Few
        CHECK(PluralRules::formIndex(ar, 15) == 4);  // Many
        CHECK(PluralRules::formIndex(ar, 100) == 5); // Other
    }
}

TEST_CASE("TranslationTable") {
    SUBCASE("simple key lookup") {
        TranslationTable t;
        t.add("hello", "你好");
        CHECK(t.get("hello") == "你好");
    }
    SUBCASE("missing key returns key itself") {
        TranslationTable t;
        CHECK(t.get("nonexistent") == "nonexistent");
    }
    SUBCASE("plurals") {
        TranslationTable t;
        t.addPlural("files", "", "1 file", "", "", "", "other files");

        // English: only 1 → One, everything else → Other
        I18n::instance().setLocale(Locale{"en", "US", ""});
        CHECK(t.getPlural("files", 0) == "other files");
        CHECK(t.getPlural("files", 1) == "1 file");
        CHECK(t.getPlural("files", 2) == "other files");
        CHECK(t.getPlural("files", 10) == "other files");
    }
    SUBCASE("plurals ru") {
        TranslationTable t;
        t.addPlural("books", "", "книга", "", "книги", "", "книг");

        // Russian: 1,21→One; 2-4,22-24→Few; others→Other
        I18n::instance().setLocale(Locale{"ru", "RU", ""});
        CHECK(t.getPlural("books", 1) == "книга");
        CHECK(t.getPlural("books", 2) == "книги");
        CHECK(t.getPlural("books", 5) == "книг");
        CHECK(t.getPlural("books", 21) == "книга");
        I18n::instance().setLocale(Locale{"en", "US", ""});
    }
    SUBCASE("JSON load flat") {
        const char* json = R"({"ok":"确定","cancel":"取消","save":"保存"})";
        TranslationTable t;
        CHECK(t.loadFromJsonString(json));
        CHECK(t.get("ok") == "确定");
        CHECK(t.get("cancel") == "取消");
        CHECK(t.get("save") == "保存");
    }
    SUBCASE("JSON with plurals") {
        const char* json = R"({"hello":"world","files":["zero","one","two","few","many","other"]})";
        TranslationTable t;
        CHECK(t.loadFromJsonString(json));
        CHECK(t.get("hello") == "world");
        // Array values are parsed as plurals — verify the full set is stored
        CHECK(!t.getPlural("files", 0).empty());
        CHECK(!t.getPlural("files", 1).empty());
    }
}

TEST_CASE("I18n") {
    SUBCASE("fallback to key when no table") {
        I18n& i18n = I18n::instance();
        CHECK(i18n.tr("hello") == "hello");
    }
    SUBCASE("translate with table") {
        I18n& i18n = I18n::instance();
        Locale zh{"zh", "CN", ""};
        TranslationTable t;
        t.add("hello", "你好");
        i18n.addTable(zh, std::move(t));
        i18n.setLocale(zh);
        CHECK(i18n.tr("hello") == "你好");
        i18n.removeTable(zh);
        // Reset locale
        i18n.setLocale(Locale{"en", "US", ""});
    }
    SUBCASE("locale change signal") {
        I18n& i18n = I18n::instance();
        bool fired = false;
        Locale received;
        int id = i18n.onLocaleChanged.connect([&](const Locale& loc) {
            fired = true;
            received = loc;
        });

        Locale fr{"fr", "FR", ""};
        i18n.setLocale(fr);
        CHECK(fired);
        CHECK(received.language == "fr");

        i18n.onLocaleChanged.disconnect(id);
        i18n.setLocale(Locale{"en", "US", ""});
    }
    SUBCASE("plural translation") {
        I18n& i18n = I18n::instance();
        Locale en{"en", "US", ""};
        TranslationTable t;
        t.addPlural("item", "", "one item", "", "", "", "{} items");
        i18n.addTable(en, std::move(t));

        Locale prev = i18n.locale();
        i18n.setLocale(en);
        CHECK(i18n.tr("item", 1) == "one item");
        CHECK(i18n.tr("item", 5) == "{} items");

        i18n.removeTable(en);
        i18n.setLocale(prev);
    }
}
