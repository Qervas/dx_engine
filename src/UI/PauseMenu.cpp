#include "PauseMenu.h"
#include "../RHI/D2DInterop.h"

PauseMenu::PauseMenu()
{
}

PauseMenu::~PauseMenu()
{
    Shutdown();
}

bool PauseMenu::Initialize(D2DInterop* d2dInterop, uint32_t screenWidth, uint32_t screenHeight)
{
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;

    IDWriteFactory* dwrite = d2dInterop->GetDWriteFactory();
    HRESULT hr;

    // Create title text format
    hr = dwrite->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        48.0f,
        L"en-us",
        &m_titleFormat
    );
    if (FAILED(hr)) return false;

    m_titleFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_titleFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Create button text format
    hr = dwrite->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        22.0f,
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

void PauseMenu::Shutdown()
{
    m_buttons.clear();
    m_buttonFormat.Reset();
    m_titleFormat.Reset();
}

void PauseMenu::CreateButtons()
{
    m_buttons.clear();

    // Resume button
    auto resumeBtn = std::make_unique<UIButton>(L"Resume");
    resumeBtn->SetTextFormat(m_buttonFormat.Get());
    resumeBtn->SetOnClick([this]() { m_lastAction = PauseAction::Resume; });
    m_buttons.push_back(std::move(resumeBtn));

    // Settings button
    auto settingsBtn = std::make_unique<UIButton>(L"Settings");
    settingsBtn->SetTextFormat(m_buttonFormat.Get());
    settingsBtn->SetOnClick([this]() { m_lastAction = PauseAction::Settings; });
    m_buttons.push_back(std::move(settingsBtn));

    // Main Menu button
    auto menuBtn = std::make_unique<UIButton>(L"Main Menu");
    menuBtn->SetTextFormat(m_buttonFormat.Get());
    menuBtn->SetOnClick([this]() { m_lastAction = PauseAction::MainMenu; });
    m_buttons.push_back(std::move(menuBtn));

    // Exit button
    auto exitBtn = std::make_unique<UIButton>(L"Exit");
    exitBtn->SetTextFormat(m_buttonFormat.Get());
    exitBtn->SetOnClick([this]() { m_lastAction = PauseAction::Exit; });
    m_buttons.push_back(std::move(exitBtn));
}

void PauseMenu::LayoutButtons()
{
    const float buttonWidth = 240.0f;
    const float buttonHeight = 50.0f;
    const float buttonSpacing = 12.0f;

    float totalHeight = static_cast<float>(m_buttons.size()) * buttonHeight +
                        static_cast<float>(m_buttons.size() - 1) * buttonSpacing;

    // Center buttons vertically (with slight offset for title)
    float startY = (static_cast<float>(m_screenHeight) - totalHeight) / 2.0f + 30.0f;
    float centerX = (static_cast<float>(m_screenWidth) - buttonWidth) / 2.0f;

    for (size_t i = 0; i < m_buttons.size(); ++i)
    {
        m_buttons[i]->SetPosition(centerX, startY + static_cast<float>(i) * (buttonHeight + buttonSpacing));
        m_buttons[i]->SetSize(buttonWidth, buttonHeight);
    }
}

void PauseMenu::HandleInput(float mouseX, float mouseY, bool mouseClicked)
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

void PauseMenu::Update(float deltaTime)
{
    for (auto& button : m_buttons)
    {
        button->Update(deltaTime);
    }
}

void PauseMenu::Render(D2DInterop* d2dInterop)
{
    if (!d2dInterop)
        return;

    ID2D1DeviceContext* ctx = d2dInterop->GetD2DContext();

    // Draw semi-transparent dark overlay over the game
    d2dInterop->SetBrushColor(0.0f, 0.0f, 0.0f, 0.7f);
    d2dInterop->FillRect(0.0f, 0.0f, static_cast<float>(m_screenWidth), static_cast<float>(m_screenHeight));

    // Draw pause menu background panel
    const float panelWidth = 350.0f;
    const float panelHeight = 380.0f;
    float panelX = (static_cast<float>(m_screenWidth) - panelWidth) / 2.0f;
    float panelY = (static_cast<float>(m_screenHeight) - panelHeight) / 2.0f;

    d2dInterop->SetBrushColor(0.1f, 0.1f, 0.15f, 0.95f);
    d2dInterop->FillRoundedRect(panelX, panelY, panelWidth, panelHeight, 12.0f);

    // Draw title
    D2D1_RECT_F titleRect = D2D1::RectF(
        0.0f,
        panelY + 20.0f,
        static_cast<float>(m_screenWidth),
        panelY + 80.0f
    );

    d2dInterop->SetBrushColor(1.0f, 1.0f, 1.0f, 1.0f);
    ctx->DrawText(
        m_title.c_str(),
        static_cast<UINT32>(m_title.length()),
        m_titleFormat.Get(),
        titleRect,
        d2dInterop->GetBrush()
    );

    // Draw buttons
    for (auto& button : m_buttons)
    {
        button->Render(d2dInterop);
    }
}

void PauseMenu::OnResize(uint32_t width, uint32_t height)
{
    m_screenWidth = width;
    m_screenHeight = height;
    LayoutButtons();
}
