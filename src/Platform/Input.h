#pragma once

#include <windows.h>
#include <cstdint>
#include <array>

// Key codes (matching Windows virtual key codes for common keys)
enum class Key : uint8_t
{
    // Letters
    A = 'A', B = 'B', C = 'C', D = 'D', E = 'E', F = 'F', G = 'G', H = 'H',
    I = 'I', J = 'J', K = 'K', L = 'L', M = 'M', N = 'N', O = 'O', P = 'P',
    Q = 'Q', R = 'R', S = 'S', T = 'T', U = 'U', V = 'V', W = 'W', X = 'X',
    Y = 'Y', Z = 'Z',

    // Numbers
    Num0 = '0', Num1 = '1', Num2 = '2', Num3 = '3', Num4 = '4',
    Num5 = '5', Num6 = '6', Num7 = '7', Num8 = '8', Num9 = '9',

    // Function keys
    F1 = VK_F1, F2 = VK_F2, F3 = VK_F3, F4 = VK_F4, F5 = VK_F5, F6 = VK_F6,
    F7 = VK_F7, F8 = VK_F8, F9 = VK_F9, F10 = VK_F10, F11 = VK_F11, F12 = VK_F12,

    // Arrow keys
    Left = VK_LEFT, Right = VK_RIGHT, Up = VK_UP, Down = VK_DOWN,

    // Modifiers
    Shift = VK_SHIFT, Control = VK_CONTROL, Alt = VK_MENU,
    LeftShift = VK_LSHIFT, RightShift = VK_RSHIFT,
    LeftControl = VK_LCONTROL, RightControl = VK_RCONTROL,

    // Common keys
    Space = VK_SPACE, Enter = VK_RETURN, Escape = VK_ESCAPE,
    Tab = VK_TAB, Backspace = VK_BACK, Delete = VK_DELETE,
    Insert = VK_INSERT, Home = VK_HOME, End = VK_END,
    PageUp = VK_PRIOR, PageDown = VK_NEXT,

    // Mouse buttons (for unified handling)
    MouseLeft = VK_LBUTTON,
    MouseRight = VK_RBUTTON,
    MouseMiddle = VK_MBUTTON,
};

enum class MouseButton : uint8_t
{
    Left = 0,
    Right = 1,
    Middle = 2,
    Count = 3
};

class Input
{
public:
    static Input& Get()
    {
        static Input instance;
        return instance;
    }

    // Call once per frame at the start
    void Update();

    // Keyboard state
    bool IsKeyDown(Key key) const;
    bool IsKeyPressed(Key key) const;   // Just pressed this frame
    bool IsKeyReleased(Key key) const;  // Just released this frame

    // Mouse state
    bool IsMouseButtonDown(MouseButton button) const;
    bool IsMouseButtonPressed(MouseButton button) const;
    bool IsMouseButtonReleased(MouseButton button) const;

    // Mouse position (screen coordinates)
    int GetMouseX() const { return m_mouseX; }
    int GetMouseY() const { return m_mouseY; }

    // Mouse delta (movement since last frame)
    int GetMouseDeltaX() const { return m_mouseDeltaX; }
    int GetMouseDeltaY() const { return m_mouseDeltaY; }

    // Mouse wheel delta
    int GetMouseWheelDelta() const { return m_mouseWheelDelta; }

    // Set window handle for mouse capture
    void SetWindowHandle(HWND hwnd) { m_hwnd = hwnd; }

    // Mouse capture (lock mouse to window center)
    void SetMouseCaptured(bool captured);
    bool IsMouseCaptured() const { return m_mouseCaptured; }

    // Process Windows messages (call from WndProc)
    void ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam);

private:
    Input();
    ~Input() = default;
    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    static constexpr int KEY_COUNT = 256;

    std::array<bool, KEY_COUNT> m_currentKeys = {};
    std::array<bool, KEY_COUNT> m_previousKeys = {};

    std::array<bool, 3> m_currentMouseButtons = {};
    std::array<bool, 3> m_previousMouseButtons = {};

    int m_mouseX = 0;
    int m_mouseY = 0;
    int m_lastMouseX = 0;
    int m_lastMouseY = 0;
    int m_mouseDeltaX = 0;
    int m_mouseDeltaY = 0;
    int m_mouseWheelDelta = 0;

    HWND m_hwnd = nullptr;
    bool m_mouseCaptured = false;
};
