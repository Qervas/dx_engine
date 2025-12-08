#include "UICheckbox.h"
#include "../RHI/D2DInterop.h"

#undef min
#undef max
#include <algorithm>

UICheckbox::UICheckbox(const std::wstring& label)
    : m_label(label)
{
}

void UICheckbox::Update(float deltaTime)
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

void UICheckbox::Render(D2DInterop* d2d)
{
    if (!m_visible || !d2d)
        return;

    float boxX = m_rect.x;
    float boxY = m_rect.y + (m_rect.height - m_boxSize) / 2.0f;

    // Interpolate box color based on hover
    float r = m_boxColor[0] + (m_hoverColor[0] - m_boxColor[0]) * m_hoverProgress;
    float g = m_boxColor[1] + (m_hoverColor[1] - m_boxColor[1]) * m_hoverProgress;
    float b = m_boxColor[2] + (m_hoverColor[2] - m_boxColor[2]) * m_hoverProgress;
    float a = m_boxColor[3] + (m_hoverColor[3] - m_boxColor[3]) * m_hoverProgress;

    // Draw box background
    d2d->SetBrushColor(r, g, b, a);
    d2d->FillRoundedRect(boxX, boxY, m_boxSize, m_boxSize, 4.0f);

    // Draw box border
    d2d->SetBrushColor(0.5f, 0.5f, 0.55f, 1.0f);
    d2d->DrawRoundedRect(boxX, boxY, m_boxSize, m_boxSize, 4.0f, 1.0f);

    // Draw checkmark if checked
    if (m_checked)
    {
        d2d->SetBrushColor(m_checkColor[0], m_checkColor[1], m_checkColor[2], m_checkColor[3]);

        // Draw checkmark as two lines
        float cx = boxX + m_boxSize / 2.0f;
        float cy = boxY + m_boxSize / 2.0f;
        float size = m_boxSize * 0.3f;

        // First line (short part of check)
        d2d->DrawLine(cx - size, cy, cx - size * 0.3f, cy + size * 0.7f, 2.5f);
        // Second line (long part of check)
        d2d->DrawLine(cx - size * 0.3f, cy + size * 0.7f, cx + size, cy - size * 0.5f, 2.5f);
    }

    // Draw label
    if (m_textFormat && !m_label.empty())
    {
        d2d->SetBrushColor(m_textColor[0], m_textColor[1], m_textColor[2], m_textColor[3]);
        float labelX = boxX + m_boxSize + 10.0f;
        float labelWidth = m_rect.width - m_boxSize - 10.0f;
        d2d->DrawText(m_label, m_textFormat, labelX, m_rect.y, labelWidth, m_rect.height);
    }
}

bool UICheckbox::HitTest(float x, float y) const
{
    return m_rect.Contains(x, y);
}

void UICheckbox::OnClick()
{
    Toggle();
}

void UICheckbox::SetBoxColor(float r, float g, float b, float a)
{
    m_boxColor[0] = r;
    m_boxColor[1] = g;
    m_boxColor[2] = b;
    m_boxColor[3] = a;
}

void UICheckbox::SetCheckColor(float r, float g, float b, float a)
{
    m_checkColor[0] = r;
    m_checkColor[1] = g;
    m_checkColor[2] = b;
    m_checkColor[3] = a;
}

void UICheckbox::SetTextColor(float r, float g, float b, float a)
{
    m_textColor[0] = r;
    m_textColor[1] = g;
    m_textColor[2] = b;
    m_textColor[3] = a;
}

void UICheckbox::SetHoverColor(float r, float g, float b, float a)
{
    m_hoverColor[0] = r;
    m_hoverColor[1] = g;
    m_hoverColor[2] = b;
    m_hoverColor[3] = a;
}
