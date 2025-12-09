#pragma once

#include "MainMenu.h"
#include "PauseMenu.h"
#include "SettingsMenu.h"
#include "../Core/AppState.h"
#include <memory>
#include <functional>

class D2DInterop;
class Window;

// Re-export settings types from SettingsMenu for Application use
using UISettingsCallbacks = SettingsCallbacks;
using UISettingsValues = SettingsValues;

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

    std::unique_ptr<MainMenu> m_mainMenu;
    std::unique_ptr<PauseMenu> m_pauseMenu;
    std::unique_ptr<SettingsMenu> m_settingsMenu;

    // State transition flags
    bool m_shouldStartGame = false;
    bool m_shouldOpenSettings = false;
    bool m_shouldCloseSettings = false;
    bool m_shouldResume = false;
    bool m_shouldReturnToMenu = false;
    bool m_shouldExit = false;
};
