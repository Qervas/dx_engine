#pragma once

#include "UIElement.h"
#include "UICheckbox.h"
#include "UISlider.h"
#include "../Core/Types.h"
#include <dwrite.h>
#include <memory>
#include <vector>
#include <functional>

class D2DInterop;

// Simplified callbacks for quick settings (subset of full settings)
struct QuickSettingsCallbacks
{
    std::function<void(bool)> onBloomChanged;
    std::function<void(bool)> onSSAOChanged;
    std::function<void(bool)> onPostProcessChanged;
    std::function<void(bool)> onWireframeChanged;
    std::function<void(float)> onExposureChanged;
};

struct QuickSettingsValues
{
    bool bloomEnabled = true;
    bool ssaoEnabled = true;
    bool postProcessEnabled = true;
    bool wireframeEnabled = false;
    float exposure = 1.0f;
};

class QuickSettings
{
public:
    QuickSettings();
    ~QuickSettings();

    bool Initialize(D2DInterop* d2dInterop, uint32_t screenWidth, uint32_t screenHeight);
    void Shutdown();

    void HandleInput(float mouseX, float mouseY, bool mouseDown, bool mouseClicked);
    void Update(float deltaTime);
    void Render(D2DInterop* d2dInterop);

    void OnResize(uint32_t width, uint32_t height);

    // Visibility
    bool IsVisible() const { return m_visible; }
    void SetVisible(bool visible) { m_visible = visible; }
    void Toggle() { m_visible = !m_visible; }

    // Settings
    void SetCallbacks(const QuickSettingsCallbacks& callbacks) { m_callbacks = callbacks; }
    void SetValues(const QuickSettingsValues& values);
    QuickSettingsValues GetValues() const;

private:
    void CreateUI();
    void LayoutUI();

    uint32_t m_screenWidth = 0;
    uint32_t m_screenHeight = 0;
    bool m_visible = false;

    // Text formats
    ComPtr<IDWriteTextFormat> m_titleFormat;
    ComPtr<IDWriteTextFormat> m_labelFormat;

    // Widgets
    std::unique_ptr<UICheckbox> m_bloomCheckbox;
    std::unique_ptr<UICheckbox> m_ssaoCheckbox;
    std::unique_ptr<UICheckbox> m_postProcessCheckbox;
    std::unique_ptr<UICheckbox> m_wireframeCheckbox;
    std::unique_ptr<UISlider> m_exposureSlider;

    std::vector<UIElement*> m_widgets;

    QuickSettingsCallbacks m_callbacks;

    // Slider being dragged
    UISlider* m_activeSlider = nullptr;
};
