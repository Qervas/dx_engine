#pragma once

#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>

// Simple configuration manager for persisting settings
// Supports basic JSON-like format: key = value pairs grouped by sections

struct GraphicsSettings
{
    bool postProcessEnabled = true;
    bool bloomEnabled = true;
    float bloomIntensity = 0.3f;
    float bloomThreshold = 1.5f;
    int toneMappingMode = 2;  // 0=None, 1=Reinhard, 2=ACES, 3=Uncharted2
    float exposure = 1.0f;
    float gamma = 2.2f;
    bool ssaoEnabled = true;
    float ssaoRadius = 0.5f;
    float ssaoIntensity = 1.5f;
};

struct DisplaySettings
{
    bool vsyncEnabled = true;
    bool fullscreen = false;
    int windowWidth = 1280;
    int windowHeight = 720;
};

struct DebugSettings
{
    bool wireframeEnabled = false;
    bool debugRenderingEnabled = true;
};

struct AppSettings
{
    GraphicsSettings graphics;
    DisplaySettings display;
    DebugSettings debug;
};

class Config
{
public:
    static Config& Get()
    {
        static Config instance;
        return instance;
    }

    bool Load(const std::string& filepath);
    bool Save(const std::string& filepath);

    AppSettings& GetSettings() { return m_settings; }
    const AppSettings& GetSettings() const { return m_settings; }

    // Convenience accessors
    GraphicsSettings& Graphics() { return m_settings.graphics; }
    DisplaySettings& Display() { return m_settings.display; }
    DebugSettings& Debug() { return m_settings.debug; }

private:
    Config() = default;
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    // Simple parsing helpers
    std::string Trim(const std::string& str);
    bool ParseBool(const std::string& value);
    int ParseInt(const std::string& value);
    float ParseFloat(const std::string& value);

    AppSettings m_settings;
};
