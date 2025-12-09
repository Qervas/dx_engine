#include "GraphicsSettingsTab.h"
#include "../UICheckbox.h"
#include "../UISlider.h"
#include "../UIDropdown.h"
#include "../../RHI/D2DInterop.h"

GraphicsSettingsTab::GraphicsSettingsTab()
{
}

GraphicsSettingsTab::~GraphicsSettingsTab()
{
    Shutdown();
}

bool GraphicsSettingsTab::Initialize(IDWriteTextFormat* labelFormat)
{
    CreateWidgets(labelFormat);
    return true;
}

void GraphicsSettingsTab::Shutdown()
{
    m_widgets.clear();
    m_postProcessCheckbox.reset();
    m_bloomCheckbox.reset();
    m_bloomIntensitySlider.reset();
    m_bloomThresholdSlider.reset();
    m_toneMappingDropdown.reset();
    m_exposureSlider.reset();
    m_gammaSlider.reset();
    m_ssaoCheckbox.reset();
    m_ssaoRadiusSlider.reset();
    m_ssaoIntensitySlider.reset();
}

void GraphicsSettingsTab::CreateWidgets(IDWriteTextFormat* labelFormat)
{
    // Post-processing toggle
    m_postProcessCheckbox = std::make_unique<UICheckbox>(L"Post Processing");
    m_postProcessCheckbox->SetTextFormat(labelFormat);
    m_postProcessCheckbox->SetChecked(true);
    m_postProcessCheckbox->SetOnChange([this](bool v) {
        if (m_callbacks.onPostProcessChanged) m_callbacks.onPostProcessChanged(v);
    });

    // Bloom
    m_bloomCheckbox = std::make_unique<UICheckbox>(L"Bloom");
    m_bloomCheckbox->SetTextFormat(labelFormat);
    m_bloomCheckbox->SetChecked(true);
    m_bloomCheckbox->SetOnChange([this](bool v) {
        if (m_callbacks.onBloomChanged) m_callbacks.onBloomChanged(v);
    });

    m_bloomIntensitySlider = std::make_unique<UISlider>(L"Bloom Intensity");
    m_bloomIntensitySlider->SetTextFormat(labelFormat);
    m_bloomIntensitySlider->SetRange(0.0f, 2.0f);
    m_bloomIntensitySlider->SetValue(0.5f);
    m_bloomIntensitySlider->SetOnChange([this](float v) {
        if (m_callbacks.onBloomIntensityChanged) m_callbacks.onBloomIntensityChanged(v);
    });

    m_bloomThresholdSlider = std::make_unique<UISlider>(L"Bloom Threshold");
    m_bloomThresholdSlider->SetTextFormat(labelFormat);
    m_bloomThresholdSlider->SetRange(0.1f, 5.0f);
    m_bloomThresholdSlider->SetValue(1.5f);
    m_bloomThresholdSlider->SetOnChange([this](float v) {
        if (m_callbacks.onBloomThresholdChanged) m_callbacks.onBloomThresholdChanged(v);
    });

    // Tone mapping
    m_toneMappingDropdown = std::make_unique<UIDropdown>(L"Tone Mapping");
    m_toneMappingDropdown->SetTextFormat(labelFormat);
    m_toneMappingDropdown->SetOptions({ L"None", L"Reinhard", L"ACES", L"Uncharted2" });
    m_toneMappingDropdown->SetSelectedIndex(2);  // ACES
    m_toneMappingDropdown->SetOnChange([this](int v) {
        if (m_callbacks.onToneMappingChanged) m_callbacks.onToneMappingChanged(v);
    });

    // Exposure & Gamma
    m_exposureSlider = std::make_unique<UISlider>(L"Exposure");
    m_exposureSlider->SetTextFormat(labelFormat);
    m_exposureSlider->SetRange(0.1f, 5.0f);
    m_exposureSlider->SetValue(1.0f);
    m_exposureSlider->SetOnChange([this](float v) {
        if (m_callbacks.onExposureChanged) m_callbacks.onExposureChanged(v);
    });

    m_gammaSlider = std::make_unique<UISlider>(L"Gamma");
    m_gammaSlider->SetTextFormat(labelFormat);
    m_gammaSlider->SetRange(1.0f, 3.0f);
    m_gammaSlider->SetValue(2.2f);
    m_gammaSlider->SetOnChange([this](float v) {
        if (m_callbacks.onGammaChanged) m_callbacks.onGammaChanged(v);
    });

    // SSAO
    m_ssaoCheckbox = std::make_unique<UICheckbox>(L"SSAO");
    m_ssaoCheckbox->SetTextFormat(labelFormat);
    m_ssaoCheckbox->SetChecked(true);
    m_ssaoCheckbox->SetOnChange([this](bool v) {
        if (m_callbacks.onSSAOChanged) m_callbacks.onSSAOChanged(v);
    });

    m_ssaoRadiusSlider = std::make_unique<UISlider>(L"SSAO Radius");
    m_ssaoRadiusSlider->SetTextFormat(labelFormat);
    m_ssaoRadiusSlider->SetRange(0.1f, 2.0f);
    m_ssaoRadiusSlider->SetValue(0.5f);
    m_ssaoRadiusSlider->SetOnChange([this](float v) {
        if (m_callbacks.onSSAORadiusChanged) m_callbacks.onSSAORadiusChanged(v);
    });

    m_ssaoIntensitySlider = std::make_unique<UISlider>(L"SSAO Intensity");
    m_ssaoIntensitySlider->SetTextFormat(labelFormat);
    m_ssaoIntensitySlider->SetRange(0.5f, 3.0f);
    m_ssaoIntensitySlider->SetValue(1.5f);
    m_ssaoIntensitySlider->SetOnChange([this](float v) {
        if (m_callbacks.onSSAOIntensityChanged) m_callbacks.onSSAOIntensityChanged(v);
    });

    // Build widget list
    m_widgets = {
        m_postProcessCheckbox.get(),
        m_bloomCheckbox.get(),
        m_bloomIntensitySlider.get(),
        m_bloomThresholdSlider.get(),
        m_toneMappingDropdown.get(),
        m_exposureSlider.get(),
        m_gammaSlider.get(),
        m_ssaoCheckbox.get(),
        m_ssaoRadiusSlider.get(),
        m_ssaoIntensitySlider.get()
    };
}

void GraphicsSettingsTab::Layout(float x, float y, float width, float rowHeight)
{
    for (size_t i = 0; i < m_widgets.size(); ++i)
    {
        m_widgets[i]->SetPosition(x, y + static_cast<float>(i) * rowHeight);
        m_widgets[i]->SetSize(width, rowHeight);
    }
}

void GraphicsSettingsTab::HandleInput(float mouseX, float mouseY, bool mouseDown, bool mouseClicked)
{
    // Handle slider dragging first
    if (m_activeSlider)
    {
        if (mouseDown)
        {
            m_activeSlider->OnMouseMove(mouseX, mouseY);
        }
        else
        {
            m_activeSlider->OnMouseUp();
            m_activeSlider = nullptr;
        }
        return;
    }

    // Handle dropdown if one is expanded
    if (m_activeDropdown && m_activeDropdown->IsExpanded())
    {
        m_activeDropdown->SetHovered(m_activeDropdown->HitTestDropdown(mouseX, mouseY));
        if (mouseClicked)
        {
            m_activeDropdown->OnClick(mouseX, mouseY);
            if (!m_activeDropdown->IsExpanded())
            {
                m_activeDropdown = nullptr;
            }
            return;
        }
    }

    // Check all widgets
    for (auto* widget : m_widgets)
    {
        // Checkbox
        if (auto* checkbox = dynamic_cast<UICheckbox*>(widget))
        {
            bool hovered = checkbox->HitTest(mouseX, mouseY);
            checkbox->SetHovered(hovered);
            if (hovered && mouseClicked)
            {
                checkbox->OnClick();
            }
        }
        // Slider
        else if (auto* slider = dynamic_cast<UISlider*>(widget))
        {
            bool hovered = slider->HitTest(mouseX, mouseY);
            slider->SetHovered(hovered);
            if (hovered && mouseClicked)
            {
                slider->OnMouseDown(mouseX, mouseY);
                if (slider->IsDragging())
                {
                    m_activeSlider = slider;
                }
            }
        }
        // Dropdown
        else if (auto* dropdown = dynamic_cast<UIDropdown*>(widget))
        {
            bool hovered = dropdown->HitTest(mouseX, mouseY);
            dropdown->SetHovered(hovered);
            if (hovered && mouseClicked)
            {
                dropdown->OnClick(mouseX, mouseY);
                if (dropdown->IsExpanded())
                {
                    m_activeDropdown = dropdown;
                }
            }
        }
    }
}

void GraphicsSettingsTab::Update(float deltaTime)
{
    for (auto* widget : m_widgets)
    {
        widget->Update(deltaTime);
    }
}

void GraphicsSettingsTab::Render(D2DInterop* d2dInterop)
{
    // Render widgets (except active dropdown which renders last)
    for (auto* widget : m_widgets)
    {
        if (widget != m_activeDropdown)
        {
            widget->Render(d2dInterop);
        }
    }

    // Render active dropdown last (on top)
    if (m_activeDropdown)
    {
        m_activeDropdown->Render(d2dInterop);
    }
}

void GraphicsSettingsTab::SetValues(const GraphicsSettingsValues& values)
{
    m_postProcessCheckbox->SetChecked(values.postProcessEnabled);
    m_bloomCheckbox->SetChecked(values.bloomEnabled);
    m_bloomIntensitySlider->SetValue(values.bloomIntensity);
    m_bloomThresholdSlider->SetValue(values.bloomThreshold);
    m_toneMappingDropdown->SetSelectedIndex(values.toneMappingMode);
    m_exposureSlider->SetValue(values.exposure);
    m_gammaSlider->SetValue(values.gamma);
    m_ssaoCheckbox->SetChecked(values.ssaoEnabled);
    m_ssaoRadiusSlider->SetValue(values.ssaoRadius);
    m_ssaoIntensitySlider->SetValue(values.ssaoIntensity);
}

GraphicsSettingsValues GraphicsSettingsTab::GetValues() const
{
    GraphicsSettingsValues values;
    values.postProcessEnabled = m_postProcessCheckbox->IsChecked();
    values.bloomEnabled = m_bloomCheckbox->IsChecked();
    values.bloomIntensity = m_bloomIntensitySlider->GetValue();
    values.bloomThreshold = m_bloomThresholdSlider->GetValue();
    values.toneMappingMode = m_toneMappingDropdown->GetSelectedIndex();
    values.exposure = m_exposureSlider->GetValue();
    values.gamma = m_gammaSlider->GetValue();
    values.ssaoEnabled = m_ssaoCheckbox->IsChecked();
    values.ssaoRadius = m_ssaoRadiusSlider->GetValue();
    values.ssaoIntensity = m_ssaoIntensitySlider->GetValue();
    return values;
}
