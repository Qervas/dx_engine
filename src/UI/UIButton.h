#pragma once

#include "UIElement.h"
#include <string>
#include <functional>
#include <dwrite.h>

class UIButton : public UIElement
{
public:
    UIButton(const std::wstring& text);
    ~UIButton() override = default;

    void Update(float deltaTime) override;
    void Render(D2DInterop* d2d) override;

    void SetText(const std::wstring& text) { m_text = text; }
    const std::wstring& GetText() const { return m_text; }

    void SetOnClick(std::function<void()> callback) { m_onClick = callback; }
    void SetTextFormat(IDWriteTextFormat* format) { m_textFormat = format; }

    void OnClick();
    bool HitTest(float x, float y) const;

    // Visual customization
    void SetNormalColor(float r, float g, float b, float a = 1.0f);
    void SetHoverColor(float r, float g, float b, float a = 1.0f);
    void SetTextColor(float r, float g, float b, float a = 1.0f);

private:
    std::wstring m_text;
    std::function<void()> m_onClick;
    IDWriteTextFormat* m_textFormat = nullptr;

    // Colors (RGBA)
    float m_normalColor[4] = { 0.15f, 0.15f, 0.25f, 0.95f };
    float m_hoverColor[4] = { 0.25f, 0.25f, 0.45f, 1.0f };
    float m_textColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    // Hover animation
    float m_hoverProgress = 0.0f;
};
