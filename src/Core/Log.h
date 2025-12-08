#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <functional>
#include <chrono>
#include <cstdarg>

// Log levels
enum class LogLevel
{
    Trace,      // Very detailed debugging
    Debug,      // Debug information
    Info,       // General information
    Warning,    // Warnings (non-fatal issues)
    Error,      // Errors (recoverable)
    Fatal       // Fatal errors (unrecoverable)
};

// Single log entry
struct LogEntry
{
    LogLevel level;
    std::string message;
    std::string category;
    std::chrono::system_clock::time_point timestamp;
    std::string file;
    int line;
};

// Log callback type
using LogCallback = std::function<void(const LogEntry&)>;

class Log
{
public:
    static Log& Get();

    // Core logging functions
    void LogMessage(LogLevel level, const char* category, const char* file, int line, const char* fmt, ...);
    void LogMessageV(LogLevel level, const char* category, const char* file, int line, const char* fmt, va_list args);

    // Get all entries (for console panel)
    const std::vector<LogEntry>& GetEntries() const { return m_entries; }

    // Clear all entries
    void Clear();

    // Filter settings
    void SetMinLevel(LogLevel level) { m_minLevel = level; }
    LogLevel GetMinLevel() const { return m_minLevel; }

    // Subscribe to new log entries
    void AddCallback(LogCallback callback);

    // Max entries before auto-trim
    void SetMaxEntries(size_t max) { m_maxEntries = max; }
    size_t GetMaxEntries() const { return m_maxEntries; }

    // Utility
    static const char* LevelToString(LogLevel level);
    static const char* LevelToIcon(LogLevel level);

private:
    Log() = default;
    ~Log() = default;
    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;

    std::vector<LogEntry> m_entries;
    std::vector<LogCallback> m_callbacks;
    mutable std::mutex m_mutex;
    LogLevel m_minLevel = LogLevel::Trace;
    size_t m_maxEntries = 10000;
};

// Convenience macros with file/line info
#define LOG_TRACE(category, fmt, ...)    Log::Get().LogMessage(LogLevel::Trace, category, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(category, fmt, ...)    Log::Get().LogMessage(LogLevel::Debug, category, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(category, fmt, ...)     Log::Get().LogMessage(LogLevel::Info, category, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARNING(category, fmt, ...)  Log::Get().LogMessage(LogLevel::Warning, category, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(category, fmt, ...)    Log::Get().LogMessage(LogLevel::Error, category, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_FATAL(category, fmt, ...)    Log::Get().LogMessage(LogLevel::Fatal, category, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

// Category-specific shortcuts (define categories as needed)
#define LOG_RENDER_INFO(fmt, ...)    LOG_INFO("Renderer", fmt, ##__VA_ARGS__)
#define LOG_RENDER_ERROR(fmt, ...)   LOG_ERROR("Renderer", fmt, ##__VA_ARGS__)
#define LOG_RENDER_WARNING(fmt, ...) LOG_WARNING("Renderer", fmt, ##__VA_ARGS__)

#define LOG_ENGINE_INFO(fmt, ...)    LOG_INFO("Engine", fmt, ##__VA_ARGS__)
#define LOG_ENGINE_ERROR(fmt, ...)   LOG_ERROR("Engine", fmt, ##__VA_ARGS__)
#define LOG_ENGINE_WARNING(fmt, ...) LOG_WARNING("Engine", fmt, ##__VA_ARGS__)

#define LOG_EDITOR_INFO(fmt, ...)    LOG_INFO("Editor", fmt, ##__VA_ARGS__)
#define LOG_EDITOR_ERROR(fmt, ...)   LOG_ERROR("Editor", fmt, ##__VA_ARGS__)

#define LOG_ASSET_INFO(fmt, ...)     LOG_INFO("Asset", fmt, ##__VA_ARGS__)
#define LOG_ASSET_ERROR(fmt, ...)    LOG_ERROR("Asset", fmt, ##__VA_ARGS__)
#define LOG_ASSET_WARNING(fmt, ...)  LOG_WARNING("Asset", fmt, ##__VA_ARGS__)
