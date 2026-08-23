#pragma once
#include "signal.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ltgui {

struct Locale {
  std::string language;
  std::string country;
  std::string variant;

  std::string toString() const;
  bool operator==(const Locale &o) const;
  bool operator!=(const Locale &o) const { return !(*this == o); }
};

class PluralRules {
public:
  enum Form {
    Zero = 0,
    One = 1,
    Two = 2,
    Few = 3,
    Many = 4,
    Other = 5,
    MaxForms = 6
  };

  static int formIndex(const Locale &locale, int64_t n);
};

class TranslationTable {
public:
  void add(const std::string &key, const std::string &value);
  void addPlural(const std::string &key, const std::string &zero,
                 const std::string &one, const std::string &two,
                 const std::string &few, const std::string &many,
                 const std::string &other);
  std::string get(const std::string &key) const;
  std::string getPlural(const std::string &key, int64_t n) const;

  bool loadFromJsonFile(const std::string &path);
  bool loadFromJsonString(const std::string &json);

  size_t entryCount() const { return entries_.size(); }

private:
  std::unordered_map<std::string, std::string> entries_;
  std::unordered_map<std::string, std::string> plurals_[6];
};

class I18n {
public:
  static I18n &instance();

  void setLocale(const Locale &locale);
  Locale locale() const { return locale_; }

  void addTable(const Locale &locale, TranslationTable table);
  void removeTable(const Locale &locale);

  std::string tr(const std::string &key) const;
  std::string tr(const std::string &key, int64_t n) const;

  bool loadTableFromFile(const Locale &locale, const std::string &jsonPath);

  Signal<const Locale &> onLocaleChanged;

  static Locale parse(const std::string &str);

  I18n(const I18n &) = delete;
  I18n &operator=(const I18n &) = delete;

private:
  I18n() = default;
  Locale locale_;
  std::unordered_map<std::string, TranslationTable> tables_;
};

} // namespace ltgui
