#pragma once

#include "UIElement.h"
#include <dwrite.h>
#include <string>
#include <functional>

class D2DInterop;

class UICheckbox : public UIElement
{
public:
    UICheckbox(const std::wstring& label = L"");
    virtual ~UICheckbox() = default;

    void Update(float deltaTime) override;
    void Render(D2DInterop* d2d) override;

    // State
    void SetChecked(bool checked) { m_checked = checked; }
    bool IsChecked() const { return m_checked; }
    void Toggle() { m_checked = !m_checked; if (m_onChange) m_onChange(m_checked); }

    // Label
    void SetLabel(const std::wstring& label) { m_label = label; }
    const std::wstring& GetLabel() const { return m_label; }

    // Text format
    void SetTextFormat(IDWriteTextFormat* format) { m_textFormat = format; }

    // Callback
    void SetOnChange(std::function<void(bool)> callback) { m_onChange = callback; }

    // Hit testing
    bool HitTest(float x, float y) const;
    void OnClick();

    // Colors
    void SetBoxColor(float r, float g, float b, float a = 1.0f);
    void SetCheckColor(float r, float g, float b, float a = 1.0f);
    void SetTextColor(float r, float g, float b, float a = 1.0f);
    void SetHoverColor(float r, float g, float b, float a = 1.0f);

    // Box size
    void SetBoxSize(float size) { m_boxSize = size; }

private:
    std::wstring m_label;
    bool m_checked = false;
    IDWriteTextFormat* m_textFormat = nullptr;
    std::function<void(bool)> m_onChange;

    float m_boxSize = 20.0f;
    float m_boxColor[4] = { 0.3f, 0.3f, 0.35f, 1.0f };
    float m_checkColor[4] = { 0.4f, 0.7f, 1.0f, 1.0f };
    float m_textColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float m_hoverColor[4] = { 0.4f, 0.4f, 0.45f, 1.0f };

    // Animation
    float m_hoverProgress = 0.0f;
};
