#include "SettingsMenu.h"
#include "../RHI/D2DInterop.h"

SettingsMenu::SettingsMenu()
{
}

SettingsMenu::~SettingsMenu()
{
    Shutdown();
}

bool SettingsMenu::Initialize(D2DInterop* d2dInterop, uint32_t screenWidth, uint32_t screenHeight)
{
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    IDWriteFactory* dwrite = d2dInterop->GetDWriteFactory();
    HRESULT hr;

    // Create title text format
    hr = dwrite->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        36.0f,
        L"en-us",
        &m_titleFormat
    );
    if (FAILED(hr)) return false;
    m_titleFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_titleFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Create tab text format
    hr = dwrite->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        18.0f,
        L"en-us",
        &m_tabFormat
    );
    if (FAILED(hr)) return false;
    m_tabFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_tabFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Create label text format
    hr = dwrite->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        16.0f,
        L"en-us",
        &m_labelFormat
    );
    if (FAILED(hr)) return false;
    m_labelFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    m_labelFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Create value text format
    hr = dwrite->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        14.0f,
        L"en-us",
        &m_valueFormat
    );
    if (FAILED(hr)) return false;
    m_valueFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    m_valueFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    CreateUI();
    LayoutUI();

    return true;
}

void SettingsMenu::Shutdown()
{
    m_graphicsWidgets.clear();
    m_displayWidgets.clear();

    m_backButton.reset();
    m_graphicsTabButton.reset();
    m_displayTabButton.reset();

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

    m_vsyncCheckbox.reset();
    m_wireframeCheckbox.reset();
    m_debugRenderingCheckbox.reset();

    m_titleFormat.Reset();
    m_tabFormat.Reset();
    m_labelFormat.Reset();
    m_valueFormat.Reset();
}

void SettingsMenu::CreateUI()
{
    // Back button
    m_backButton = std::make_unique<UIButton>(L"\u2190 Back");
    m_backButton->SetTextFormat(m_tabFormat.Get());
    m_backButton->SetNormalColor(0.2f, 0.2f, 0.25f, 1.0f);
    m_backButton->SetHoverColor(0.3f, 0.3f, 0.35f, 1.0f);
    m_backButton->SetOnClick([this]() { m_lastAction = SettingsAction::Back; });

    // Tab buttons
    m_graphicsTabButton = std::make_unique<UIButton>(L"Graphics");
    m_graphicsTabButton->SetTextFormat(m_tabFormat.Get());
    m_graphicsTabButton->SetNormalColor(0.3f, 0.5f, 0.7f, 1.0f);  // Selected color
    m_graphicsTabButton->SetHoverColor(0.35f, 0.55f, 0.75f, 1.0f);
    m_graphicsTabButton->SetOnClick([this]() { SwitchTab(Tab::Graphics); });

    m_displayTabButton = std::make_unique<UIButton>(L"Display");
    m_displayTabButton->SetTextFormat(m_tabFormat.Get());
    m_displayTabButton->SetNormalColor(0.2f, 0.2f, 0.25f, 1.0f);
    m_displayTabButton->SetHoverColor(0.3f, 0.3f, 0.35f, 1.0f);
    m_displayTabButton->SetOnClick([this]() { SwitchTab(Tab::Display); });

    // --- Graphics settings ---
    m_postProcessCheckbox = std::make_unique<UICheckbox>(L"Post Processing");
    m_postProcessCheckbox->SetTextFormat(m_labelFormat.Get());
    m_postProcessCheckbox->SetChecked(true);
    m_postProcessCheckbox->SetOnChange([this](bool v) { if (m_callbacks.onPostProcessChanged) m_callbacks.onPostProcessChanged(v); });

    m_bloomCheckbox = std::make_unique<UICheckbox>(L"Bloom");
    m_bloomCheckbox->SetTextFormat(m_labelFormat.Get());
    m_bloomCheckbox->SetChecked(true);
    m_bloomCheckbox->SetOnChange([this](bool v) { if (m_callbacks.onBloomChanged) m_callbacks.onBloomChanged(v); });

    m_bloomIntensitySlider = std::make_unique<UISlider>(L"Bloom Intensity");
    m_bloomIntensitySlider->SetTextFormat(m_labelFormat.Get());
    m_bloomIntensitySlider->SetRange(0.0f, 2.0f);
    m_bloomIntensitySlider->SetValue(0.5f);
    m_bloomIntensitySlider->SetOnChange([this](float v) { if (m_callbacks.onBloomIntensityChanged) m_callbacks.onBloomIntensityChanged(v); });

    m_bloomThresholdSlider = std::make_unique<UISlider>(L"Bloom Threshold");
    m_bloomThresholdSlider->SetTextFormat(m_labelFormat.Get());
    m_bloomThresholdSlider->SetRange(0.1f, 5.0f);
    m_bloomThresholdSlider->SetValue(1.5f);
    m_bloomThresholdSlider->SetOnChange([this](float v) { if (m_callbacks.onBloomThresholdChanged) m_callbacks.onBloomThresholdChanged(v); });

    m_toneMappingDropdown = std::make_unique<UIDropdown>(L"Tone Mapping");
    m_toneMappingDropdown->SetTextFormat(m_labelFormat.Get());
    m_toneMappingDropdown->SetOptions({ L"None", L"Reinhard", L"ACES", L"Uncharted2" });
    m_toneMappingDropdown->SetSelectedIndex(2);  // ACES
    m_toneMappingDropdown->SetOnChange([this](int v) { if (m_callbacks.onToneMappingChanged) m_callbacks.onToneMappingChanged(v); });

    m_exposureSlider = std::make_unique<UISlider>(L"Exposure");
    m_exposureSlider->SetTextFormat(m_labelFormat.Get());
    m_exposureSlider->SetRange(0.1f, 5.0f);
    m_exposureSlider->SetValue(1.0f);
    m_exposureSlider->SetOnChange([this](float v) { if (m_callbacks.onExposureChanged) m_callbacks.onExposureChanged(v); });

    m_gammaSlider = std::make_unique<UISlider>(L"Gamma");
    m_gammaSlider->SetTextFormat(m_labelFormat.Get());
    m_gammaSlider->SetRange(1.0f, 3.0f);
    m_gammaSlider->SetValue(2.2f);
    m_gammaSlider->SetOnChange([this](float v) { if (m_callbacks.onGammaChanged) m_callbacks.onGammaChanged(v); });

    m_ssaoCheckbox = std::make_unique<UICheckbox>(L"SSAO");
    m_ssaoCheckbox->SetTextFormat(m_labelFormat.Get());
    m_ssaoCheckbox->SetChecked(true);
    m_ssaoCheckbox->SetOnChange([this](bool v) { if (m_callbacks.onSSAOChanged) m_callbacks.onSSAOChanged(v); });

    m_ssaoRadiusSlider = std::make_unique<UISlider>(L"SSAO Radius");
    m_ssaoRadiusSlider->SetTextFormat(m_labelFormat.Get());
    m_ssaoRadiusSlider->SetRange(0.1f, 2.0f);
    m_ssaoRadiusSlider->SetValue(0.5f);
    m_ssaoRadiusSlider->SetOnChange([this](float v) { if (m_callbacks.onSSAORadiusChanged) m_callbacks.onSSAORadiusChanged(v); });

    m_ssaoIntensitySlider = std::make_unique<UISlider>(L"SSAO Intensity");
    m_ssaoIntensitySlider->SetTextFormat(m_labelFormat.Get());
    m_ssaoIntensitySlider->SetRange(0.5f, 3.0f);
    m_ssaoIntensitySlider->SetValue(1.5f);
    m_ssaoIntensitySlider->SetOnChange([this](float v) { if (m_callbacks.onSSAOIntensityChanged) m_callbacks.onSSAOIntensityChanged(v); });

    // --- Display settings ---
    m_vsyncCheckbox = std::make_unique<UICheckbox>(L"VSync");
    m_vsyncCheckbox->SetTextFormat(m_labelFormat.Get());
    m_vsyncCheckbox->SetChecked(true);
    m_vsyncCheckbox->SetOnChange([this](bool v) { if (m_callbacks.onVSyncChanged) m_callbacks.onVSyncChanged(v); });

    m_wireframeCheckbox = std::make_unique<UICheckbox>(L"Wireframe");
    m_wireframeCheckbox->SetTextFormat(m_labelFormat.Get());
    m_wireframeCheckbox->SetChecked(false);
    m_wireframeCheckbox->SetOnChange([this](bool v) { if (m_callbacks.onWireframeChanged) m_callbacks.onWireframeChanged(v); });

    m_debugRenderingCheckbox = std::make_unique<UICheckbox>(L"Debug Rendering");
    m_debugRenderingCheckbox->SetTextFormat(m_labelFormat.Get());
    m_debugRenderingCheckbox->SetChecked(true);
    m_debugRenderingCheckbox->SetOnChange([this](bool v) { if (m_callbacks.onDebugRenderingChanged) m_callbacks.onDebugRenderingChanged(v); });

    // Build widget lists
    m_graphicsWidgets = {
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

    m_displayWidgets = {
        m_vsyncCheckbox.get(),
        m_wireframeCheckbox.get(),
        m_debugRenderingCheckbox.get()
    };
}

void SettingsMenu::LayoutUI()
{
    const float panelWidth = 700.0f;
    const float panelHeight = 550.0f;
    float panelX = (static_cast<float>(m_screenWidth) - panelWidth) / 2.0f;
    float panelY = (static_cast<float>(m_screenHeight) - panelHeight) / 2.0f;

    const float sidebarWidth = 150.0f;
    const float contentX = panelX + sidebarWidth + 20.0f;
    const float contentWidth = panelWidth - sidebarWidth - 40.0f;

    // Back button (top left)
    m_backButton->SetPosition(panelX + 10.0f, panelY + 10.0f);
    m_backButton->SetSize(100.0f, 35.0f);

    // Tab buttons (left sidebar)
    float tabY = panelY + 80.0f;
    m_graphicsTabButton->SetPosition(panelX + 10.0f, tabY);
    m_graphicsTabButton->SetSize(sidebarWidth - 20.0f, 40.0f);

    m_displayTabButton->SetPosition(panelX + 10.0f, tabY + 50.0f);
    m_displayTabButton->SetSize(sidebarWidth - 20.0f, 40.0f);

    // Settings widgets (right content area)
    const float rowHeight = 40.0f;
    const float startY = panelY + 70.0f;

    // Layout graphics widgets
    for (size_t i = 0; i < m_graphicsWidgets.size(); ++i)
    {
        m_graphicsWidgets[i]->SetPosition(contentX, startY + static_cast<float>(i) * rowHeight);
        m_graphicsWidgets[i]->SetSize(contentWidth, rowHeight);
    }

    // Layout display widgets
    for (size_t i = 0; i < m_displayWidgets.size(); ++i)
    {
        m_displayWidgets[i]->SetPosition(contentX, startY + static_cast<float>(i) * rowHeight);
        m_displayWidgets[i]->SetSize(contentWidth, rowHeight);
    }
}

void SettingsMenu::SwitchTab(Tab tab)
{
    m_currentTab = tab;

    // Update tab button colors
    if (tab == Tab::Graphics)
    {
        m_graphicsTabButton->SetNormalColor(0.3f, 0.5f, 0.7f, 1.0f);
        m_displayTabButton->SetNormalColor(0.2f, 0.2f, 0.25f, 1.0f);
    }
    else
    {
        m_graphicsTabButton->SetNormalColor(0.2f, 0.2f, 0.25f, 1.0f);
        m_displayTabButton->SetNormalColor(0.3f, 0.5f, 0.7f, 1.0f);
    }

    // Close any open dropdown
    if (m_activeDropdown)
    {
        m_activeDropdown->SetExpanded(false);
        m_activeDropdown = nullptr;
    }
}

void SettingsMenu::HandleInput(float mouseX, float mouseY, bool mouseDown, bool mouseClicked)
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
        // Update hovered option (we need to track this in the dropdown)

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

    // Back button
    bool backHovered = m_backButton->HitTest(mouseX, mouseY);
    m_backButton->SetHovered(backHovered);
    if (backHovered && mouseClicked)
    {
        m_backButton->OnClick();
        return;
    }

    // Tab buttons
    bool graphicsTabHovered = m_graphicsTabButton->HitTest(mouseX, mouseY);
    m_graphicsTabButton->SetHovered(graphicsTabHovered);
    if (graphicsTabHovered && mouseClicked)
    {
        m_graphicsTabButton->OnClick();
        return;
    }

    bool displayTabHovered = m_displayTabButton->HitTest(mouseX, mouseY);
    m_displayTabButton->SetHovered(displayTabHovered);
    if (displayTabHovered && mouseClicked)
    {
        m_displayTabButton->OnClick();
        return;
    }

    // Current tab widgets
    auto& widgets = (m_currentTab == Tab::Graphics) ? m_graphicsWidgets : m_displayWidgets;

    for (auto* widget : widgets)
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

void SettingsMenu::Update(float deltaTime)
{
    m_backButton->Update(deltaTime);
    m_graphicsTabButton->Update(deltaTime);
    m_displayTabButton->Update(deltaTime);

    auto& widgets = (m_currentTab == Tab::Graphics) ? m_graphicsWidgets : m_displayWidgets;
    for (auto* widget : widgets)
    {
        widget->Update(deltaTime);
    }
}

void SettingsMenu::Render(D2DInterop* d2d)
{
    if (!d2d)
        return;

    const float panelWidth = 700.0f;
    const float panelHeight = 550.0f;
    float panelX = (static_cast<float>(m_screenWidth) - panelWidth) / 2.0f;
    float panelY = (static_cast<float>(m_screenHeight) - panelHeight) / 2.0f;

    // Draw dark overlay
    d2d->SetBrushColor(0.0f, 0.0f, 0.0f, 0.8f);
    d2d->FillRect(0.0f, 0.0f, static_cast<float>(m_screenWidth), static_cast<float>(m_screenHeight));

    // Draw main panel background
    d2d->SetBrushColor(0.1f, 0.1f, 0.12f, 0.98f);
    d2d->FillRoundedRect(panelX, panelY, panelWidth, panelHeight, 12.0f);

    // Draw sidebar
    const float sidebarWidth = 150.0f;
    d2d->SetBrushColor(0.08f, 0.08f, 0.1f, 1.0f);
    d2d->FillRoundedRect(panelX, panelY, sidebarWidth, panelHeight, 12.0f);

    // Draw title
    d2d->SetBrushColor(1.0f, 1.0f, 1.0f, 1.0f);
    d2d->DrawText(L"SETTINGS", m_titleFormat.Get(),
                  panelX + sidebarWidth, panelY + 10.0f,
                  panelWidth - sidebarWidth, 50.0f);

    // Draw back button
    m_backButton->Render(d2d);

    // Draw tab buttons
    m_graphicsTabButton->Render(d2d);
    m_displayTabButton->Render(d2d);

    // Draw current tab section header
    const float contentX = panelX + sidebarWidth + 20.0f;
    d2d->SetBrushColor(0.7f, 0.7f, 0.7f, 1.0f);
    std::wstring sectionTitle = (m_currentTab == Tab::Graphics) ? L"GRAPHICS" : L"DISPLAY";
    d2d->DrawText(sectionTitle, m_tabFormat.Get(),
                  contentX, panelY + 55.0f, 200.0f, 20.0f);

    // Draw current tab widgets
    auto& widgets = (m_currentTab == Tab::Graphics) ? m_graphicsWidgets : m_displayWidgets;
    for (auto* widget : widgets)
    {
        widget->Render(d2d);
    }

    // Draw expanded dropdown on top (if any)
    if (m_activeDropdown && m_activeDropdown->IsExpanded())
    {
        // The dropdown already renders itself expanded in its Render() call
        // but we could add extra rendering here if needed
    }
}

void SettingsMenu::OnResize(uint32_t width, uint32_t height)
{
    m_screenWidth = width;
    m_screenHeight = height;
    LayoutUI();
}

void SettingsMenu::SetValues(const SettingsValues& values)
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

    m_vsyncCheckbox->SetChecked(values.vsyncEnabled);
    m_wireframeCheckbox->SetChecked(values.wireframeEnabled);
    m_debugRenderingCheckbox->SetChecked(values.debugRenderingEnabled);
}

SettingsValues SettingsMenu::GetValues() const
{
    SettingsValues values;
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

    values.vsyncEnabled = m_vsyncCheckbox->IsChecked();
    values.wireframeEnabled = m_wireframeCheckbox->IsChecked();
    values.debugRenderingEnabled = m_debugRenderingCheckbox->IsChecked();

    return values;
}
