#pragma once

#include "UIElement.h"
#include "UIButton.h"
#include "Settings/ISettingsTab.h"
#include "Settings/GraphicsSettingsTab.h"
#include "Settings/DisplaySettingsTab.h"
#include "../Core/Types.h"
#include <dwrite.h>
#include <memory>
#include <vector>
#include <functional>

class D2DInterop;

enum class SettingsAction
{
    None,
    Back
};

// Combined callbacks - forwards to individual tabs
struct SettingsCallbacks
{
    GraphicsSettingsCallbacks graphics;
    DisplaySettingsCallbacks display;
};

// Combined values - aggregates from individual tabs
struct SettingsValues
{
    GraphicsSettingsValues graphics;
    DisplaySettingsValues display;
};

class SettingsMenu
{
public:
    SettingsMenu();
    ~SettingsMenu();

    bool Initialize(D2DInterop* d2dInterop, uint32_t screenWidth, uint32_t screenHeight);
    void Shutdown();

    void HandleInput(float mouseX, float mouseY, bool mouseDown, bool mouseClicked);
    void Update(float deltaTime);
    void Render(D2DInterop* d2dInterop);

    void OnResize(uint32_t width, uint32_t height);

    // Actions
    SettingsAction GetLastAction() const { return m_lastAction; }
    void ClearAction() { m_lastAction = SettingsAction::None; }

    // Settings
    void SetCallbacks(const SettingsCallbacks& callbacks);
    void SetValues(const SettingsValues& values);
    SettingsValues GetValues() const;

private:
    void CreateUI();
    void LayoutUI();
    void SwitchTab(size_t tabIndex);

    uint32_t m_screenWidth = 0;
    uint32_t m_screenHeight = 0;
    SettingsAction m_lastAction = SettingsAction::None;
    size_t m_currentTabIndex = 0;

    // Text formats
    ComPtr<IDWriteTextFormat> m_titleFormat;
    ComPtr<IDWriteTextFormat> m_tabFormat;
    ComPtr<IDWriteTextFormat> m_labelFormat;

    // Back button
    std::unique_ptr<UIButton> m_backButton;

    // Tab buttons
    std::vector<std::unique_ptr<UIButton>> m_tabButtons;

    // Settings tabs (submodules)
    std::unique_ptr<GraphicsSettingsTab> m_graphicsTab;
    std::unique_ptr<DisplaySettingsTab> m_displayTab;
    std::vector<ISettingsTab*> m_tabs;
};
