#pragma once
#include <cstdio>
#include <cstdarg>
#include <cstring>

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

    // Enable/disable a named category (e.g. "GPU", "Window", "D3D11", "GL")
    void setEnabled(const char* category, bool enabled) {
        for (int i = 0; i < categoryCount_; i++) {
            if (std::strcmp(categories_[i].name, category) == 0) {
                categories_[i].enabled = enabled;
                return;
            }
        }
        // Add new category if there's room
        if (categoryCount_ < 16) {
            categories_[categoryCount_++] = {category, enabled};
        }
    }

    bool isEnabled(const char* category) const {
        for (int i = 0; i < categoryCount_; i++) {
            if (std::strcmp(categories_[i].name, category) == 0)
                return categories_[i].enabled;
        }
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

    struct Category {
        const char* name;
        bool enabled = true;
    };
    Category categories_[16];
    int categoryCount_ = 0;
};

// Convenience macros — shorter than Logger::instance().log(...)
#define LOG_DEBUG(cat, fmt, ...) \
    do { ltgui::Logger::instance().log(ltgui::LogLevel::Debug, cat, fmt, ##__VA_ARGS__); } while(0)
#define LOG_INFO(cat, fmt, ...) \
    do { ltgui::Logger::instance().log(ltgui::LogLevel::Info,  cat, fmt, ##__VA_ARGS__); } while(0)
#define LOG_WARN(cat, fmt, ...) \
    do { ltgui::Logger::instance().log(ltgui::LogLevel::Warn,  cat, fmt, ##__VA_ARGS__); } while(0)
#define LOG_ERROR(cat, fmt, ...) \
    do { ltgui::Logger::instance().log(ltgui::LogLevel::Error, cat, fmt, ##__VA_ARGS__); } while(0)

} // namespace ltgui
