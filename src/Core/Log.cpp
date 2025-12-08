#include "Log.h"
#include <cstdio>
#include <algorithm>

#ifdef _WIN32
#include <Windows.h>
#endif

Log& Log::Get()
{
    static Log instance;
    return instance;
}

void Log::LogMessage(LogLevel level, const char* category, const char* file, int line, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogMessageV(level, category, file, line, fmt, args);
    va_end(args);
}

void Log::LogMessageV(LogLevel level, const char* category, const char* file, int line, const char* fmt, va_list args)
{
    // Check minimum level
    if (level < m_minLevel)
        return;

    // Format the message
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    // Create entry
    LogEntry entry;
    entry.level = level;
    entry.message = buffer;
    entry.category = category ? category : "General";
    entry.timestamp = std::chrono::system_clock::now();
    entry.file = file ? file : "";
    entry.line = line;

    // Extract just filename from path
    if (!entry.file.empty())
    {
        size_t pos = entry.file.find_last_of("\\/");
        if (pos != std::string::npos)
        {
            entry.file = entry.file.substr(pos + 1);
        }
    }

    // Thread-safe access
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Add entry
        m_entries.push_back(entry);

        // Trim if over max
        if (m_entries.size() > m_maxEntries)
        {
            m_entries.erase(m_entries.begin(), m_entries.begin() + (m_entries.size() - m_maxEntries));
        }

        // Notify callbacks
        for (auto& callback : m_callbacks)
        {
            callback(entry);
        }
    }

    // Also output to debug console (Visual Studio Output window)
#ifdef _WIN32
    char debugBuffer[4096];
    snprintf(debugBuffer, sizeof(debugBuffer), "[%s][%s] %s\n",
        LevelToString(level), entry.category.c_str(), buffer);
    OutputDebugStringA(debugBuffer);
#endif

    // Also print to stdout for console apps
    const char* colorCode = "";
    const char* resetCode = "";

#ifndef _WIN32
    // ANSI colors for non-Windows
    switch (level)
    {
    case LogLevel::Trace:   colorCode = "\033[90m"; break;  // Gray
    case LogLevel::Debug:   colorCode = "\033[36m"; break;  // Cyan
    case LogLevel::Info:    colorCode = "\033[32m"; break;  // Green
    case LogLevel::Warning: colorCode = "\033[33m"; break;  // Yellow
    case LogLevel::Error:   colorCode = "\033[31m"; break;  // Red
    case LogLevel::Fatal:   colorCode = "\033[35m"; break;  // Magenta
    }
    resetCode = "\033[0m";
#endif

    fprintf(level >= LogLevel::Error ? stderr : stdout,
        "%s[%s][%s] %s%s\n",
        colorCode, LevelToString(level), entry.category.c_str(), buffer, resetCode);
}

void Log::Clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.clear();
}

void Log::AddCallback(LogCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callbacks.push_back(callback);
}

const char* Log::LevelToString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Trace:   return "TRACE";
    case LogLevel::Debug:   return "DEBUG";
    case LogLevel::Info:    return "INFO";
    case LogLevel::Warning: return "WARN";
    case LogLevel::Error:   return "ERROR";
    case LogLevel::Fatal:   return "FATAL";
    default:                return "UNKNOWN";
    }
}

const char* Log::LevelToIcon(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Trace:   return " ";
    case LogLevel::Debug:   return " ";
    case LogLevel::Info:    return " ";
    case LogLevel::Warning: return "!";
    case LogLevel::Error:   return "X";
    case LogLevel::Fatal:   return "X";
    default:                return "?";
    }
}
