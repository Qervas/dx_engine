#include "MainMenu.h"
#include "../RHI/D2DInterop.h"

MainMenu::MainMenu()
{
}

MainMenu::~MainMenu()
{
    Shutdown();
}

bool MainMenu::Initialize(D2DInterop* d2dInterop, uint32_t screenWidth, uint32_t screenHeight)
{
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    IDWriteFactory* dwrite = d2dInterop->GetDWriteFactory();
    HRESULT hr;

    // Create title text format (large, bold)
    hr = dwrite->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        72.0f,
        L"en-us",
        &m_titleFormat
    );
    if (FAILED(hr)) return false;

    m_titleFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_titleFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Create subtitle text format
    hr = dwrite->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_ITALIC,
        DWRITE_FONT_STRETCH_NORMAL,
        20.0f,
        L"en-us",
        &m_subtitleFormat
    );
    if (FAILED(hr)) return false;

    m_subtitleFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_subtitleFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Create button text format
    hr = dwrite->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        24.0f,
        L"en-us",
        &m_buttonFormat
    );
    if (FAILED(hr)) return false;

    m_buttonFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_buttonFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    CreateButtons();
    LayoutButtons();

    return true;
}

void MainMenu::Shutdown()
{
    m_buttons.clear();
    m_buttonFormat.Reset();
    m_subtitleFormat.Reset();
    m_titleFormat.Reset();
}

void MainMenu::CreateButtons()
{
    m_buttons.clear();

    // Play button
    auto playBtn = std::make_unique<UIButton>(L"Play");
    playBtn->SetTextFormat(m_buttonFormat.Get());
    playBtn->SetOnClick([this]() { m_lastAction = MenuAction::Play; });
    m_buttons.push_back(std::move(playBtn));

    // Settings button
    auto settingsBtn = std::make_unique<UIButton>(L"Settings");
    settingsBtn->SetTextFormat(m_buttonFormat.Get());
    settingsBtn->SetOnClick([this]() { m_lastAction = MenuAction::Settings; });
    m_buttons.push_back(std::move(settingsBtn));

    // Exit button
    auto exitBtn = std::make_unique<UIButton>(L"Exit");
    exitBtn->SetTextFormat(m_buttonFormat.Get());
    exitBtn->SetOnClick([this]() { m_lastAction = MenuAction::Exit; });
    m_buttons.push_back(std::move(exitBtn));
}

void MainMenu::LayoutButtons()
{
    const float buttonWidth = 280.0f;
    const float buttonHeight = 55.0f;
    const float buttonSpacing = 15.0f;

    float totalHeight = static_cast<float>(m_buttons.size()) * buttonHeight +
                        static_cast<float>(m_buttons.size() - 1) * buttonSpacing;

    // Position buttons below center (leaving room for title)
    float startY = static_cast<float>(m_screenHeight) / 2.0f + 20.0f;
    float centerX = (static_cast<float>(m_screenWidth) - buttonWidth) / 2.0f;

    for (size_t i = 0; i < m_buttons.size(); ++i)
    {
        m_buttons[i]->SetPosition(centerX, startY + static_cast<float>(i) * (buttonHeight + buttonSpacing));
        m_buttons[i]->SetSize(buttonWidth, buttonHeight);
    }
}

void MainMenu::HandleInput(float mouseX, float mouseY, bool mouseClicked)
{
    for (auto& button : m_buttons)
    {
        bool hovered = button->HitTest(mouseX, mouseY);
        button->SetHovered(hovered);

        if (hovered && mouseClicked)
        {
            button->OnClick();
        }
    }
}

void MainMenu::Update(float deltaTime)
{
    for (auto& button : m_buttons)
    {
        button->Update(deltaTime);
    }
}

void MainMenu::Render(D2DInterop* d2dInterop)
{
    if (!d2dInterop)
        return;

    ID2D1DeviceContext* ctx = d2dInterop->GetD2DContext();

    // Draw title
    D2D1_RECT_F titleRect = D2D1::RectF(
        0.0f,
        static_cast<float>(m_screenHeight) * 0.15f,
        static_cast<float>(m_screenWidth),
        static_cast<float>(m_screenHeight) * 0.35f
    );

    d2dInterop->SetBrushColor(1.0f, 1.0f, 1.0f, 1.0f);
    ctx->DrawText(
        m_title.c_str(),
        static_cast<UINT32>(m_title.length()),
        m_titleFormat.Get(),
        titleRect,
        d2dInterop->GetBrush()
    );

    // Draw subtitle
    D2D1_RECT_F subtitleRect = D2D1::RectF(
        0.0f,
        static_cast<float>(m_screenHeight) * 0.32f,
        static_cast<float>(m_screenWidth),
        static_cast<float>(m_screenHeight) * 0.42f
    );

    d2dInterop->SetBrushColor(0.7f, 0.7f, 0.8f, 1.0f);
    ctx->DrawText(
        m_subtitle.c_str(),
        static_cast<UINT32>(m_subtitle.length()),
        m_subtitleFormat.Get(),
        subtitleRect,
        d2dInterop->GetBrush()
    );

    // Draw buttons
    for (auto& button : m_buttons)
    {
        button->Render(d2dInterop);
    }

    // Draw footer text
    std::wstring footerText = L"Press Play to begin";
    D2D1_RECT_F footerRect = D2D1::RectF(
        0.0f,
        static_cast<float>(m_screenHeight) - 60.0f,
        static_cast<float>(m_screenWidth),
        static_cast<float>(m_screenHeight) - 20.0f
    );

    d2dInterop->SetBrushColor(0.5f, 0.5f, 0.6f, 1.0f);
    ctx->DrawText(
        footerText.c_str(),
        static_cast<UINT32>(footerText.length()),
        m_subtitleFormat.Get(),
        footerRect,
        d2dInterop->GetBrush()
    );
}

void MainMenu::OnResize(uint32_t width, uint32_t height)
{
    m_screenWidth = width;
    m_screenHeight = height;
    LayoutButtons();
}
