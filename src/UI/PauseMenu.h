#pragma once

#include "UIButton.h"
#include "../Core/Types.h"
#include <vector>
#include <memory>
#include <dwrite.h>

class D2DInterop;

enum class PauseAction
{
    None,
    Resume,
    Settings,
    MainMenu,
    Exit
};

class PauseMenu
{
public:
    PauseMenu();
    ~PauseMenu();

    bool Initialize(D2DInterop* d2dInterop, uint32_t screenWidth, uint32_t screenHeight);
    void Shutdown();

    void Update(float deltaTime);
    void Render(D2DInterop* d2dInterop);

    PauseAction GetLastAction() const { return m_lastAction; }
    void ClearAction() { m_lastAction = PauseAction::None; }

    void OnResize(uint32_t width, uint32_t height);

    // Call this each frame with mouse position and click state
    void HandleInput(float mouseX, float mouseY, bool mouseClicked);

private:
    void CreateButtons();
    void LayoutButtons();

    uint32_t m_screenWidth = 0;
    uint32_t m_screenHeight = 0;

    // DirectWrite text formats
    ComPtr<IDWriteTextFormat> m_titleFormat;
    ComPtr<IDWriteTextFormat> m_buttonFormat;

    // UI Elements
    std::vector<std::unique_ptr<UIButton>> m_buttons;

    // State
    PauseAction m_lastAction = PauseAction::None;

    // Title
    std::wstring m_title = L"Paused";
};
