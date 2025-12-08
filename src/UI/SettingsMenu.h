#pragma once

#include "UIElement.h"
#include "UIButton.h"
#include "UICheckbox.h"
#include "UISlider.h"
#include "UIDropdown.h"
#include "UILabel.h"
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

// Settings callbacks struct - app provides these to connect UI to actual settings
struct SettingsCallbacks
{
    // Graphics
    std::function<void(bool)> onPostProcessChanged;
    std::function<void(bool)> onBloomChanged;
    std::function<void(float)> onBloomIntensityChanged;
    std::function<void(float)> onBloomThresholdChanged;
    std::function<void(int)> onToneMappingChanged;
    std::function<void(float)> onExposureChanged;
    std::function<void(float)> onGammaChanged;
    std::function<void(bool)> onSSAOChanged;
    std::function<void(float)> onSSAORadiusChanged;
    std::function<void(float)> onSSAOIntensityChanged;

    // Display
    std::function<void(bool)> onVSyncChanged;
    std::function<void(bool)> onWireframeChanged;
    std::function<void(bool)> onDebugRenderingChanged;
};

// Current settings values - app provides these to initialize UI state
struct SettingsValues
{
    // Graphics
    bool postProcessEnabled = true;
    bool bloomEnabled = true;
    float bloomIntensity = 0.5f;
    float bloomThreshold = 1.5f;
    int toneMappingMode = 2;  // ACES
    float exposure = 1.0f;
    float gamma = 2.2f;
    bool ssaoEnabled = true;
    float ssaoRadius = 0.5f;
    float ssaoIntensity = 1.5f;

    // Display
    bool vsyncEnabled = true;
    bool wireframeEnabled = false;
    bool debugRenderingEnabled = true;
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
    void SetCallbacks(const SettingsCallbacks& callbacks) { m_callbacks = callbacks; }
    void SetValues(const SettingsValues& values);
    SettingsValues GetValues() const;

private:
    void CreateUI();
    void LayoutUI();

    enum class Tab { Graphics, Display };
    void SwitchTab(Tab tab);

    uint32_t m_screenWidth = 0;
    uint32_t m_screenHeight = 0;
    SettingsAction m_lastAction = SettingsAction::None;
    Tab m_currentTab = Tab::Graphics;

    // Text formats
    ComPtr<IDWriteTextFormat> m_titleFormat;
    ComPtr<IDWriteTextFormat> m_tabFormat;
    ComPtr<IDWriteTextFormat> m_labelFormat;
    ComPtr<IDWriteTextFormat> m_valueFormat;

    // Back button
    std::unique_ptr<UIButton> m_backButton;

    // Tab buttons
    std::unique_ptr<UIButton> m_graphicsTabButton;
    std::unique_ptr<UIButton> m_displayTabButton;

    // Graphics settings
    std::unique_ptr<UICheckbox> m_postProcessCheckbox;
    std::unique_ptr<UICheckbox> m_bloomCheckbox;
    std::unique_ptr<UISlider> m_bloomIntensitySlider;
    std::unique_ptr<UISlider> m_bloomThresholdSlider;
    std::unique_ptr<UIDropdown> m_toneMappingDropdown;
    std::unique_ptr<UISlider> m_exposureSlider;
    std::unique_ptr<UISlider> m_gammaSlider;
    std::unique_ptr<UICheckbox> m_ssaoCheckbox;
    std::unique_ptr<UISlider> m_ssaoRadiusSlider;
    std::unique_ptr<UISlider> m_ssaoIntensitySlider;

    // Display settings
    std::unique_ptr<UICheckbox> m_vsyncCheckbox;
    std::unique_ptr<UICheckbox> m_wireframeCheckbox;
    std::unique_ptr<UICheckbox> m_debugRenderingCheckbox;

    // All widgets for iteration
    std::vector<UIElement*> m_graphicsWidgets;
    std::vector<UIElement*> m_displayWidgets;

    // Callbacks
    SettingsCallbacks m_callbacks;

    // Slider being dragged
    UISlider* m_activeSlider = nullptr;
    UIDropdown* m_activeDropdown = nullptr;
};
