/* Start Header ************************************************************************/
/*!
\file       FSMEditor.cpp
\author     LEE Wen Jie, Brian, wenjiebrian.lee, 2301261, wenjiebrian.lee\@digipen.edu
\date       06/10/2025
\brief      This file contains definitions for imgui window FSM editor.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "FSMEditor.h"

namespace Ermine
{
    void FSMEditorImGUI::InitializeDefaultNodes()
    {
        m_nodes = {
        {1, "Idle", &g_IdleState},
        {2, "Roam", &g_RoamState},
        {3, "Attack", &g_AttackState},
        {4, "Dead", &g_DeadState},
        };
        m_nextNodeId = 5;
    }

    void FSMEditorImGUI::Render()
    {
        if (!ImNodes::GetCurrentContext())
        {
            ImNodes::CreateContext();
            ImNodes::StyleColorsDark();
        }

        ImGui::Begin(Name().c_str());

        if (m_SelectedEntity == 0)
        {
            ImGui::Text("No entity selected.");
            ImGui::End();
            return;
        }

        auto& fsm = ECS::GetInstance().GetComponent<StateMachine>(m_SelectedEntity);

        // Initialize if empty
        if (m_nodes.empty())
            InitializeDefaultNodes();

        // (Optional) sync m_links from fsm.transitions
        m_links.clear();
        for (auto& [fromState, toState] : fsm.transitions)
        {
            int fromId = -1, toId = -1;
            for (auto& node : m_nodes)
            {
                if (node.statePtr == fromState) fromId = node.id;
                if (node.statePtr == toState)   toId = node.id;
            }
            if (fromId != -1 && toId != -1)
                m_links.emplace_back(fromId, toId);
        }

        // Draw editor
        ImNodes::BeginNodeEditor();
        for (auto& node : m_nodes)
        {
            ImNodes::BeginNode(node.id);
            ImNodes::BeginNodeTitleBar();
            ImGui::TextUnformatted(node.name.c_str());
            ImNodes::EndNodeTitleBar();

            ImNodes::BeginOutputAttribute(node.id * 10);
            ImGui::Text("In");
            ImNodes::EndOutputAttribute();

            ImNodes::BeginInputAttribute(node.id * 10 + 1);
            ImGui::Text("Out");
            ImNodes::EndInputAttribute();

            ImNodes::EndNode();
        }

        int linkId = 1;
        for (auto& link : m_links)
            ImNodes::Link(linkId++, link.first * 10, link.second * 10 + 1);
        ImNodes::EndNodeEditor();

        // Handle new link
        int startAttr, endAttr;
        if (ImNodes::IsLinkCreated(&startAttr, &endAttr))
        {
            int fromId = startAttr / 10;
            int toId = (endAttr - 1) / 10;

            State* fromState = nullptr;
            State* toState = nullptr;
            for (auto& node : m_nodes)
            {
                if (node.id == fromId) fromState = node.statePtr;
                if (node.id == toId)   toState = node.statePtr;
            }

            if (fromState && toState)
            {
                m_links.emplace_back(fromId, toId);
                fsm.transitions[fromState] = toState;
            }
        }

        ImGui::End();
    }
}