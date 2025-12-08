#include "QuickSettings.h"
#include "../RHI/D2DInterop.h"

QuickSettings::QuickSettings()
{
}

QuickSettings::~QuickSettings()
{
    Shutdown();
}

bool QuickSettings::Initialize(D2DInterop* d2dInterop, uint32_t screenWidth, uint32_t screenHeight)
{
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    IDWriteFactory* dwrite = d2dInterop->GetDWriteFactory();
    HRESULT hr;

    // Create title text format
    hr = dwrite->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        16.0f,
        L"en-us",
        &m_titleFormat
    );
    if (FAILED(hr)) return false;
    m_titleFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    m_titleFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Create label text format
    hr = dwrite->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        14.0f,
        L"en-us",
        &m_labelFormat
    );
    if (FAILED(hr)) return false;
    m_labelFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    m_labelFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    CreateUI();
    LayoutUI();

    return true;
}

void QuickSettings::Shutdown()
{
    m_widgets.clear();

    m_bloomCheckbox.reset();
    m_ssaoCheckbox.reset();
    m_postProcessCheckbox.reset();
    m_wireframeCheckbox.reset();
    m_exposureSlider.reset();

    m_titleFormat.Reset();
    m_labelFormat.Reset();
}

void QuickSettings::CreateUI()
{
    m_bloomCheckbox = std::make_unique<UICheckbox>(L"Bloom");
    m_bloomCheckbox->SetTextFormat(m_labelFormat.Get());
    m_bloomCheckbox->SetChecked(true);
    m_bloomCheckbox->SetOnChange([this](bool v) { if (m_callbacks.onBloomChanged) m_callbacks.onBloomChanged(v); });

    m_ssaoCheckbox = std::make_unique<UICheckbox>(L"SSAO");
    m_ssaoCheckbox->SetTextFormat(m_labelFormat.Get());
    m_ssaoCheckbox->SetChecked(true);
    m_ssaoCheckbox->SetOnChange([this](bool v) { if (m_callbacks.onSSAOChanged) m_callbacks.onSSAOChanged(v); });

    m_postProcessCheckbox = std::make_unique<UICheckbox>(L"Post FX");
    m_postProcessCheckbox->SetTextFormat(m_labelFormat.Get());
    m_postProcessCheckbox->SetChecked(true);
    m_postProcessCheckbox->SetOnChange([this](bool v) { if (m_callbacks.onPostProcessChanged) m_callbacks.onPostProcessChanged(v); });

    m_wireframeCheckbox = std::make_unique<UICheckbox>(L"Wireframe");
    m_wireframeCheckbox->SetTextFormat(m_labelFormat.Get());
    m_wireframeCheckbox->SetChecked(false);
    m_wireframeCheckbox->SetOnChange([this](bool v) { if (m_callbacks.onWireframeChanged) m_callbacks.onWireframeChanged(v); });

    m_exposureSlider = std::make_unique<UISlider>(L"Exposure");
    m_exposureSlider->SetTextFormat(m_labelFormat.Get());
    m_exposureSlider->SetRange(0.1f, 5.0f);
    m_exposureSlider->SetValue(1.0f);
    m_exposureSlider->SetLabelWidth(70.0f);
    m_exposureSlider->SetOnChange([this](float v) { if (m_callbacks.onExposureChanged) m_callbacks.onExposureChanged(v); });

    m_widgets = {
        m_bloomCheckbox.get(),
        m_ssaoCheckbox.get(),
        m_postProcessCheckbox.get(),
        m_wireframeCheckbox.get(),
        m_exposureSlider.get()
    };
}

void QuickSettings::LayoutUI()
{
    const float panelWidth = 200.0f;
    const float panelHeight = 200.0f;
    const float padding = 10.0f;
    float panelX = static_cast<float>(m_screenWidth) - panelWidth - 20.0f;
    float panelY = 80.0f;  // Below menu bar area

    const float rowHeight = 30.0f;
    const float startY = panelY + 35.0f;  // After title

    for (size_t i = 0; i < m_widgets.size(); ++i)
    {
        m_widgets[i]->SetPosition(panelX + padding, startY + static_cast<float>(i) * rowHeight);
        m_widgets[i]->SetSize(panelWidth - padding * 2.0f, rowHeight);
    }
}

void QuickSettings::HandleInput(float mouseX, float mouseY, bool mouseDown, bool mouseClicked)
{
    if (!m_visible)
        return;

    // Handle slider dragging
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

    for (auto* widget : m_widgets)
    {
        if (auto* checkbox = dynamic_cast<UICheckbox*>(widget))
        {
            bool hovered = checkbox->HitTest(mouseX, mouseY);
            checkbox->SetHovered(hovered);
            if (hovered && mouseClicked)
            {
                checkbox->OnClick();
            }
        }
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
    }
}

void QuickSettings::Update(float deltaTime)
{
    if (!m_visible)
        return;

    for (auto* widget : m_widgets)
    {
        widget->Update(deltaTime);
    }
}

void QuickSettings::Render(D2DInterop* d2d)
{
    if (!m_visible || !d2d)
        return;

    const float panelWidth = 200.0f;
    const float panelHeight = 200.0f;
    float panelX = static_cast<float>(m_screenWidth) - panelWidth - 20.0f;
    float panelY = 80.0f;

    // Draw panel background (semi-transparent)
    d2d->SetBrushColor(0.1f, 0.1f, 0.12f, 0.85f);
    d2d->FillRoundedRect(panelX, panelY, panelWidth, panelHeight, 8.0f);

    // Draw panel border
    d2d->SetBrushColor(0.3f, 0.3f, 0.35f, 1.0f);
    d2d->DrawRoundedRect(panelX, panelY, panelWidth, panelHeight, 8.0f, 1.0f);

    // Draw title
    d2d->SetBrushColor(1.0f, 1.0f, 1.0f, 1.0f);
    d2d->DrawText(L"Quick Settings", m_titleFormat.Get(),
                  panelX + 10.0f, panelY + 8.0f, panelWidth - 20.0f, 25.0f);

    // Draw separator line
    d2d->SetBrushColor(0.3f, 0.3f, 0.35f, 1.0f);
    d2d->DrawLine(panelX + 10.0f, panelY + 32.0f, panelX + panelWidth - 10.0f, panelY + 32.0f, 1.0f);

    // Draw widgets
    for (auto* widget : m_widgets)
    {
        widget->Render(d2d);
    }
}

void QuickSettings::OnResize(uint32_t width, uint32_t height)
{
    m_screenWidth = width;
    m_screenHeight = height;
    LayoutUI();
}

void QuickSettings::SetValues(const QuickSettingsValues& values)
{
    m_bloomCheckbox->SetChecked(values.bloomEnabled);
    m_ssaoCheckbox->SetChecked(values.ssaoEnabled);
    m_postProcessCheckbox->SetChecked(values.postProcessEnabled);
    m_wireframeCheckbox->SetChecked(values.wireframeEnabled);
    m_exposureSlider->SetValue(values.exposure);
}

QuickSettingsValues QuickSettings::GetValues() const
{
    QuickSettingsValues values;
    values.bloomEnabled = m_bloomCheckbox->IsChecked();
    values.ssaoEnabled = m_ssaoCheckbox->IsChecked();
    values.postProcessEnabled = m_postProcessCheckbox->IsChecked();
    values.wireframeEnabled = m_wireframeCheckbox->IsChecked();
    values.exposure = m_exposureSlider->GetValue();
    return values;
}
