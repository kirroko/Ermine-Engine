/* Start Header ************************************************************************/
/*!
\file       FrameController.h
\author     Wong Jun Yu, Kean, keanwng\@gmail.com
\date       09/03/2025
\brief      This file contains the declaration of the FrameController system.
            This file is used to control the frame rate of the game.
Copyright (C) 2025 TwoJumpingRabbits
*/
/* End Header **************************************************************************/

#pragma once
#include "PreCompile.h"

namespace Ermine
{
    class EE_API FrameController
    {
        float m_targetDeltaTime;
        float m_fixedDeltaTime;
        float m_deltaTime;
        float m_accumulator;

        int m_frameCount;
        float m_fpsTimer;
        float m_FPS;

        double m_last_frame_time;
    public:
        FrameController(float targetFPS = 60.0f, float fixedFPS = 60.0f);

        /**
         * @brief Call this at the beginning of each frame to calculate the delta time.
         */
        void BeginFrame();

        /**
         * @brief Check if a fixed update step is needed
         * @return true if a fixed update step is needed
         */
        bool ShouldUpdateFixed();

        /**
         * @brief Get the delta time
         * @return delta time
         */
        float GetDeltaTime() const;
        /**
         * @brief Get the fixed delta time
         * @return fixed delta time 
         */
        float GetFixedDeltaTime() const;
        /**
         * @brief Get remaining time ratio for interpolation
         * @return interpolation alpha
         */
        float GetInterpolationAlpha() const;

        /**
         * @brief Get the current FPS
         * @return current FPS
         */
        float GetFPS() const;
    };
}
