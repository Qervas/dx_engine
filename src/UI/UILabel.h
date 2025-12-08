#pragma once

#include "UIElement.h"
#include <dwrite.h>
#include <string>

class D2DInterop;

class UILabel : public UIElement
{
public:
    UILabel(const std::wstring& text = L"");
    virtual ~UILabel() = default;

    void Update(float deltaTime) override;
    void Render(D2DInterop* d2d) override;

    // Text
    void SetText(const std::wstring& text) { m_text = text; }
    const std::wstring& GetText() const { return m_text; }

    // Text format
    void SetTextFormat(IDWriteTextFormat* format) { m_textFormat = format; }

    // Colors
    void SetTextColor(float r, float g, float b, float a = 1.0f);

    // Alignment
    enum class HAlign { Left, Center, Right };
    enum class VAlign { Top, Center, Bottom };
    void SetHorizontalAlignment(HAlign align) { m_hAlign = align; }
    void SetVerticalAlignment(VAlign align) { m_vAlign = align; }

private:
    std::wstring m_text;
    IDWriteTextFormat* m_textFormat = nullptr;
    float m_textColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    HAlign m_hAlign = HAlign::Left;
    VAlign m_vAlign = VAlign::Center;
};
