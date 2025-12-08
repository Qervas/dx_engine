#include "UIDropdown.h"
#include "../RHI/D2DInterop.h"

#undef min
#undef max
#include <algorithm>

const std::wstring UIDropdown::s_emptyOption = L"";

UIDropdown::UIDropdown(const std::wstring& label)
    : m_label(label)
{
}

void UIDropdown::Update(float deltaTime)
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

void UIDropdown::Render(D2DInterop* d2d)
{
    if (!m_visible || !d2d)
        return;

    // Calculate dropdown button bounds
    float buttonX = m_rect.x + m_labelWidth;
    float buttonWidth = m_rect.width - m_labelWidth;
    float buttonY = m_rect.y;
    float buttonHeight = m_rect.height;

    // Draw label
    if (m_textFormat && !m_label.empty())
    {
        d2d->SetBrushColor(m_textColor[0], m_textColor[1], m_textColor[2], m_textColor[3]);
        d2d->DrawText(m_label, m_textFormat, m_rect.x, m_rect.y, m_labelWidth - 10.0f, m_rect.height);
    }

    // Interpolate background color based on hover
    float r = m_backgroundColor[0] + (m_hoverColor[0] - m_backgroundColor[0]) * m_hoverProgress;
    float g = m_backgroundColor[1] + (m_hoverColor[1] - m_backgroundColor[1]) * m_hoverProgress;
    float b = m_backgroundColor[2] + (m_hoverColor[2] - m_backgroundColor[2]) * m_hoverProgress;
    float a = m_backgroundColor[3] + (m_hoverColor[3] - m_backgroundColor[3]) * m_hoverProgress;

    // Draw dropdown button background
    d2d->SetBrushColor(r, g, b, a);
    d2d->FillRoundedRect(buttonX, buttonY, buttonWidth, buttonHeight, 4.0f);

    // Draw border
    d2d->SetBrushColor(m_borderColor[0], m_borderColor[1], m_borderColor[2], m_borderColor[3]);
    d2d->DrawRoundedRect(buttonX, buttonY, buttonWidth, buttonHeight, 4.0f, 1.0f);

    // Draw selected option text
    if (m_textFormat && m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_options.size()))
    {
        d2d->SetBrushColor(m_textColor[0], m_textColor[1], m_textColor[2], m_textColor[3]);
        d2d->DrawText(m_options[m_selectedIndex], m_textFormat,
                      buttonX + 10.0f, buttonY, buttonWidth - 30.0f, buttonHeight);
    }

    // Draw dropdown arrow
    float arrowX = buttonX + buttonWidth - 20.0f;
    float arrowY = buttonY + buttonHeight / 2.0f;
    float arrowSize = 5.0f;

    d2d->SetBrushColor(m_textColor[0], m_textColor[1], m_textColor[2], m_textColor[3]);
    if (m_expanded)
    {
        // Up arrow
        d2d->DrawLine(arrowX - arrowSize, arrowY + arrowSize / 2.0f, arrowX, arrowY - arrowSize / 2.0f, 2.0f);
        d2d->DrawLine(arrowX, arrowY - arrowSize / 2.0f, arrowX + arrowSize, arrowY + arrowSize / 2.0f, 2.0f);
    }
    else
    {
        // Down arrow
        d2d->DrawLine(arrowX - arrowSize, arrowY - arrowSize / 2.0f, arrowX, arrowY + arrowSize / 2.0f, 2.0f);
        d2d->DrawLine(arrowX, arrowY + arrowSize / 2.0f, arrowX + arrowSize, arrowY - arrowSize / 2.0f, 2.0f);
    }

    // Draw expanded dropdown list
    if (m_expanded && !m_options.empty())
    {
        float listY = buttonY + buttonHeight + 2.0f;
        float listHeight = static_cast<float>(m_options.size()) * m_optionHeight;

        // Draw list background
        d2d->SetBrushColor(m_backgroundColor[0], m_backgroundColor[1], m_backgroundColor[2], 0.98f);
        d2d->FillRoundedRect(buttonX, listY, buttonWidth, listHeight, 4.0f);

        // Draw list border
        d2d->SetBrushColor(m_borderColor[0], m_borderColor[1], m_borderColor[2], m_borderColor[3]);
        d2d->DrawRoundedRect(buttonX, listY, buttonWidth, listHeight, 4.0f, 1.0f);

        // Draw options
        for (size_t i = 0; i < m_options.size(); ++i)
        {
            float optionY = listY + static_cast<float>(i) * m_optionHeight;

            // Highlight hovered or selected option
            if (static_cast<int>(i) == m_hoveredOption)
            {
                d2d->SetBrushColor(m_hoverColor[0], m_hoverColor[1], m_hoverColor[2], m_hoverColor[3]);
                d2d->FillRect(buttonX + 2.0f, optionY + 2.0f, buttonWidth - 4.0f, m_optionHeight - 4.0f);
            }
            else if (static_cast<int>(i) == m_selectedIndex)
            {
                d2d->SetBrushColor(0.25f, 0.25f, 0.3f, 1.0f);
                d2d->FillRect(buttonX + 2.0f, optionY + 2.0f, buttonWidth - 4.0f, m_optionHeight - 4.0f);
            }

            // Draw option text
            if (m_textFormat)
            {
                d2d->SetBrushColor(m_textColor[0], m_textColor[1], m_textColor[2], m_textColor[3]);
                d2d->DrawText(m_options[i], m_textFormat,
                              buttonX + 10.0f, optionY, buttonWidth - 20.0f, m_optionHeight);
            }
        }
    }
}

void UIDropdown::SetSelectedIndex(int index)
{
    if (index >= 0 && index < static_cast<int>(m_options.size()))
    {
        m_selectedIndex = index;
    }
}

const std::wstring& UIDropdown::GetSelectedOption() const
{
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_options.size()))
    {
        return m_options[m_selectedIndex];
    }
    return s_emptyOption;
}

bool UIDropdown::HitTest(float x, float y) const
{
    // Test against button area
    float buttonX = m_rect.x + m_labelWidth;
    float buttonWidth = m_rect.width - m_labelWidth;

    return x >= buttonX && x <= buttonX + buttonWidth &&
           y >= m_rect.y && y <= m_rect.y + m_rect.height;
}

bool UIDropdown::HitTestDropdown(float x, float y) const
{
    if (!m_expanded)
        return HitTest(x, y);

    UIRect bounds = GetExpandedBounds();
    return bounds.Contains(x, y);
}

int UIDropdown::HitTestOption(float x, float y) const
{
    if (!m_expanded || m_options.empty())
        return -1;

    float buttonX = m_rect.x + m_labelWidth;
    float buttonWidth = m_rect.width - m_labelWidth;
    float listY = m_rect.y + m_rect.height + 2.0f;

    if (x < buttonX || x > buttonX + buttonWidth)
        return -1;

    if (y < listY)
        return -1;

    int index = static_cast<int>((y - listY) / m_optionHeight);
    if (index >= 0 && index < static_cast<int>(m_options.size()))
        return index;

    return -1;
}

void UIDropdown::OnClick(float x, float y)
{
    if (m_expanded)
    {
        int optionIndex = HitTestOption(x, y);
        if (optionIndex >= 0)
        {
            m_selectedIndex = optionIndex;
            m_expanded = false;
            if (m_onChange)
            {
                m_onChange(m_selectedIndex);
            }
        }
        else if (HitTest(x, y))
        {
            // Clicked on button while expanded - close
            m_expanded = false;
        }
        else
        {
            // Clicked outside - close
            m_expanded = false;
        }
    }
    else if (HitTest(x, y))
    {
        m_expanded = true;
    }
}

UIRect UIDropdown::GetExpandedBounds() const
{
    float buttonX = m_rect.x + m_labelWidth;
    float buttonWidth = m_rect.width - m_labelWidth;
    float totalHeight = m_rect.height;

    if (m_expanded && !m_options.empty())
    {
        totalHeight += 2.0f + static_cast<float>(m_options.size()) * m_optionHeight;
    }

    return UIRect{ buttonX, m_rect.y, buttonWidth, totalHeight };
}

void UIDropdown::SetBackgroundColor(float r, float g, float b, float a)
{
    m_backgroundColor[0] = r;
    m_backgroundColor[1] = g;
    m_backgroundColor[2] = b;
    m_backgroundColor[3] = a;
}

void UIDropdown::SetHoverColor(float r, float g, float b, float a)
{
    m_hoverColor[0] = r;
    m_hoverColor[1] = g;
    m_hoverColor[2] = b;
    m_hoverColor[3] = a;
}

void UIDropdown::SetTextColor(float r, float g, float b, float a)
{
    m_textColor[0] = r;
    m_textColor[1] = g;
    m_textColor[2] = b;
    m_textColor[3] = a;
}

void UIDropdown::SetBorderColor(float r, float g, float b, float a)
{
    m_borderColor[0] = r;
    m_borderColor[1] = g;
    m_borderColor[2] = b;
    m_borderColor[3] = a;
}
