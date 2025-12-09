#pragma once

#include "ISettingsTab.h"
#include "../UICheckbox.h"
#include "../UISlider.h"
#include "../UIDropdown.h"
#include <memory>
#include <functional>

// Callbacks for graphics settings
struct GraphicsSettingsCallbacks
{
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
};

// Values for graphics settings
struct GraphicsSettingsValues
{
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
};

class GraphicsSettingsTab : public ISettingsTab
{
public:
    GraphicsSettingsTab();
    ~GraphicsSettingsTab() override;

    bool Initialize(IDWriteTextFormat* labelFormat) override;
    void Shutdown() override;

    void HandleInput(float mouseX, float mouseY, bool mouseDown, bool mouseClicked) override;
    void Update(float deltaTime) override;
    void Render(D2DInterop* d2dInterop) override;

    void Layout(float x, float y, float width, float rowHeight) override;

    const wchar_t* GetTabName() const override { return L"Graphics"; }
    std::vector<UIElement*>& GetWidgets() override { return m_widgets; }

    // Settings
    void SetCallbacks(const GraphicsSettingsCallbacks& callbacks) { m_callbacks = callbacks; }
    void SetValues(const GraphicsSettingsValues& values);
    GraphicsSettingsValues GetValues() const;

private:
    void CreateWidgets(IDWriteTextFormat* labelFormat);

    std::vector<UIElement*> m_widgets;

    // Post-processing
    std::unique_ptr<UICheckbox> m_postProcessCheckbox;
    std::unique_ptr<UICheckbox> m_bloomCheckbox;
    std::unique_ptr<UISlider> m_bloomIntensitySlider;
    std::unique_ptr<UISlider> m_bloomThresholdSlider;
    std::unique_ptr<UIDropdown> m_toneMappingDropdown;
    std::unique_ptr<UISlider> m_exposureSlider;
    std::unique_ptr<UISlider> m_gammaSlider;

    // SSAO
    std::unique_ptr<UICheckbox> m_ssaoCheckbox;
    std::unique_ptr<UISlider> m_ssaoRadiusSlider;
    std::unique_ptr<UISlider> m_ssaoIntensitySlider;

    GraphicsSettingsCallbacks m_callbacks;

    // Active controls
    UISlider* m_activeSlider = nullptr;
    UIDropdown* m_activeDropdown = nullptr;
};
