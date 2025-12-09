#include "DisplaySettingsTab.h"
#include "../UICheckbox.h"
#include "../UIDropdown.h"
#include "../../RHI/D2DInterop.h"

DisplaySettingsTab::DisplaySettingsTab()
{
}

DisplaySettingsTab::~DisplaySettingsTab()
{
    Shutdown();
}

bool DisplaySettingsTab::Initialize(IDWriteTextFormat* labelFormat)
{
    CreateWidgets(labelFormat);
    return true;
}

void DisplaySettingsTab::Shutdown()
{
    m_widgets.clear();
    m_displayModeDropdown.reset();
    m_resolutionDropdown.reset();
    m_vsyncCheckbox.reset();
    m_wireframeCheckbox.reset();
    m_debugRenderingCheckbox.reset();
}

void DisplaySettingsTab::CreateWidgets(IDWriteTextFormat* labelFormat)
{
    // Display mode
    m_displayModeDropdown = std::make_unique<UIDropdown>(L"Display Mode");
    m_displayModeDropdown->SetTextFormat(labelFormat);
    m_displayModeDropdown->SetOptions({ L"Windowed", L"Fullscreen Borderless", L"Fullscreen Exclusive" });
    m_displayModeDropdown->SetSelectedIndex(0);
    m_displayModeDropdown->SetOnChange([this](int v) {
        if (m_callbacks.onDisplayModeChanged) m_callbacks.onDisplayModeChanged(v);
    });

    // Resolution
    m_resolutionDropdown = std::make_unique<UIDropdown>(L"Resolution");
    m_resolutionDropdown->SetTextFormat(labelFormat);
    m_resolutionDropdown->SetOptions({
        L"1280 x 720", L"1366 x 768", L"1600 x 900", L"1920 x 1080",
        L"2560 x 1440", L"3840 x 2160"
    });
    m_resolutionDropdown->SetSelectedIndex(0);
    m_resolutionDropdown->SetOnChange([this](int v) {
        if (m_callbacks.onResolutionChanged) m_callbacks.onResolutionChanged(v);
    });

    // VSync
    m_vsyncCheckbox = std::make_unique<UICheckbox>(L"VSync");
    m_vsyncCheckbox->SetTextFormat(labelFormat);
    m_vsyncCheckbox->SetChecked(true);
    m_vsyncCheckbox->SetOnChange([this](bool v) {
        if (m_callbacks.onVSyncChanged) m_callbacks.onVSyncChanged(v);
    });

    // Wireframe
    m_wireframeCheckbox = std::make_unique<UICheckbox>(L"Wireframe");
    m_wireframeCheckbox->SetTextFormat(labelFormat);
    m_wireframeCheckbox->SetChecked(false);
    m_wireframeCheckbox->SetOnChange([this](bool v) {
        if (m_callbacks.onWireframeChanged) m_callbacks.onWireframeChanged(v);
    });

    // Debug rendering
    m_debugRenderingCheckbox = std::make_unique<UICheckbox>(L"Debug Rendering");
    m_debugRenderingCheckbox->SetTextFormat(labelFormat);
    m_debugRenderingCheckbox->SetChecked(true);
    m_debugRenderingCheckbox->SetOnChange([this](bool v) {
        if (m_callbacks.onDebugRenderingChanged) m_callbacks.onDebugRenderingChanged(v);
    });

    // Build widget list
    m_widgets = {
        m_displayModeDropdown.get(),
        m_resolutionDropdown.get(),
        m_vsyncCheckbox.get(),
        m_wireframeCheckbox.get(),
        m_debugRenderingCheckbox.get()
    };
}

void DisplaySettingsTab::Layout(float x, float y, float width, float rowHeight)
{
    for (size_t i = 0; i < m_widgets.size(); ++i)
    {
        m_widgets[i]->SetPosition(x, y + static_cast<float>(i) * rowHeight);
        m_widgets[i]->SetSize(width, rowHeight);
    }
}

void DisplaySettingsTab::HandleInput(float mouseX, float mouseY, bool mouseDown, bool mouseClicked)
{
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

void DisplaySettingsTab::Update(float deltaTime)
{
    for (auto* widget : m_widgets)
    {
        widget->Update(deltaTime);
    }
}

void DisplaySettingsTab::Render(D2DInterop* d2dInterop)
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

void DisplaySettingsTab::SetValues(const DisplaySettingsValues& values)
{
    m_displayModeDropdown->SetSelectedIndex(values.displayMode);
    m_resolutionDropdown->SetSelectedIndex(values.resolutionIndex);
    m_vsyncCheckbox->SetChecked(values.vsyncEnabled);
    m_wireframeCheckbox->SetChecked(values.wireframeEnabled);
    m_debugRenderingCheckbox->SetChecked(values.debugRenderingEnabled);
}

DisplaySettingsValues DisplaySettingsTab::GetValues() const
{
    DisplaySettingsValues values;
    values.displayMode = m_displayModeDropdown->GetSelectedIndex();
    values.resolutionIndex = m_resolutionDropdown->GetSelectedIndex();
    values.vsyncEnabled = m_vsyncCheckbox->IsChecked();
    values.wireframeEnabled = m_wireframeCheckbox->IsChecked();
    values.debugRenderingEnabled = m_debugRenderingCheckbox->IsChecked();
    return values;
}
