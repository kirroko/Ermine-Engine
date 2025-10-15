/* Start Header ************************************************************************/
/*!
\file       FSMEditor.h
\author     LEE Wen Jie, Brian, wenjiebrian.lee, 2301261, wenjiebrian.lee\@digipen.edu
\date       06/10/2025
\brief      This file contains declarations for imgui window FSM editor.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "ImGuiUIWindow.h"
#include "FiniteStateMachine.h"
#include "ECS.h"
#include "imgui.h"
#include "imnodes.h"
#include "States.h"

namespace Ermine
{
    class FSMEditorImGUI : public ImGUIWindow
    {
    public:
        FSMEditorImGUI() : ImGUIWindow("FSM Editor") {}
        ~FSMEditorImGUI() override = default;

        void Update() override {}
        void Render() override;

        void SetSelectedEntity(EntityID entity) { m_SelectedEntity = entity; }

    private:
        EntityID m_SelectedEntity = 0;
        int m_nextNodeId = 1;

        struct FSMNode
        {
            int id;
            std::string name;
            State* statePtr;
        };

        std::vector<FSMNode> m_nodes;
        std::vector<std::pair<int, int>> m_links; // (fromId, toId)

        char m_newNodeName[64] = "";

        void InitializeDefaultNodes();
        void CreateNode(const std::string& name);
    };
}
