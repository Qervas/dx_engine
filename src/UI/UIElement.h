#pragma once

#include <cstdint>

class D2DInterop;

struct UIRect
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool Contains(float px, float py) const
    {
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }
};

class UIElement
{
public:
    virtual ~UIElement() = default;

    virtual void Update(float deltaTime) = 0;
    virtual void Render(D2DInterop* d2d) = 0;

    void SetPosition(float x, float y) { m_rect.x = x; m_rect.y = y; }
    void SetSize(float w, float h) { m_rect.width = w; m_rect.height = h; }
    const UIRect& GetRect() const { return m_rect; }

    bool IsHovered() const { return m_hovered; }
    void SetHovered(bool hovered) { m_hovered = hovered; }

    bool IsVisible() const { return m_visible; }
    void SetVisible(bool visible) { m_visible = visible; }

protected:
    UIRect m_rect = {};
    bool m_hovered = false;
    bool m_visible = true;
};
