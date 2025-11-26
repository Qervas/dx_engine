#include "Window.h"
#include "Input.h"
#include <imgui.h>
#include <shellscalingapi.h>

#pragma comment(lib, "Shcore.lib")

// Forward declare ImGui Win32 handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Window::Window(const std::wstring& title, uint32_t width, uint32_t height)
    : m_title(title)
    , m_width(width)
    , m_height(height)
{
    m_hInstance = GetModuleHandle(nullptr);

    // Enable DPI awareness (Per-Monitor DPI aware)
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
}

Window::~Window()
{
    Shutdown();
}

bool Window::Initialize()
{
    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = m_hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"DXEngineWindowClass";

    if (!RegisterClassEx(&wc))
    {
        MessageBox(nullptr, L"Failed to register window class", L"Error", MB_OK);
        return false;
    }

    // Calculate window size to get desired client area
    RECT windowRect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
    AdjustWindowRectEx(&windowRect, WS_OVERLAPPEDWINDOW, FALSE, 0);
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    // Create window
    m_hwnd = CreateWindowEx(
        0,
        L"DXEngineWindowClass",
        m_title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowWidth, windowHeight,
        nullptr,
        nullptr,
        m_hInstance,
        this  // Pass this pointer to use in WindowProc
    );

    if (!m_hwnd)
    {
        MessageBox(nullptr, L"Failed to create window", L"Error", MB_OK);
        return false;
    }

    // Get actual client area size (in case of DPI scaling)
    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);
    m_width = clientRect.right - clientRect.left;
    m_height = clientRect.bottom - clientRect.top;

    // Show window
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    // Initialize input system with window handle
    Input::Get().SetWindowHandle(m_hwnd);

    return true;
}

void Window::Shutdown()
{
    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }

    UnregisterClass(L"DXEngineWindowClass", m_hInstance);
}

bool Window::ProcessMessages()
{
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            m_shouldClose = true;
            return false;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return !m_shouldClose;
}

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Forward to ImGui first
    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
        return true;

    Window* window = nullptr;

    if (uMsg == WM_NCCREATE)
    {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        window = reinterpret_cast<Window*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    }
    else
    {
        window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    // Forward messages to input system
    Input::Get().ProcessMessage(uMsg, wParam, lParam);

    if (window)
    {
        switch (uMsg)
        {
        case WM_DESTROY:
            window->m_shouldClose = true;
            PostQuitMessage(0);
            return 0;

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE)
            {
                // Toggle mouse capture with Escape
                Input& input = Input::Get();
                if (input.IsMouseCaptured())
                {
                    input.SetMouseCaptured(false);
                }
                else
                {
                    // If not captured, close the window
                    window->m_shouldClose = true;
                    PostQuitMessage(0);
                }
            }
            return 0;

        case WM_LBUTTONDOWN:
            // Capture mouse on left click (for FPS controls)
            // But only if ImGui doesn't want the mouse
            if (!Input::Get().IsMouseCaptured() && !ImGui::GetIO().WantCaptureMouse)
            {
                Input::Get().SetMouseCaptured(true);
            }
            return 0;

        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED)
            {
                uint32_t newWidth = LOWORD(lParam);
                uint32_t newHeight = HIWORD(lParam);
                if (newWidth > 0 && newHeight > 0)
                {
                    window->OnResize(newWidth, newHeight);
                }
            }
            return 0;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void Window::OnResize(uint32_t width, uint32_t height)
{
    if (width == m_width && height == m_height)
        return;

    m_width = width;
    m_height = height;
    m_wasResized = true;

    if (m_resizeCallback)
    {
        m_resizeCallback(width, height);
    }
}
