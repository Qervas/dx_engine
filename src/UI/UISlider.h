#pragma once

#include "UIElement.h"
#include <dwrite.h>
#include <string>
#include <functional>

class D2DInterop;

class UISlider : public UIElement
{
public:
    UISlider(const std::wstring& label = L"");
    virtual ~UISlider() = default;

    void Update(float deltaTime) override;
    void Render(D2DInterop* d2d) override;

    // Value
    void SetValue(float value);
    float GetValue() const { return m_value; }

    // Range
    void SetRange(float min, float max) { m_min = min; m_max = max; }
    float GetMin() const { return m_min; }
    float GetMax() const { return m_max; }

    // Step (for snapping)
    void SetStep(float step) { m_step = step; }

    // Label
    void SetLabel(const std::wstring& label) { m_label = label; }
    const std::wstring& GetLabel() const { return m_label; }

    // Show value text
    void SetShowValue(bool show) { m_showValue = show; }

    // Format string for value display (e.g., "%.2f")
    void SetValueFormat(const std::wstring& format) { m_valueFormat = format; }

    // Text format
    void SetTextFormat(IDWriteTextFormat* format) { m_textFormat = format; }

    // Callback
    void SetOnChange(std::function<void(float)> callback) { m_onChange = callback; }

    // Hit testing and interaction
    bool HitTest(float x, float y) const;
    bool HitTestTrack(float x, float y) const;
    void OnMouseDown(float x, float y);
    void OnMouseMove(float x, float y);
    void OnMouseUp();
    bool IsDragging() const { return m_dragging; }

    // Colors
    void SetTrackColor(float r, float g, float b, float a = 1.0f);
    void SetFillColor(float r, float g, float b, float a = 1.0f);
    void SetHandleColor(float r, float g, float b, float a = 1.0f);
    void SetTextColor(float r, float g, float b, float a = 1.0f);

    // Dimensions
    void SetTrackHeight(float height) { m_trackHeight = height; }
    void SetHandleRadius(float radius) { m_handleRadius = radius; }
    void SetLabelWidth(float width) { m_labelWidth = width; }

private:
    float ValueToPosition(float value) const;
    float PositionToValue(float x) const;
    void NotifyChange();

    std::wstring m_label;
    float m_value = 0.5f;
    float m_min = 0.0f;
    float m_max = 1.0f;
    float m_step = 0.0f;  // 0 = continuous
    bool m_showValue = true;
    std::wstring m_valueFormat = L"%.2f";

    IDWriteTextFormat* m_textFormat = nullptr;
    std::function<void(float)> m_onChange;

    bool m_dragging = false;

    // Dimensions
    float m_trackHeight = 6.0f;
    float m_handleRadius = 8.0f;
    float m_labelWidth = 150.0f;
    float m_valueWidth = 50.0f;

    // Colors
    float m_trackColor[4] = { 0.2f, 0.2f, 0.25f, 1.0f };
    float m_fillColor[4] = { 0.4f, 0.7f, 1.0f, 1.0f };
    float m_handleColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float m_textColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    // Animation
    float m_hoverProgress = 0.0f;
};
