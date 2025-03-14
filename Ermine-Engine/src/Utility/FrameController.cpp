/* Start Header ************************************************************************/
/*!
\file       FrameController.cpp
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       09/03/2025
\brief      This file contains the declaration of the FrameController system.
            This file is used to control the frame rate of the game.
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/
#include "PreCompile.h"
#include "FrameController.h"

#include <iomanip>
#include <sstream>
#include <spdlog/spdlog.h>
#include <GLFW/glfw3.h>

#include "Logger.h"

using namespace Ermine;

FrameController::FrameController(float targetFPS, float fixedFPS) : m_targetDeltaTime(1.0f/ targetFPS), m_fixedDeltaTime(1.0f/ fixedFPS),
    m_deltaTime(0.0f), m_accumulator(0.0f), m_fpsTimer(0.0f), m_FPS(0.0f), m_frameCount(0)
{
    m_last_frame_time = glfwGetTime();
}

void FrameController::BeginFrame()
{
    auto currentTime = glfwGetTime();
    m_deltaTime = static_cast<float>(currentTime - m_last_frame_time);
    m_last_frame_time = currentTime;
    
    m_accumulator += m_deltaTime;
    
    // if (m_deltaTime < m_targetDeltaTime) // Sleep if we are too fast to save CPU
    // {
    //     float sleepTime = m_targetDeltaTime - m_deltaTime;
    //     std::this_thread::sleep_for(std::chrono::duration<float>(sleepTime));
    // }

    m_fpsTimer += m_deltaTime;
    ++m_frameCount;

    if (m_fpsTimer >= 1.0f)
    {
        m_FPS = static_cast<float>(m_frameCount) / m_fpsTimer;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << m_FPS;
        std::string fpsString = oss.str();
        
        EE_CORE_INFO("FPS: {0}", fpsString);

        m_fpsTimer = .0f;
        m_frameCount = 0; 
    }
}

bool FrameController::ShouldUpdateFixed()
{
    if (m_accumulator >= m_fixedDeltaTime)
    {
        m_accumulator -= m_fixedDeltaTime;
        return true;
    }
    return false;
}

float FrameController::GetDeltaTime() const
{
    return m_deltaTime;
}

float FrameController::GetFixedDeltaTime() const
{
    return m_fixedDeltaTime;
}

float FrameController::GetInterpolationAlpha() const
{
    return m_accumulator / m_fixedDeltaTime;
}

float FrameController::GetFPS() const
{
    return m_FPS;
}


