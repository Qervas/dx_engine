#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <fstream>

// Graphics settings - post-processing, effects
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

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GraphicsSettings,
        postProcessEnabled, bloomEnabled, bloomIntensity, bloomThreshold,
        toneMappingMode, exposure, gamma, ssaoEnabled, ssaoRadius, ssaoIntensity)
};

// Display settings - window mode, resolution
struct DisplaySettings
{
    int displayMode = 0;      // 0=Windowed, 1=Fullscreen Borderless, 2=Fullscreen Exclusive
    int resolutionIndex = 3;  // Index into resolution list (default 1920x1080)
    bool vsyncEnabled = true;
    bool wireframeEnabled = false;
    bool debugRenderingEnabled = true;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(DisplaySettings,
        displayMode, resolutionIndex, vsyncEnabled, wireframeEnabled, debugRenderingEnabled)
};

// All application settings
struct AppSettings
{
    GraphicsSettings graphics;
    DisplaySettings display;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(AppSettings, graphics, display)
};

// Singleton config manager
class Config
{
public:
    static Config& Get()
    {
        static Config instance;
        return instance;
    }

    // File operations
    bool Load(const std::string& filepath);
    bool Save();
    bool Save(const std::string& filepath);

    // Convenience accessors
    GraphicsSettings& Graphics() { return m_settings.graphics; }
    DisplaySettings& Display() { return m_settings.display; }
    const GraphicsSettings& Graphics() const { return m_settings.graphics; }
    const DisplaySettings& Display() const { return m_settings.display; }

    // Dirty tracking
    void MarkDirty() { m_dirty = true; }
    bool IsDirty() const { return m_dirty; }
    void SaveIfDirty() { if (m_dirty) Save(); }

private:
    Config() = default;
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    AppSettings m_settings;
    std::string m_filepath;
    bool m_dirty = false;
};
