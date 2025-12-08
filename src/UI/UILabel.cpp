#include "UILabel.h"
#include "../RHI/D2DInterop.h"

UILabel::UILabel(const std::wstring& text)
    : m_text(text)
{
}

void UILabel::Update(float deltaTime)
{
    // Labels don't need updating
}

void UILabel::Render(D2DInterop* d2d)
{
    if (!m_visible || !d2d || !m_textFormat)
        return;

    d2d->SetBrushColor(m_textColor[0], m_textColor[1], m_textColor[2], m_textColor[3]);
    d2d->DrawText(m_text, m_textFormat, m_rect.x, m_rect.y, m_rect.width, m_rect.height);
}

void UILabel::SetTextColor(float r, float g, float b, float a)
{
    m_textColor[0] = r;
    m_textColor[1] = g;
    m_textColor[2] = b;
    m_textColor[3] = a;
}
