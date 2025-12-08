#pragma once

enum class AppState
{
    MainMenu,   // Show main menu, no game resources loaded
    Loading,    // Transitioning, loading game resources
    InGame,     // Full game loop running
    Paused      // In-game pause overlay
};
