#pragma once

#include "UIButton.h"
#include "../Core/Types.h"
#include <vector>
#include <memory>
#include <dwrite.h>

class D2DInterop;

enum class MenuAction
{
    None,
    Play,
    Settings,
    Exit
};

class MainMenu
{
public:
    MainMenu();
    ~MainMenu();

    bool Initialize(D2DInterop* d2dInterop, uint32_t screenWidth, uint32_t screenHeight);
    void Shutdown();

    void Update(float deltaTime);
    void Render(D2DInterop* d2dInterop);

    MenuAction GetLastAction() const { return m_lastAction; }
    void ClearAction() { m_lastAction = MenuAction::None; }

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
    ComPtr<IDWriteTextFormat> m_subtitleFormat;

    // UI Elements
    std::vector<std::unique_ptr<UIButton>> m_buttons;

    // State
    MenuAction m_lastAction = MenuAction::None;

    // Title
    std::wstring m_title = L"DX12 Engine";
    std::wstring m_subtitle = L"DirectX 12 Rendering Demo";
};
