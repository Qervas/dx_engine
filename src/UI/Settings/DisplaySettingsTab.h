#pragma once

#include "ISettingsTab.h"
#include "../UICheckbox.h"
#include "../UIDropdown.h"
#include <memory>
#include <functional>

// Callbacks for display settings
struct DisplaySettingsCallbacks
{
    std::function<void(int)> onDisplayModeChanged;
    std::function<void(int)> onResolutionChanged;
    std::function<void(bool)> onVSyncChanged;
    std::function<void(bool)> onWireframeChanged;
    std::function<void(bool)> onDebugRenderingChanged;
};

// Values for display settings
struct DisplaySettingsValues
{
    int displayMode = 0;  // 0=Windowed, 1=Fullscreen Borderless, 2=Exclusive Fullscreen
    int resolutionIndex = 0;
    bool vsyncEnabled = true;
    bool wireframeEnabled = false;
    bool debugRenderingEnabled = true;
};

class DisplaySettingsTab : public ISettingsTab
{
public:
    DisplaySettingsTab();
    ~DisplaySettingsTab() override;

    bool Initialize(IDWriteTextFormat* labelFormat) override;
    void Shutdown() override;

    void HandleInput(float mouseX, float mouseY, bool mouseDown, bool mouseClicked) override;
    void Update(float deltaTime) override;
    void Render(D2DInterop* d2dInterop) override;

    void Layout(float x, float y, float width, float rowHeight) override;

    const wchar_t* GetTabName() const override { return L"Display"; }
    std::vector<UIElement*>& GetWidgets() override { return m_widgets; }

    // Settings
    void SetCallbacks(const DisplaySettingsCallbacks& callbacks) { m_callbacks = callbacks; }
    void SetValues(const DisplaySettingsValues& values);
    DisplaySettingsValues GetValues() const;

private:
    void CreateWidgets(IDWriteTextFormat* labelFormat);

    std::vector<UIElement*> m_widgets;

    // Display mode & resolution
    std::unique_ptr<UIDropdown> m_displayModeDropdown;
    std::unique_ptr<UIDropdown> m_resolutionDropdown;

    // Other display options
    std::unique_ptr<UICheckbox> m_vsyncCheckbox;
    std::unique_ptr<UICheckbox> m_wireframeCheckbox;
    std::unique_ptr<UICheckbox> m_debugRenderingCheckbox;

    DisplaySettingsCallbacks m_callbacks;

    // Active dropdown
    UIDropdown* m_activeDropdown = nullptr;
};
