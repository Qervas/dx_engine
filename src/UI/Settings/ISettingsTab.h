#pragma once

#include "../UIElement.h"
#include "../../Core/Types.h"
#include <dwrite.h>
#include <vector>
#include <string>

class D2DInterop;

// Base interface for settings tabs
class ISettingsTab
{
public:
    virtual ~ISettingsTab() = default;

    virtual bool Initialize(IDWriteTextFormat* labelFormat) = 0;
    virtual void Shutdown() = 0;

    virtual void HandleInput(float mouseX, float mouseY, bool mouseDown, bool mouseClicked) = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render(D2DInterop* d2dInterop) = 0;

    virtual void Layout(float x, float y, float width, float rowHeight) = 0;

    virtual const wchar_t* GetTabName() const = 0;

    // Get all widgets for input handling
    virtual std::vector<UIElement*>& GetWidgets() = 0;
};
