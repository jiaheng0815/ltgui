#pragma once
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <unordered_map>
#include <string>

namespace ltgui {

// Minimal logger — replaces scattered printf/fprintf calls.
// Category-based: each subsystem (GPU, Window, Platform) gets its own
// toggle. By default, everything prints in debug builds; in release
// builds only Error and Warn level messages appear.

enum class LogLevel {
    Debug = 0,
    Info  = 1,
    Warn  = 2,
    Error = 3,
};

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    // Enable/disable a named category (e.g. "GPU", "Window", "D3D11", "GL").
    // Uses an unordered_map so there is no hard limit on the number of categories.
    void setEnabled(const char* category, bool enabled) {
        categories_[category] = enabled;
    }

    bool isEnabled(const char* category) const {
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

    void log(LogLevel level, const char* category, const char* fmt, ...) {
#ifdef NDEBUG
        // Release builds: only warn and error
        if (level < LogLevel::Warn) return;
#endif
        if (!isEnabled(category)) return;

        const char* prefix = "";
        switch (level) {
        case LogLevel::Debug: prefix = "DEBUG"; break;
        case LogLevel::Info:  prefix = "INFO "; break;
        case LogLevel::Warn:  prefix = "WARN "; break;
        case LogLevel::Error: prefix = "ERROR"; break;
        }

        FILE* out = (level >= LogLevel::Error) ? stderr : stdout;
        fprintf(out, "[%s][%s] ", category, prefix);

        va_list args;
        va_start(args, fmt);
        vfprintf(out, fmt, args);
        va_end(args);

        fprintf(out, "\n");
    }

private:
    Logger() = default;
    std::unordered_map<std::string, bool> categories_;
};

// Convenience macros — cross-compiler variadic support
#ifdef _MSC_VER
#define LOG_DEBUG(cat, fmt, ...) \
    do { ltgui::Logger::instance().log(ltgui::LogLevel::Debug, cat, fmt, __VA_ARGS__); } while(0)
#define LOG_INFO(cat, fmt, ...) \
    do { ltgui::Logger::instance().log(ltgui::LogLevel::Info,  cat, fmt, __VA_ARGS__); } while(0)
#define LOG_WARN(cat, fmt, ...) \
    do { ltgui::Logger::instance().log(ltgui::LogLevel::Warn,  cat, fmt, __VA_ARGS__); } while(0)
#define LOG_ERROR(cat, fmt, ...) \
    do { ltgui::Logger::instance().log(ltgui::LogLevel::Error, cat, fmt, __VA_ARGS__); } while(0)
#else
#define LOG_DEBUG(cat, fmt, ...) \
    do { ltgui::Logger::instance().log(ltgui::LogLevel::Debug, cat, fmt, ##__VA_ARGS__); } while(0)
#define LOG_INFO(cat, fmt, ...) \
    do { ltgui::Logger::instance().log(ltgui::LogLevel::Info,  cat, fmt, ##__VA_ARGS__); } while(0)
#define LOG_WARN(cat, fmt, ...) \
    do { ltgui::Logger::instance().log(ltgui::LogLevel::Warn,  cat, fmt, ##__VA_ARGS__); } while(0)
#define LOG_ERROR(cat, fmt, ...) \
    do { ltgui::Logger::instance().log(ltgui::LogLevel::Error, cat, fmt, ##__VA_ARGS__); } while(0)
#endif

} // namespace ltgui
