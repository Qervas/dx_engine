#include "Window.h"
#include "Input.h"

Window::Window(const std::wstring& title, uint32_t width, uint32_t height)
    : m_title(title)
    , m_width(width)
    , m_height(height)
{
    m_hInstance = GetModuleHandle(nullptr);
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

    // Create window
    m_hwnd = CreateWindowEx(
        0,
        L"DXEngineWindowClass",
        m_title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        m_width, m_height,
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
            if (!Input::Get().IsMouseCaptured())
            {
                Input::Get().SetMouseCaptured(true);
            }
            return 0;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
