#include "Window.h"
#include "Input.h"
#include <shellscalingapi.h>

#pragma comment(lib, "Shcore.lib")

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

    // Calculate window size to get desired client area (with menu)
    // Use fixed window style - no resize, no maximize (like Call of Duty)
    // Resolution changes only through settings menu
    constexpr DWORD windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    RECT windowRect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
    AdjustWindowRectEx(&windowRect, windowStyle, TRUE, 0);  // TRUE for menu
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    // Create window
    m_hwnd = CreateWindowEx(
        0,
        L"DXEngineWindowClass",
        m_title.c_str(),
        windowStyle,
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

    // Create and set menu bar
    CreateMenuBar();

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

    // Apply initial display mode if set (e.g., fullscreen from config)
    // Clear resize flag after this since swapchain isn't created yet
    if (m_initialDisplayMode != WindowDisplayMode::Windowed)
    {
        SetDisplayMode(m_initialDisplayMode);
        m_wasResized = false;  // Clear flag - swapchain will be created at this size
    }

    return true;
}

void Window::CreateMenuBar()
{
    m_menuBar = ::CreateMenu();

    // File menu
    HMENU fileMenu = CreatePopupMenu();
    AppendMenu(fileMenu, MF_STRING, static_cast<UINT>(MenuCommand::FileExit), L"E&xit\tAlt+F4");
    AppendMenu(m_menuBar, MF_POPUP, (UINT_PTR)fileMenu, L"&File");

    // View menu
    HMENU viewMenu = CreatePopupMenu();
    AppendMenu(viewMenu, MF_STRING, static_cast<UINT>(MenuCommand::ViewWireframe), L"&Wireframe\tF1");
    AppendMenu(viewMenu, MF_STRING | MF_CHECKED, static_cast<UINT>(MenuCommand::ViewDebugRendering), L"&Debug Rendering\tF2");
    AppendMenu(viewMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(viewMenu, MF_STRING, static_cast<UINT>(MenuCommand::ViewFullscreen), L"&Fullscreen\tF11");
    AppendMenu(m_menuBar, MF_POPUP, (UINT_PTR)viewMenu, L"&View");

    // Settings menu
    HMENU settingsMenu = CreatePopupMenu();
    AppendMenu(settingsMenu, MF_STRING | MF_CHECKED, static_cast<UINT>(MenuCommand::SettingsPostProcess), L"&Post-Processing\tF3");
    AppendMenu(settingsMenu, MF_STRING | MF_CHECKED, static_cast<UINT>(MenuCommand::SettingsBloom), L"&Bloom\tF4");
    AppendMenu(settingsMenu, MF_STRING | MF_CHECKED, static_cast<UINT>(MenuCommand::SettingsSSAO), L"&SSAO\tF5");
    AppendMenu(settingsMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(settingsMenu, MF_STRING | MF_CHECKED, static_cast<UINT>(MenuCommand::SettingsVSync), L"&VSync\tF6");
    AppendMenu(m_menuBar, MF_POPUP, (UINT_PTR)settingsMenu, L"&Settings");

    SetMenu(m_hwnd, m_menuBar);
}

void Window::SetMenuChecked(MenuCommand cmd, bool checked)
{
    if (m_menuBar)
    {
        CheckMenuItem(m_menuBar, static_cast<UINT>(cmd),
            MF_BYCOMMAND | (checked ? MF_CHECKED : MF_UNCHECKED));
    }
}

bool Window::IsMenuChecked(MenuCommand cmd) const
{
    if (m_menuBar)
    {
        UINT state = GetMenuState(m_menuBar, static_cast<UINT>(cmd), MF_BYCOMMAND);
        return (state & MF_CHECKED) != 0;
    }
    return false;
}

void Window::OnMenuCommand(UINT commandId)
{
    // Toggle checkable menu items
    MenuCommand cmd = static_cast<MenuCommand>(commandId);
    switch (cmd)
    {
    case MenuCommand::ViewWireframe:
    case MenuCommand::ViewDebugRendering:
    case MenuCommand::SettingsPostProcess:
    case MenuCommand::SettingsBloom:
    case MenuCommand::SettingsSSAO:
    case MenuCommand::SettingsVSync:
        SetMenuChecked(cmd, !IsMenuChecked(cmd));
        break;
    default:
        break;
    }

    // Notify callback
    if (m_menuCallback)
    {
        m_menuCallback(cmd);
    }
}

void Window::Shutdown()
{
    if (m_menuBar)
    {
        DestroyMenu(m_menuBar);
        m_menuBar = nullptr;
    }

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

        case WM_COMMAND:
            // Menu command
            if (HIWORD(wParam) == 0)  // Menu item
            {
                UINT commandId = LOWORD(wParam);
                if (commandId == static_cast<UINT>(MenuCommand::FileExit))
                {
                    window->m_shouldClose = true;
                    PostQuitMessage(0);
                }
                else
                {
                    window->OnMenuCommand(commandId);
                }
            }
            return 0;

        case WM_KEYDOWN:
            // Handle keyboard shortcuts
            switch (wParam)
            {
            case VK_ESCAPE:
                // Just release mouse capture - don't quit (let Application handle ESC)
                if (Input::Get().IsMouseCaptured())
                {
                    Input::Get().SetMouseCaptured(false);
                }
                break;
            case VK_F1:
                window->OnMenuCommand(static_cast<UINT>(MenuCommand::ViewWireframe));
                break;
            case VK_F2:
                window->OnMenuCommand(static_cast<UINT>(MenuCommand::ViewDebugRendering));
                break;
            case VK_F3:
                window->OnMenuCommand(static_cast<UINT>(MenuCommand::SettingsPostProcess));
                break;
            case VK_F4:
                window->OnMenuCommand(static_cast<UINT>(MenuCommand::SettingsBloom));
                break;
            case VK_F5:
                window->OnMenuCommand(static_cast<UINT>(MenuCommand::SettingsSSAO));
                break;
            case VK_F6:
                window->OnMenuCommand(static_cast<UINT>(MenuCommand::SettingsVSync));
                break;
            case VK_F11:
                window->OnMenuCommand(static_cast<UINT>(MenuCommand::ViewFullscreen));
                break;
            }
            return 0;

        case WM_LBUTTONDOWN:
            // Capture mouse on left click (for FPS controls) - only when enabled
            if (window->m_mouseCaptureEnabled && !Input::Get().IsMouseCaptured())
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

void Window::SetDisplayMode(WindowDisplayMode mode)
{
    if (mode == m_displayMode)
        return;

    // Fixed window style - no resize, no maximize
    constexpr DWORD windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    if (mode == WindowDisplayMode::Windowed)
    {
        // Restore windowed mode with fixed style
        SetWindowLongPtr(m_hwnd, GWL_STYLE, windowStyle);
        SetWindowPos(m_hwnd, HWND_NOTOPMOST,
            m_windowedRect.left, m_windowedRect.top,
            m_windowedRect.right - m_windowedRect.left,
            m_windowedRect.bottom - m_windowedRect.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        SetMenu(m_hwnd, m_menuBar);
    }
    else if (mode == WindowDisplayMode::FullscreenBorderless)
    {
        // Save current window position
        GetWindowRect(m_hwnd, &m_windowedRect);

        // Get monitor info for the monitor the window is on
        HMONITOR hMonitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfo(hMonitor, &mi);

        // Remove window decorations and menu
        SetMenu(m_hwnd, nullptr);
        SetWindowLongPtr(m_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);

        // Set window to cover the entire monitor
        SetWindowPos(m_hwnd, HWND_TOP,
            mi.rcMonitor.left, mi.rcMonitor.top,
            mi.rcMonitor.right - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }
    // FullscreenExclusive would require DXGI swapchain changes

    m_displayMode = mode;
}

void Window::SetResolution(uint32_t width, uint32_t height)
{
    if (m_displayMode == WindowDisplayMode::Windowed)
    {
        // Fixed window style - no resize, no maximize
        constexpr DWORD windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

        // Calculate window size to get desired client area
        RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
        AdjustWindowRectEx(&windowRect, windowStyle, TRUE, 0);

        int windowWidth = windowRect.right - windowRect.left;
        int windowHeight = windowRect.bottom - windowRect.top;

        // Center window on current monitor
        HMONITOR hMonitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfo(hMonitor, &mi);

        int x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - windowWidth) / 2;
        int y = mi.rcWork.top + (mi.rcWork.bottom - mi.rcWork.top - windowHeight) / 2;

        SetWindowPos(m_hwnd, nullptr, x, y, windowWidth, windowHeight, SWP_NOZORDER);
    }
}

std::vector<Resolution> Window::GetAvailableResolutions()
{
    std::vector<Resolution> resolutions;

    // Common resolutions
    resolutions.push_back({ 1280, 720 });
    resolutions.push_back({ 1366, 768 });
    resolutions.push_back({ 1600, 900 });
    resolutions.push_back({ 1920, 1080 });
    resolutions.push_back({ 2560, 1440 });
    resolutions.push_back({ 3840, 2160 });

    return resolutions;
}
