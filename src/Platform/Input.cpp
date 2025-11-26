#include "Input.h"

Input::Input()
{
    m_currentKeys.fill(false);
    m_previousKeys.fill(false);
    m_currentMouseButtons.fill(false);
    m_previousMouseButtons.fill(false);
}

void Input::Update()
{
    // Store previous frame's state
    m_previousKeys = m_currentKeys;
    m_previousMouseButtons = m_currentMouseButtons;

    // Update keyboard state using GetAsyncKeyState
    for (int i = 0; i < KEY_COUNT; ++i)
    {
        m_currentKeys[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
    }

    // Update mouse button state
    m_currentMouseButtons[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    m_currentMouseButtons[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    m_currentMouseButtons[2] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;

    // Update mouse position
    POINT cursorPos;
    GetCursorPos(&cursorPos);

    if (m_hwnd)
    {
        ScreenToClient(m_hwnd, &cursorPos);
    }

    m_lastMouseX = m_mouseX;
    m_lastMouseY = m_mouseY;
    m_mouseX = cursorPos.x;
    m_mouseY = cursorPos.y;

    // Calculate delta
    if (m_mouseCaptured && m_hwnd)
    {
        // When captured, delta is relative to center of window
        RECT rect;
        GetClientRect(m_hwnd, &rect);
        int centerX = (rect.right - rect.left) / 2;
        int centerY = (rect.bottom - rect.top) / 2;

        m_mouseDeltaX = m_mouseX - centerX;
        m_mouseDeltaY = m_mouseY - centerY;

        // Reset cursor to center
        POINT center = { centerX, centerY };
        ClientToScreen(m_hwnd, &center);
        SetCursorPos(center.x, center.y);

        // Update our tracked position to center
        m_mouseX = centerX;
        m_mouseY = centerY;
    }
    else
    {
        m_mouseDeltaX = m_mouseX - m_lastMouseX;
        m_mouseDeltaY = m_mouseY - m_lastMouseY;
    }

    // Reset wheel delta (it's accumulated through messages)
    m_mouseWheelDelta = 0;
}

bool Input::IsKeyDown(Key key) const
{
    return m_currentKeys[static_cast<uint8_t>(key)];
}

bool Input::IsKeyPressed(Key key) const
{
    uint8_t k = static_cast<uint8_t>(key);
    return m_currentKeys[k] && !m_previousKeys[k];
}

bool Input::IsKeyReleased(Key key) const
{
    uint8_t k = static_cast<uint8_t>(key);
    return !m_currentKeys[k] && m_previousKeys[k];
}

bool Input::IsMouseButtonDown(MouseButton button) const
{
    return m_currentMouseButtons[static_cast<uint8_t>(button)];
}

bool Input::IsMouseButtonPressed(MouseButton button) const
{
    uint8_t b = static_cast<uint8_t>(button);
    return m_currentMouseButtons[b] && !m_previousMouseButtons[b];
}

bool Input::IsMouseButtonReleased(MouseButton button) const
{
    uint8_t b = static_cast<uint8_t>(button);
    return !m_currentMouseButtons[b] && m_previousMouseButtons[b];
}

void Input::SetMouseCaptured(bool captured)
{
    if (m_mouseCaptured == captured)
        return;

    m_mouseCaptured = captured;

    if (captured)
    {
        // Hide cursor and capture it
        ShowCursor(FALSE);
        if (m_hwnd)
        {
            SetCapture(m_hwnd);

            // Move cursor to center of window
            RECT rect;
            GetClientRect(m_hwnd, &rect);
            POINT center = { (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2 };
            ClientToScreen(m_hwnd, &center);
            SetCursorPos(center.x, center.y);
        }
    }
    else
    {
        // Show cursor and release capture
        ShowCursor(TRUE);
        ReleaseCapture();
    }
}

void Input::ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_MOUSEWHEEL:
        m_mouseWheelDelta += GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
        break;

    case WM_MOUSEMOVE:
        // Position is handled in Update() for consistency
        break;

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
        // These are handled via GetAsyncKeyState in Update()
        break;
    }
}
