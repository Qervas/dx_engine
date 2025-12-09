#include "UIManager.h"
#include "../RHI/D2DInterop.h"

UIManager::UIManager()
{
}

UIManager::~UIManager()
{
    Shutdown();
}

bool UIManager::Initialize(D2DInterop* d2dInterop, uint32_t screenWidth, uint32_t screenHeight)
{
    // Initialize main menu
    m_mainMenu = std::make_unique<MainMenu>();
    if (!m_mainMenu->Initialize(d2dInterop, screenWidth, screenHeight))
    {
        return false;
    }

    // Initialize pause menu
    m_pauseMenu = std::make_unique<PauseMenu>();
    if (!m_pauseMenu->Initialize(d2dInterop, screenWidth, screenHeight))
    {
        return false;
    }

    // Initialize settings menu
    m_settingsMenu = std::make_unique<SettingsMenu>();
    if (!m_settingsMenu->Initialize(d2dInterop, screenWidth, screenHeight))
    {
        return false;
    }

    SetupMenuCallbacks();

    return true;
}

void UIManager::Shutdown()
{
    m_settingsMenu.reset();
    m_pauseMenu.reset();
    m_mainMenu.reset();
}

void UIManager::SetupMenuCallbacks()
{
    // Main menu doesn't need internal callbacks - we check actions directly
    // Same for pause menu
}

void UIManager::SetupSettingsCallbacks()
{
    SettingsCallbacks callbacks;

    callbacks.onPostProcessChanged = [this](bool v) {
        if (m_callbacks.onPostProcessChanged) m_callbacks.onPostProcessChanged(v);
    };
    callbacks.onBloomChanged = [this](bool v) {
        if (m_callbacks.onBloomChanged) m_callbacks.onBloomChanged(v);
    };
    callbacks.onBloomIntensityChanged = [this](float v) {
        if (m_callbacks.onBloomIntensity) m_callbacks.onBloomIntensity(v);
    };
    callbacks.onBloomThresholdChanged = [this](float v) {
        if (m_callbacks.onBloomThreshold) m_callbacks.onBloomThreshold(v);
    };
    callbacks.onToneMappingChanged = [this](int v) {
        if (m_callbacks.onToneMapping) m_callbacks.onToneMapping(v);
    };
    callbacks.onExposureChanged = [this](float v) {
        if (m_callbacks.onExposure) m_callbacks.onExposure(v);
    };
    callbacks.onGammaChanged = [this](float v) {
        if (m_callbacks.onGamma) m_callbacks.onGamma(v);
    };
    callbacks.onSSAOChanged = [this](bool v) {
        if (m_callbacks.onSSAOChanged) m_callbacks.onSSAOChanged(v);
    };
    callbacks.onSSAORadiusChanged = [this](float v) {
        if (m_callbacks.onSSAORadius) m_callbacks.onSSAORadius(v);
    };
    callbacks.onSSAOIntensityChanged = [this](float v) {
        if (m_callbacks.onSSAOIntensity) m_callbacks.onSSAOIntensity(v);
    };
    callbacks.onVSyncChanged = [this](bool v) {
        if (m_callbacks.onVSync) m_callbacks.onVSync(v);
    };
    callbacks.onWireframeChanged = [this](bool v) {
        if (m_callbacks.onWireframe) m_callbacks.onWireframe(v);
    };
    callbacks.onDebugRenderingChanged = [this](bool v) {
        if (m_callbacks.onDebugRendering) m_callbacks.onDebugRendering(v);
    };

    m_settingsMenu->SetCallbacks(callbacks);
}

void UIManager::UpdateMainMenu(float deltaTime, float mouseX, float mouseY, bool mouseClicked)
{
    m_mainMenu->HandleInput(mouseX, mouseY, mouseClicked);
    m_mainMenu->Update(deltaTime);

    MenuAction action = m_mainMenu->GetLastAction();
    m_mainMenu->ClearAction();

    switch (action)
    {
    case MenuAction::Play:
        m_shouldStartGame = true;
        break;
    case MenuAction::Settings:
        m_shouldOpenSettings = true;
        break;
    case MenuAction::Exit:
        m_shouldExit = true;
        break;
    default:
        break;
    }
}

void UIManager::RenderMainMenu(D2DInterop* d2d)
{
    // Clear with dark background
    d2d->Clear(0.02f, 0.02f, 0.05f, 1.0f);
    m_mainMenu->Render(d2d);
}

void UIManager::UpdateSettings(float deltaTime, float mouseX, float mouseY, bool mouseDown, bool mouseClicked)
{
    m_settingsMenu->HandleInput(mouseX, mouseY, mouseDown, mouseClicked);
    m_settingsMenu->Update(deltaTime);

    SettingsAction action = m_settingsMenu->GetLastAction();
    m_settingsMenu->ClearAction();

    if (action == SettingsAction::Back)
    {
        m_shouldCloseSettings = true;
    }
}

void UIManager::RenderSettings(D2DInterop* d2d)
{
    // Clear with dark background
    d2d->Clear(0.02f, 0.02f, 0.05f, 1.0f);
    m_settingsMenu->Render(d2d);
}

void UIManager::UpdatePaused(float deltaTime, float mouseX, float mouseY, bool mouseClicked)
{
    m_pauseMenu->HandleInput(mouseX, mouseY, mouseClicked);
    m_pauseMenu->Update(deltaTime);

    PauseAction action = m_pauseMenu->GetLastAction();
    m_pauseMenu->ClearAction();

    switch (action)
    {
    case PauseAction::Resume:
        m_shouldResume = true;
        break;
    case PauseAction::Settings:
        m_shouldOpenSettings = true;
        break;
    case PauseAction::MainMenu:
        m_shouldReturnToMenu = true;
        break;
    case PauseAction::Exit:
        m_shouldExit = true;
        break;
    default:
        break;
    }
}

void UIManager::RenderPaused(D2DInterop* d2d)
{
    m_pauseMenu->Render(d2d);
}

void UIManager::ClearTransitionFlags()
{
    m_shouldStartGame = false;
    m_shouldOpenSettings = false;
    m_shouldCloseSettings = false;
    m_shouldResume = false;
    m_shouldReturnToMenu = false;
    m_shouldExit = false;
}

void UIManager::OnResize(uint32_t width, uint32_t height)
{
    if (m_mainMenu) m_mainMenu->OnResize(width, height);
    if (m_pauseMenu) m_pauseMenu->OnResize(width, height);
    if (m_settingsMenu) m_settingsMenu->OnResize(width, height);
}

void UIManager::SetSettingsCallbacks(const UISettingsCallbacks& callbacks)
{
    m_callbacks = callbacks;
    SetupSettingsCallbacks();
}

void UIManager::SyncSettingsValues(const UISettingsValues& values)
{
    SettingsValues settingsValues;
    settingsValues.postProcessEnabled = values.postProcessEnabled;
    settingsValues.bloomEnabled = values.bloomEnabled;
    settingsValues.bloomIntensity = values.bloomIntensity;
    settingsValues.bloomThreshold = values.bloomThreshold;
    settingsValues.toneMappingMode = values.toneMappingMode;
    settingsValues.exposure = values.exposure;
    settingsValues.gamma = values.gamma;
    settingsValues.ssaoEnabled = values.ssaoEnabled;
    settingsValues.ssaoRadius = values.ssaoRadius;
    settingsValues.ssaoIntensity = values.ssaoIntensity;
    settingsValues.vsyncEnabled = values.vsyncEnabled;
    settingsValues.wireframeEnabled = values.wireframeEnabled;
    settingsValues.debugRenderingEnabled = values.debugRenderingEnabled;
    m_settingsMenu->SetValues(settingsValues);
}
