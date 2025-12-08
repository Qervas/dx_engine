#pragma once

#include "MainMenu.h"
#include "PauseMenu.h"
#include "SettingsMenu.h"
#include "QuickSettings.h"
#include "../Core/AppState.h"
#include <memory>
#include <functional>

class D2DInterop;
class Window;

// Callbacks from UI to application for settings changes
struct UISettingsCallbacks
{
    // Graphics
    std::function<void(bool)> onPostProcessChanged;
    std::function<void(bool)> onBloomChanged;
    std::function<void(float)> onBloomIntensity;
    std::function<void(float)> onBloomThreshold;
    std::function<void(int)> onToneMapping;
    std::function<void(float)> onExposure;
    std::function<void(float)> onGamma;
    std::function<void(bool)> onSSAOChanged;
    std::function<void(float)> onSSAORadius;
    std::function<void(float)> onSSAOIntensity;

    // Display
    std::function<void(bool)> onVSync;
    std::function<void(bool)> onWireframe;
    std::function<void(bool)> onDebugRendering;
};

// Current settings values for syncing UI state
struct UISettingsValues
{
    bool postProcessEnabled = true;
    bool bloomEnabled = true;
    float bloomIntensity = 0.5f;
    float bloomThreshold = 1.5f;
    int toneMappingMode = 2;
    float exposure = 1.0f;
    float gamma = 2.2f;
    bool ssaoEnabled = true;
    float ssaoRadius = 0.5f;
    float ssaoIntensity = 1.5f;
    bool vsyncEnabled = true;
    bool wireframeEnabled = false;
    bool debugRenderingEnabled = true;
};

class UIManager
{
public:
    UIManager();
    ~UIManager();

    bool Initialize(D2DInterop* d2dInterop, uint32_t screenWidth, uint32_t screenHeight);
    void Shutdown();

    // State-based update/render
    void UpdateMainMenu(float deltaTime, float mouseX, float mouseY, bool mouseClicked);
    void RenderMainMenu(D2DInterop* d2d);

    void UpdateSettings(float deltaTime, float mouseX, float mouseY, bool mouseDown, bool mouseClicked);
    void RenderSettings(D2DInterop* d2d);

    void UpdatePaused(float deltaTime, float mouseX, float mouseY, bool mouseClicked);
    void RenderPaused(D2DInterop* d2d);

    void UpdateQuickSettings(float deltaTime, float mouseX, float mouseY, bool mouseDown, bool mouseClicked);
    void RenderQuickSettings(D2DInterop* d2d);

    // Quick settings visibility
    bool IsQuickSettingsVisible() const;
    void ToggleQuickSettings();

    // State transition queries
    bool ShouldStartGame() const { return m_shouldStartGame; }
    bool ShouldOpenSettings() const { return m_shouldOpenSettings; }
    bool ShouldCloseSettings() const { return m_shouldCloseSettings; }
    bool ShouldResume() const { return m_shouldResume; }
    bool ShouldReturnToMenu() const { return m_shouldReturnToMenu; }
    bool ShouldExit() const { return m_shouldExit; }

    void ClearTransitionFlags();

    // Resize handling
    void OnResize(uint32_t width, uint32_t height);

    // Settings callbacks
    void SetSettingsCallbacks(const UISettingsCallbacks& callbacks);
    void SyncSettingsValues(const UISettingsValues& values);

private:
    void SetupMenuCallbacks();
    void SetupSettingsCallbacks();
    void SetupQuickSettingsCallbacks();

    std::unique_ptr<MainMenu> m_mainMenu;
    std::unique_ptr<PauseMenu> m_pauseMenu;
    std::unique_ptr<SettingsMenu> m_settingsMenu;
    std::unique_ptr<QuickSettings> m_quickSettings;

    UISettingsCallbacks m_callbacks;

    // State transition flags
    bool m_shouldStartGame = false;
    bool m_shouldOpenSettings = false;
    bool m_shouldCloseSettings = false;
    bool m_shouldResume = false;
    bool m_shouldReturnToMenu = false;
    bool m_shouldExit = false;
};
