#pragma once

#include "UIElement.h"
#include <dwrite.h>
#include <string>
#include <vector>
#include <functional>

class D2DInterop;

class UIDropdown : public UIElement
{
public:
    UIDropdown(const std::wstring& label = L"");
    virtual ~UIDropdown() = default;

    void Update(float deltaTime) override;
    void Render(D2DInterop* d2d) override;

    // Options
    void SetOptions(const std::vector<std::wstring>& options) { m_options = options; }
    void AddOption(const std::wstring& option) { m_options.push_back(option); }
    void ClearOptions() { m_options.clear(); m_selectedIndex = 0; }

    // Selection
    void SetSelectedIndex(int index);
    int GetSelectedIndex() const { return m_selectedIndex; }
    const std::wstring& GetSelectedOption() const;

    // Label
    void SetLabel(const std::wstring& label) { m_label = label; }
    const std::wstring& GetLabel() const { return m_label; }

    // Expanded state
    bool IsExpanded() const { return m_expanded; }
    void SetExpanded(bool expanded) { m_expanded = expanded; }
    void ToggleExpanded() { m_expanded = !m_expanded; }

    // Text format
    void SetTextFormat(IDWriteTextFormat* format) { m_textFormat = format; }

    // Callback
    void SetOnChange(std::function<void(int)> callback) { m_onChange = callback; }

    // Hit testing
    bool HitTest(float x, float y) const;
    bool HitTestDropdown(float x, float y) const;  // Test against expanded dropdown area
    int HitTestOption(float x, float y) const;  // Returns option index or -1
    void OnClick(float x, float y);

    // Colors
    void SetBackgroundColor(float r, float g, float b, float a = 1.0f);
    void SetHoverColor(float r, float g, float b, float a = 1.0f);
    void SetTextColor(float r, float g, float b, float a = 1.0f);
    void SetBorderColor(float r, float g, float b, float a = 1.0f);

    // Dimensions
    void SetLabelWidth(float width) { m_labelWidth = width; }
    void SetOptionHeight(float height) { m_optionHeight = height; }

    // Get expanded bounds (for proper layering/clipping)
    UIRect GetExpandedBounds() const;

private:
    std::wstring m_label;
    std::vector<std::wstring> m_options;
    int m_selectedIndex = 0;
    bool m_expanded = false;
    int m_hoveredOption = -1;

    IDWriteTextFormat* m_textFormat = nullptr;
    std::function<void(int)> m_onChange;

    // Dimensions
    float m_labelWidth = 150.0f;
    float m_optionHeight = 30.0f;

    // Colors
    float m_backgroundColor[4] = { 0.2f, 0.2f, 0.25f, 1.0f };
    float m_hoverColor[4] = { 0.3f, 0.3f, 0.35f, 1.0f };
    float m_textColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float m_borderColor[4] = { 0.4f, 0.4f, 0.45f, 1.0f };

    // Animation
    float m_hoverProgress = 0.0f;

    static const std::wstring s_emptyOption;
};
