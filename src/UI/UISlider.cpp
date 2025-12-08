#include "UISlider.h"
#include "../RHI/D2DInterop.h"

#undef min
#undef max
#include <algorithm>
#include <cstdio>

UISlider::UISlider(const std::wstring& label)
    : m_label(label)
{
}

void UISlider::Update(float deltaTime)
{
    // Animate hover transition
    const float transitionSpeed = 8.0f;
    float targetProgress = (m_hovered || m_dragging) ? 1.0f : 0.0f;

    if (m_hoverProgress < targetProgress)
    {
        m_hoverProgress = std::min(m_hoverProgress + deltaTime * transitionSpeed, targetProgress);
    }
    else if (m_hoverProgress > targetProgress)
    {
        m_hoverProgress = std::max(m_hoverProgress - deltaTime * transitionSpeed, targetProgress);
    }
}

void UISlider::Render(D2DInterop* d2d)
{
    if (!m_visible || !d2d)
        return;

    // Calculate track bounds
    float trackX = m_rect.x + m_labelWidth;
    float trackWidth = m_rect.width - m_labelWidth - m_valueWidth - 10.0f;
    float trackY = m_rect.y + (m_rect.height - m_trackHeight) / 2.0f;

    // Draw label
    if (m_textFormat && !m_label.empty())
    {
        d2d->SetBrushColor(m_textColor[0], m_textColor[1], m_textColor[2], m_textColor[3]);
        d2d->DrawText(m_label, m_textFormat, m_rect.x, m_rect.y, m_labelWidth - 10.0f, m_rect.height);
    }

    // Draw track background
    d2d->SetBrushColor(m_trackColor[0], m_trackColor[1], m_trackColor[2], m_trackColor[3]);
    d2d->FillRoundedRect(trackX, trackY, trackWidth, m_trackHeight, m_trackHeight / 2.0f);

    // Calculate fill width based on value
    float normalizedValue = (m_value - m_min) / (m_max - m_min);
    float fillWidth = trackWidth * normalizedValue;

    // Draw filled portion
    if (fillWidth > 0)
    {
        d2d->SetBrushColor(m_fillColor[0], m_fillColor[1], m_fillColor[2], m_fillColor[3]);
        d2d->FillRoundedRect(trackX, trackY, fillWidth, m_trackHeight, m_trackHeight / 2.0f);
    }

    // Draw handle
    float handleX = trackX + fillWidth;
    float handleY = m_rect.y + m_rect.height / 2.0f;
    float handleScale = 1.0f + m_hoverProgress * 0.2f;  // Grow slightly on hover

    d2d->SetBrushColor(m_handleColor[0], m_handleColor[1], m_handleColor[2], m_handleColor[3]);
    d2d->FillCircle(handleX, handleY, m_handleRadius * handleScale);

    // Draw value text
    if (m_showValue && m_textFormat)
    {
        wchar_t valueText[32];
        swprintf_s(valueText, m_valueFormat.c_str(), m_value);

        d2d->SetBrushColor(m_textColor[0], m_textColor[1], m_textColor[2], m_textColor[3]);
        float valueX = trackX + trackWidth + 10.0f;
        d2d->DrawText(valueText, m_textFormat, valueX, m_rect.y, m_valueWidth, m_rect.height);
    }
}

void UISlider::SetValue(float value)
{
    // Clamp to range
    value = std::max(m_min, std::min(m_max, value));

    // Apply step if set
    if (m_step > 0)
    {
        value = m_min + std::round((value - m_min) / m_step) * m_step;
        value = std::max(m_min, std::min(m_max, value));
    }

    if (m_value != value)
    {
        m_value = value;
    }
}

bool UISlider::HitTest(float x, float y) const
{
    return m_rect.Contains(x, y);
}

bool UISlider::HitTestTrack(float x, float y) const
{
    float trackX = m_rect.x + m_labelWidth;
    float trackWidth = m_rect.width - m_labelWidth - m_valueWidth - 10.0f;
    float trackY = m_rect.y;
    float trackHeight = m_rect.height;

    return x >= trackX - m_handleRadius && x <= trackX + trackWidth + m_handleRadius &&
           y >= trackY && y <= trackY + trackHeight;
}

void UISlider::OnMouseDown(float x, float y)
{
    if (HitTestTrack(x, y))
    {
        m_dragging = true;
        OnMouseMove(x, y);
    }
}

void UISlider::OnMouseMove(float x, float y)
{
    if (m_dragging)
    {
        float trackX = m_rect.x + m_labelWidth;
        float trackWidth = m_rect.width - m_labelWidth - m_valueWidth - 10.0f;

        float normalizedX = (x - trackX) / trackWidth;
        normalizedX = std::max(0.0f, std::min(1.0f, normalizedX));

        float newValue = m_min + normalizedX * (m_max - m_min);
        SetValue(newValue);
        NotifyChange();
    }
}

void UISlider::OnMouseUp()
{
    m_dragging = false;
}

float UISlider::ValueToPosition(float value) const
{
    float trackX = m_rect.x + m_labelWidth;
    float trackWidth = m_rect.width - m_labelWidth - m_valueWidth - 10.0f;
    float normalizedValue = (value - m_min) / (m_max - m_min);
    return trackX + trackWidth * normalizedValue;
}

float UISlider::PositionToValue(float x) const
{
    float trackX = m_rect.x + m_labelWidth;
    float trackWidth = m_rect.width - m_labelWidth - m_valueWidth - 10.0f;
    float normalizedX = (x - trackX) / trackWidth;
    normalizedX = std::max(0.0f, std::min(1.0f, normalizedX));
    return m_min + normalizedX * (m_max - m_min);
}

void UISlider::NotifyChange()
{
    if (m_onChange)
    {
        m_onChange(m_value);
    }
}

void UISlider::SetTrackColor(float r, float g, float b, float a)
{
    m_trackColor[0] = r;
    m_trackColor[1] = g;
    m_trackColor[2] = b;
    m_trackColor[3] = a;
}

void UISlider::SetFillColor(float r, float g, float b, float a)
{
    m_fillColor[0] = r;
    m_fillColor[1] = g;
    m_fillColor[2] = b;
    m_fillColor[3] = a;
}

void UISlider::SetHandleColor(float r, float g, float b, float a)
{
    m_handleColor[0] = r;
    m_handleColor[1] = g;
    m_handleColor[2] = b;
    m_handleColor[3] = a;
}

void UISlider::SetTextColor(float r, float g, float b, float a)
{
    m_textColor[0] = r;
    m_textColor[1] = g;
    m_textColor[2] = b;
    m_textColor[3] = a;
}
