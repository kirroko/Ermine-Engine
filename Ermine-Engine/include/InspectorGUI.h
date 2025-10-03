/* Start Header ************************************************************************/
/*!
\file       InspectorGUI.h
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
#include "ECS.h"

namespace Ermine
{
    class InspectorGUI : public ImGUIWindow
    {
    public:
        InspectorGUI();
        InspectorGUI(EntityID entity, std::string name = "Inspector");

        void SetEntity(EntityID entity);
		EntityID GetEntity() const { return m_entity; }

        void Render() override;  // defined in .cpp

    private:
        EntityID m_entity;
    };
}
