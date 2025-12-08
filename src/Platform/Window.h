#pragma once

#include <windows.h>
#include <cstdint>
#include <string>
#include <functional>

// Menu command IDs
enum class MenuCommand : UINT
{
    // File menu
    FileExit = 1001,

    // View menu
    ViewWireframe = 2001,
    ViewDebugRendering = 2002,
    ViewFullscreen = 2003,

    // Settings menu
    SettingsPostProcess = 3001,
    SettingsBloom = 3002,
    SettingsSSAO = 3003,
    SettingsVSync = 3004
};

class Window
{
public:
    Window(const std::wstring& title, uint32_t width, uint32_t height);
    ~Window();

    bool Initialize();
    void Shutdown();
    bool ProcessMessages();

    // Accessors
    HWND GetHandle() const { return m_hwnd; }
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    bool ShouldClose() const { return m_shouldClose; }
    bool WasResized() const { return m_wasResized; }
    void ClearResizeFlag() { m_wasResized = false; }

    // Menu state
    void SetMenuChecked(MenuCommand cmd, bool checked);
    bool IsMenuChecked(MenuCommand cmd) const;

    // Mouse capture control - when disabled, clicks won't auto-capture the mouse
    void SetMouseCaptureEnabled(bool enabled) { m_mouseCaptureEnabled = enabled; }
    bool IsMouseCaptureEnabled() const { return m_mouseCaptureEnabled; }

    // Callbacks
    using ResizeCallback = std::function<void(uint32_t, uint32_t)>;
    using MenuCallback = std::function<void(MenuCommand)>;
    void SetResizeCallback(ResizeCallback callback) { m_resizeCallback = callback; }
    void SetMenuCallback(MenuCallback callback) { m_menuCallback = callback; }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void OnResize(uint32_t width, uint32_t height);
    void CreateMenuBar();
    void OnMenuCommand(UINT commandId);

    HWND m_hwnd = nullptr;
    HMENU m_menuBar = nullptr;
    HINSTANCE m_hInstance = nullptr;
    std::wstring m_title;
    uint32_t m_width;
    uint32_t m_height;
    bool m_shouldClose = false;
    bool m_wasResized = false;
    bool m_mouseCaptureEnabled = true;  // Set to false in menu mode
    ResizeCallback m_resizeCallback;
    MenuCallback m_menuCallback;
};
