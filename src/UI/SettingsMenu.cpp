#include "SettingsMenu.h"
#include "../RHI/D2DInterop.h"

SettingsMenu::SettingsMenu()
{
}

SettingsMenu::~SettingsMenu()
{
    Shutdown();
}

bool SettingsMenu::Initialize(D2DInterop* d2dInterop, uint32_t screenWidth, uint32_t screenHeight)
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
        36.0f,
        L"en-us",
        &m_titleFormat
    );
    if (FAILED(hr)) return false;
    m_titleFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_titleFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Create tab text format
    hr = dwrite->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        18.0f,
        L"en-us",
        &m_tabFormat
    );
    if (FAILED(hr)) return false;
    m_tabFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_tabFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Create label text format
    hr = dwrite->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        16.0f,
        L"en-us",
        &m_labelFormat
    );
    if (FAILED(hr)) return false;
    m_labelFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    m_labelFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    CreateUI();
    LayoutUI();

    return true;
}

void SettingsMenu::Shutdown()
{
    m_tabs.clear();
    m_tabButtons.clear();
    m_backButton.reset();

    m_graphicsTab.reset();
    m_displayTab.reset();

    m_titleFormat.Reset();
    m_tabFormat.Reset();
    m_labelFormat.Reset();
}

void SettingsMenu::CreateUI()
{
    // Back button
    m_backButton = std::make_unique<UIButton>(L"\u2190 Back");
    m_backButton->SetTextFormat(m_tabFormat.Get());
    m_backButton->SetNormalColor(0.2f, 0.2f, 0.25f, 1.0f);
    m_backButton->SetHoverColor(0.3f, 0.3f, 0.35f, 1.0f);
    m_backButton->SetOnClick([this]() { m_lastAction = SettingsAction::Back; });

    // Create settings tabs (submodules)
    m_graphicsTab = std::make_unique<GraphicsSettingsTab>();
    m_graphicsTab->Initialize(m_labelFormat.Get());

    m_displayTab = std::make_unique<DisplaySettingsTab>();
    m_displayTab->Initialize(m_labelFormat.Get());

    // Build tabs list
    m_tabs = { m_graphicsTab.get(), m_displayTab.get() };

    // Create tab buttons dynamically based on tabs
    for (size_t i = 0; i < m_tabs.size(); ++i)
    {
        auto tabButton = std::make_unique<UIButton>(m_tabs[i]->GetTabName());
        tabButton->SetTextFormat(m_tabFormat.Get());

        // First tab is selected by default
        if (i == 0)
        {
            tabButton->SetNormalColor(0.3f, 0.5f, 0.7f, 1.0f);  // Selected
            tabButton->SetHoverColor(0.35f, 0.55f, 0.75f, 1.0f);
        }
        else
        {
            tabButton->SetNormalColor(0.2f, 0.2f, 0.25f, 1.0f);  // Unselected
            tabButton->SetHoverColor(0.3f, 0.3f, 0.35f, 1.0f);
        }

        size_t tabIndex = i;
        tabButton->SetOnClick([this, tabIndex]() { SwitchTab(tabIndex); });

        m_tabButtons.push_back(std::move(tabButton));
    }
}

void SettingsMenu::LayoutUI()
{
    const float panelWidth = 700.0f;
    const float panelHeight = 550.0f;
    float panelX = (static_cast<float>(m_screenWidth) - panelWidth) / 2.0f;
    float panelY = (static_cast<float>(m_screenHeight) - panelHeight) / 2.0f;

    const float sidebarWidth = 150.0f;
    const float contentX = panelX + sidebarWidth + 20.0f;
    const float contentWidth = panelWidth - sidebarWidth - 40.0f;

    // Back button (top left)
    m_backButton->SetPosition(panelX + 10.0f, panelY + 10.0f);
    m_backButton->SetSize(100.0f, 35.0f);

    // Tab buttons (left sidebar)
    float tabY = panelY + 80.0f;
    for (size_t i = 0; i < m_tabButtons.size(); ++i)
    {
        m_tabButtons[i]->SetPosition(panelX + 10.0f, tabY + static_cast<float>(i) * 50.0f);
        m_tabButtons[i]->SetSize(sidebarWidth - 20.0f, 40.0f);
    }

    // Layout all tabs content
    const float rowHeight = 40.0f;
    const float startY = panelY + 70.0f;

    for (auto* tab : m_tabs)
    {
        tab->Layout(contentX, startY, contentWidth, rowHeight);
    }
}

void SettingsMenu::SwitchTab(size_t tabIndex)
{
    if (tabIndex >= m_tabs.size())
        return;

    m_currentTabIndex = tabIndex;

    // Update tab button colors
    for (size_t i = 0; i < m_tabButtons.size(); ++i)
    {
        if (i == tabIndex)
        {
            m_tabButtons[i]->SetNormalColor(0.3f, 0.5f, 0.7f, 1.0f);  // Selected
            m_tabButtons[i]->SetHoverColor(0.35f, 0.55f, 0.75f, 1.0f);
        }
        else
        {
            m_tabButtons[i]->SetNormalColor(0.2f, 0.2f, 0.25f, 1.0f);  // Unselected
            m_tabButtons[i]->SetHoverColor(0.3f, 0.3f, 0.35f, 1.0f);
        }
    }
}

void SettingsMenu::HandleInput(float mouseX, float mouseY, bool mouseDown, bool mouseClicked)
{
    // Handle back button
    bool backHovered = m_backButton->HitTest(mouseX, mouseY);
    m_backButton->SetHovered(backHovered);
    if (backHovered && mouseClicked)
    {
        m_backButton->OnClick();
        return;
    }

    // Handle tab buttons
    for (auto& tabButton : m_tabButtons)
    {
        bool tabHovered = tabButton->HitTest(mouseX, mouseY);
        tabButton->SetHovered(tabHovered);
        if (tabHovered && mouseClicked)
        {
            tabButton->OnClick();
            return;
        }
    }

    // Handle current tab's widgets
    if (m_currentTabIndex < m_tabs.size())
    {
        m_tabs[m_currentTabIndex]->HandleInput(mouseX, mouseY, mouseDown, mouseClicked);
    }
}

void SettingsMenu::Update(float deltaTime)
{
    m_backButton->Update(deltaTime);

    for (auto& tabButton : m_tabButtons)
    {
        tabButton->Update(deltaTime);
    }

    // Update current tab
    if (m_currentTabIndex < m_tabs.size())
    {
        m_tabs[m_currentTabIndex]->Update(deltaTime);
    }
}

void SettingsMenu::Render(D2DInterop* d2dInterop)
{
    if (!d2dInterop)
        return;

    // Panel dimensions
    const float panelWidth = 700.0f;
    const float panelHeight = 550.0f;
    float panelX = (static_cast<float>(m_screenWidth) - panelWidth) / 2.0f;
    float panelY = (static_cast<float>(m_screenHeight) - panelHeight) / 2.0f;

    // Draw dark overlay
    d2dInterop->SetBrushColor(0.0f, 0.0f, 0.0f, 0.8f);
    d2dInterop->FillRect(0.0f, 0.0f, static_cast<float>(m_screenWidth), static_cast<float>(m_screenHeight));

    // Draw main panel background
    d2dInterop->SetBrushColor(0.1f, 0.1f, 0.12f, 0.98f);
    d2dInterop->FillRoundedRect(panelX, panelY, panelWidth, panelHeight, 12.0f);

    // Draw sidebar
    const float sidebarWidth = 150.0f;
    d2dInterop->SetBrushColor(0.08f, 0.08f, 0.1f, 1.0f);
    d2dInterop->FillRoundedRect(panelX, panelY, sidebarWidth, panelHeight, 12.0f);

    // Draw title
    d2dInterop->SetBrushColor(1.0f, 1.0f, 1.0f, 1.0f);
    d2dInterop->DrawText(L"SETTINGS", m_titleFormat.Get(),
                  panelX + sidebarWidth, panelY + 10.0f,
                  panelWidth - sidebarWidth, 50.0f);

    // Draw back button
    m_backButton->Render(d2dInterop);

    // Draw tab buttons
    for (auto& tabButton : m_tabButtons)
    {
        tabButton->Render(d2dInterop);
    }

    // Draw current tab section header
    const float contentX = panelX + sidebarWidth + 20.0f;
    d2dInterop->SetBrushColor(0.7f, 0.7f, 0.7f, 1.0f);
    if (m_currentTabIndex < m_tabs.size())
    {
        d2dInterop->DrawText(m_tabs[m_currentTabIndex]->GetTabName(), m_tabFormat.Get(),
                      contentX, panelY + 55.0f, 200.0f, 20.0f);
    }

    // Render current tab content
    if (m_currentTabIndex < m_tabs.size())
    {
        m_tabs[m_currentTabIndex]->Render(d2dInterop);
    }
}

void SettingsMenu::OnResize(uint32_t width, uint32_t height)
{
    m_screenWidth = width;
    m_screenHeight = height;
    LayoutUI();
}

void SettingsMenu::SetCallbacks(const SettingsCallbacks& callbacks)
{
    m_graphicsTab->SetCallbacks(callbacks.graphics);
    m_displayTab->SetCallbacks(callbacks.display);
}

void SettingsMenu::SetValues(const SettingsValues& values)
{
    m_graphicsTab->SetValues(values.graphics);
    m_displayTab->SetValues(values.display);
}

SettingsValues SettingsMenu::GetValues() const
{
    SettingsValues values;
    values.graphics = m_graphicsTab->GetValues();
    values.display = m_displayTab->GetValues();
    return values;
}
