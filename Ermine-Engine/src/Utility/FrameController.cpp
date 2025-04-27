/* Start Header ************************************************************************/
/*!
\file       FrameController.cpp
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       09/03/2025
\brief      This file contains the declaration of the FrameController system.
            This file is used to control the frame rate of the game.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
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

void FrameController::Init(float targetFPS, float fixedFPS)
{
    s_targetDeltaTime = 1.0f / targetFPS;
    s_fixedDeltaTime = 1.0f / fixedFPS;
    s_deltaTime = 0.0f;
    s_accumulator = 0.0f;
    s_framePacing = false;
    s_fpsTimer = 0.0f;
    s_FPS = 0.0f;
    s_frameCount = 0;
    s_last_frame_time = glfwGetTime();
    EE_CORE_INFO("FrameController initialized with targetFPS: {0}, fixedFPS: {1}", targetFPS, fixedFPS);
}

void FrameController::BeginFrame()
{
    auto currentTime = glfwGetTime();
    s_deltaTime = static_cast<float>(currentTime - s_last_frame_time);
    s_last_frame_time = currentTime;
    
    s_accumulator += s_deltaTime;
    
    if (s_framePacing && s_deltaTime < s_targetDeltaTime) // Sleep if we are too fast to save CPU
    {
        float sleepTime = s_targetDeltaTime - s_deltaTime;
        std::this_thread::sleep_for(std::chrono::duration<float>(sleepTime));
    }

    // Calculate FPS
    s_fpsTimer += s_deltaTime;
    ++s_frameCount;

    //if (s_fpsTimer >= 1.0f)
    //{
    //    s_FPS = static_cast<float>(s_frameCount) / s_fpsTimer;
    //    std::ostringstream oss;
    //    oss << std::fixed << std::setprecision(2) << s_FPS;
    //    std::string fpsString = oss.str();
    //    
    //    EE_CORE_INFO("FPS: {0}", fpsString);

    //    s_fpsTimer = .0f;
    //    s_frameCount = 0; 
    //}
}

bool FrameController::ShouldUpdateFixed()
{
    if (s_accumulator >= s_fixedDeltaTime)
    {
        s_accumulator -= s_fixedDeltaTime;
        return true;
    }
    return false;
}


