/* Start Header ************************************************************************/
/*!
\file       VideoImGUI.h
\author     Ridhwan Afandi, mohamedridhwan.b, 2301367, mohamedridhwan.b\@digipen.edu
\date       01/29/2026
\brief      ImGui window for video playback controls and render settings.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once

#include "PreCompile.h"
#include "ImGuiUIWindow.h"

namespace Ermine
{
    class VideoImGUI : public ImGUIWindow
    {
    public:
        VideoImGUI();
        void Update() override;
        void Render() override;

    private:
        bool m_showWindow = true;

        char m_videoName[128] = "intro";
        char m_videoPath[260] = "../Resources/Videos/intro.mpeg";
        bool m_loop = false;
        int m_fitModeIndex = 0;
    };
}
