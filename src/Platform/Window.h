#pragma once

#include <windows.h>
#include <cstdint>
#include <string>

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

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND m_hwnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    std::wstring m_title;
    uint32_t m_width;
    uint32_t m_height;
    bool m_shouldClose = false;
};
