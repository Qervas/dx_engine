#include "UIButton.h"
#include "../RHI/D2DInterop.h"

#undef min
#undef max
#include <algorithm>

UIButton::UIButton(const std::wstring& text)
    : m_text(text)
{
}

void UIButton::Update(float deltaTime)
{
    // Animate hover transition
    const float transitionSpeed = 8.0f;
    float targetProgress = m_hovered ? 1.0f : 0.0f;

    if (m_hoverProgress < targetProgress)
    {
        m_hoverProgress = std::min(m_hoverProgress + deltaTime * transitionSpeed, targetProgress);
    }
    else if (m_hoverProgress > targetProgress)
    {
        m_hoverProgress = std::max(m_hoverProgress - deltaTime * transitionSpeed, targetProgress);
    }
}

void UIButton::Render(D2DInterop* d2d)
{
    if (!m_visible || !d2d)
        return;

    // Interpolate between normal and hover colors
    float r = m_normalColor[0] + (m_hoverColor[0] - m_normalColor[0]) * m_hoverProgress;
    float g = m_normalColor[1] + (m_hoverColor[1] - m_normalColor[1]) * m_hoverProgress;
    float b = m_normalColor[2] + (m_hoverColor[2] - m_normalColor[2]) * m_hoverProgress;
    float a = m_normalColor[3] + (m_hoverColor[3] - m_normalColor[3]) * m_hoverProgress;

    // Draw button background with rounded corners
    d2d->SetBrushColor(r, g, b, a);
    d2d->FillRoundedRect(m_rect.x, m_rect.y, m_rect.width, m_rect.height, 8.0f);

    // Draw button text
    if (m_textFormat)
    {
        d2d->SetBrushColor(m_textColor[0], m_textColor[1], m_textColor[2], m_textColor[3]);
        d2d->DrawText(m_text, m_textFormat, m_rect.x, m_rect.y, m_rect.width, m_rect.height);
    }
}

void UIButton::OnClick()
{
    if (m_onClick)
    {
        m_onClick();
    }
}

bool UIButton::HitTest(float x, float y) const
{
    return m_rect.Contains(x, y);
}

void UIButton::SetNormalColor(float r, float g, float b, float a)
{
    m_normalColor[0] = r;
    m_normalColor[1] = g;
    m_normalColor[2] = b;
    m_normalColor[3] = a;
}

void UIButton::SetHoverColor(float r, float g, float b, float a)
{
    m_hoverColor[0] = r;
    m_hoverColor[1] = g;
    m_hoverColor[2] = b;
    m_hoverColor[3] = a;
}

void UIButton::SetTextColor(float r, float g, float b, float a)
{
    m_textColor[0] = r;
    m_textColor[1] = g;
    m_textColor[2] = b;
    m_textColor[3] = a;
}
