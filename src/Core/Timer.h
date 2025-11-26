#pragma once

#include <windows.h>
#include <cstdint>

class Timer
{
public:
    Timer()
    {
        QueryPerformanceFrequency(&m_frequency);
        Reset();
    }

    void Reset()
    {
        QueryPerformanceCounter(&m_startTime);
        m_lastTime = m_startTime;
        m_deltaTime = 0.0f;
        m_totalTime = 0.0f;
        m_frameCount = 0;
        m_fps = 0.0f;
        m_fpsAccumulator = 0.0f;
        m_fpsFrameCount = 0;
    }

    void Tick()
    {
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);

        // Calculate delta time
        m_deltaTime = static_cast<float>(currentTime.QuadPart - m_lastTime.QuadPart) /
                      static_cast<float>(m_frequency.QuadPart);

        // Clamp delta time to prevent huge jumps (e.g., after breakpoint)
        if (m_deltaTime > 0.25f)
        {
            m_deltaTime = 0.25f;
        }

        m_lastTime = currentTime;

        // Calculate total time
        m_totalTime = static_cast<float>(currentTime.QuadPart - m_startTime.QuadPart) /
                      static_cast<float>(m_frequency.QuadPart);

        m_frameCount++;

        // Calculate FPS (updated once per second)
        m_fpsAccumulator += m_deltaTime;
        m_fpsFrameCount++;

        if (m_fpsAccumulator >= 1.0f)
        {
            m_fps = static_cast<float>(m_fpsFrameCount) / m_fpsAccumulator;
            m_fpsAccumulator = 0.0f;
            m_fpsFrameCount = 0;
        }
    }

    // Time since last Tick() in seconds
    float GetDeltaTime() const { return m_deltaTime; }

    // Time since Reset() in seconds
    float GetTotalTime() const { return m_totalTime; }

    // Frame count since Reset()
    uint64_t GetFrameCount() const { return m_frameCount; }

    // Frames per second (updated once per second)
    float GetFPS() const { return m_fps; }

    // Get delta time in milliseconds
    float GetDeltaTimeMS() const { return m_deltaTime * 1000.0f; }

private:
    LARGE_INTEGER m_frequency;
    LARGE_INTEGER m_startTime;
    LARGE_INTEGER m_lastTime;

    float m_deltaTime = 0.0f;
    float m_totalTime = 0.0f;
    uint64_t m_frameCount = 0;

    float m_fps = 0.0f;
    float m_fpsAccumulator = 0.0f;
    int m_fpsFrameCount = 0;
};
