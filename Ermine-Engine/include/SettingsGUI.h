/* Start Header ************************************************************************/
/*!
\file       SettingsGUI.h
\author     WEE HONG RU Curtis, h.wee, 2301266, h.wee\@digipen.edu
\date       Sep 01, 2025
\brief      This file contains the implementation of the Inspector GUI window.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "PreCompile.h"
#include "ImguiUIWindow.h"

namespace Ermine
{
    class SettingsGUI : public ImGUIWindow
    {
    public:
        /**
         * @brief Constructor
         */
        SettingsGUI();

        /**
        * @brief Constructor
        * @param EntityID
        * @param Inspector name
        */
        SettingsGUI(std::string name = "Settings");

        /**
        * @brief Set bool
        * @param isOpen
        */
        static void SetSettingsOpen(bool isOpen);

        static bool GetSettingsOpen();

		static void SetFontSize(float fontSize, float baseFontSize);

        static float GetFontSizeS();

        static float GetBaseFontSize();

		static void SetMode(int mode);

		static int GetMode();

        /**
        * @brief Inherited from ImGUIWindow, render loop
        */
        void Render() override;  // defined in .cpp

    private:
        bool settingsIsOpen = false;

		//int s_mode = 0; // 0: Light, 1: Dark, 2: Pink, 3: Cyberpunk, 4: Overwatch(Dark), 5: Overwatch(Light)
    };
}
