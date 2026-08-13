#pragma once
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>

namespace ltgui {

// Minimal logger — replaces scattered printf/fprintf calls.
// Category-based: each subsystem (GPU, Window, Platform) gets its own
// toggle. By default, everything prints in debug builds; in release
// builds only Error and Warn level messages appear.

enum class LogLevel {
  Debug = 0,
  Info = 1,
  Warn = 2,
  Error = 3,
};

class Logger {
public:
  static Logger &instance() {
    static Logger logger;
    return logger;
  }

  // Global override — when true, ALL categories at ALL levels print,
  // regardless of NDEBUG or per-category settings.
  void setGlobalDebug(bool enabled) { globalDebug_ = enabled; }
  bool globalDebug() const { return globalDebug_; }

  // Enable/disable a named category (e.g. "GPU", "Window", "D3D11", "GL").
  // Uses an unordered_map so there is no hard limit on the number of
  // categories.
  void setEnabled(const char *category, bool enabled) {
    categories_[category] = enabled;
  }

  bool isEnabled(const char *category) const {
    if (globalDebug_)
      return true;
    auto it = categories_.find(category);
    if (it != categories_.end())
      return it->second;
    // Unknown categories default to enabled in debug, off in release
#ifdef NDEBUG
    return false;
#else
    return true;
#endif
  }

  void log(LogLevel level, const char *category, const char *fmt, ...) {
    if (!globalDebug_) {
#ifdef NDEBUG
      // Release builds: only warn and error (unless globalDebug_ overrides)
      if (level < LogLevel::Warn)
        return;
#endif
    }
    if (!isEnabled(category))
      return;

    const char *prefix = "";
    switch (level) {
    case LogLevel::Debug:
      prefix = "DEBUG";
      break;
    case LogLevel::Info:
      prefix = "INFO ";
      break;
    case LogLevel::Warn:
      prefix = "WARN ";
      break;
    case LogLevel::Error:
      prefix = "ERROR";
      break;
    }

    FILE *out = (level >= LogLevel::Error) ? stderr : stdout;
    fprintf(out, "[%s][%s] ", category, prefix);

    va_list args;
    va_start(args, fmt);
    vfprintf(out, fmt, args);
    va_end(args);

    fprintf(out, "\n");
  }

private:
  Logger() = default;
  bool globalDebug_ = false;
  std::unordered_map<std::string, bool> categories_;
};

// Convenience macros — cross-compiler variadic support.
// MSVC, GCC, and Clang all support ##__VA_ARGS__ (the GNU extension)
// for suppressing the trailing comma when called with no variadic args.
#define LOG_DEBUG(cat, fmt, ...)                                               \
  do {                                                                         \
    ltgui::Logger::instance().log(ltgui::LogLevel::Debug, cat, fmt,            \
                                  ##__VA_ARGS__);                              \
  } while (0)
#define LOG_INFO(cat, fmt, ...)                                                \
  do {                                                                         \
    ltgui::Logger::instance().log(ltgui::LogLevel::Info, cat, fmt,             \
                                  ##__VA_ARGS__);                              \
  } while (0)
#define LOG_WARN(cat, fmt, ...)                                                \
  do {                                                                         \
    ltgui::Logger::instance().log(ltgui::LogLevel::Warn, cat, fmt,             \
                                  ##__VA_ARGS__);                              \
  } while (0)
#define LOG_ERROR(cat, fmt, ...)                                               \
  do {                                                                         \
    ltgui::Logger::instance().log(ltgui::LogLevel::Error, cat, fmt,            \
                                  ##__VA_ARGS__);                              \
  } while (0)

} // namespace ltgui
