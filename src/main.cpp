#include "Core/Application.h"
#include <Windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    Application app;

    if (!app.Initialize())
    {
        MessageBox(nullptr, L"Failed to initialize application", L"Error", MB_OK);
        return 1;
    }

    app.Run();

    return 0;
}
