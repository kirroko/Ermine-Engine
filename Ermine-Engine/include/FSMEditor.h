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
#include "FSMNode.h"
#include <deque>

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

        std::deque<std::shared_ptr<ScriptNode>>& GetNodesForEntity(EntityID entity);
        const std::deque<std::shared_ptr<ScriptNode>>& GetNodesForEntity(EntityID entity) const;

    private:
        EntityID m_SelectedEntity = 0;
        int m_nextNodeId = 1;
        char m_newNodeName[64] = "";
        std::vector<int> nodesToDetachScript;
        std::vector<int> nodesToDelete;

        void CreateNode(const std::string& name);
    };
}
