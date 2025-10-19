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
    std::deque<std::shared_ptr<ScriptNode>>& FSMEditorImGUI::GetNodesForEntity(EntityID entity)
    {
        if (!ECS::GetInstance().HasComponent<StateMachine>(entity))
            ECS::GetInstance().AddComponent(entity, StateMachine());
        return ECS::GetInstance().GetComponent<StateMachine>(entity).m_Nodes;
    }

    const std::deque<std::shared_ptr<ScriptNode>>& FSMEditorImGUI::GetNodesForEntity(EntityID entity) const
    {
        static const std::deque<std::shared_ptr<ScriptNode>> empty;
        if (!ECS::GetInstance().HasComponent<StateMachine>(entity))
            return empty;
        return ECS::GetInstance().GetComponent<StateMachine>(entity).m_Nodes;
    }

    void FSMEditorImGUI::CreateNode(const std::string& name)
    {
        if (m_SelectedEntity == 0)
            return;

        if (!ECS::GetInstance().HasComponent<StateMachine>(m_SelectedEntity))
            ECS::GetInstance().AddComponent(m_SelectedEntity, StateMachine());

        auto& fsm = ECS::GetInstance().GetComponent<StateMachine>(m_SelectedEntity);

        auto node = std::make_shared<ScriptNode>();
        node->id = m_nextNodeId++;
        node->name = name;
        node->isAttached = false;
        node->scriptClassName = "";

        //m_entityNodes[m_SelectedEntity].push_back(std::move(node));
        fsm.m_Nodes.push_back(node);
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

        if (!ECS::GetInstance().HasComponent<StateMachine>(m_SelectedEntity))
        {
            ImGui::Text("Entity has no StateMachine component.");
            ImGui::End();
            return;
        }

        auto& fsm = ECS::GetInstance().GetComponent<StateMachine>(m_SelectedEntity);

        ImGui::InputText("New Node Name", m_newNodeName, IM_ARRAYSIZE(m_newNodeName));
        ImGui::SameLine();
        if (ImGui::Button("Add Node"))
        {
            if (strlen(m_newNodeName) > 0)
            {
                CreateNode(m_newNodeName);
                m_newNodeName[0] = '\0';
            }
        }
        ImGui::Separator();

        // Draw editor nodes and links
        ImNodes::BeginNodeEditor();

        for (auto& nodePtr : fsm.m_Nodes)
        {
            auto& snode = *nodePtr;
            ImNodes::BeginNode(snode.id);
            ImNodes::BeginNodeTitleBar();
            ImGui::TextUnformatted((snode.name + " (Script)").c_str());
            ImNodes::EndNodeTitleBar();

            // Attach script UI
            if (snode.isAttached)
                ImGui::Text("Script: %s", snode.scriptClassName.c_str());
            else if (ImGui::Button(("Attach Script##" + std::to_string(snode.id)).c_str()))
                ImGui::OpenPopup(("AttachScriptPopup" + std::to_string(snode.id)).c_str());

            if (ImGui::BeginPopup(("AttachScriptPopup" + std::to_string(snode.id)).c_str()))
            {
                static char scriptName[128] = "";
                ImGui::InputText("Class Name", scriptName, IM_ARRAYSIZE(scriptName));
                if (ImGui::Button("Confirm"))
                {
                    snode.isAttached = true;
                    snode.scriptClassName = scriptName;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImNodes::BeginInputAttribute(snode.id * 10 + 1);
            ImGui::Text("In");
            ImNodes::EndInputAttribute();

            ImNodes::BeginOutputAttribute(snode.id * 10);
            ImGui::Text("Out");
            ImNodes::EndOutputAttribute();

            ImNodes::EndNode();
        }

        int linkId = 1;
        for (auto& link : fsm.m_Links)
            ImNodes::Link(linkId++, link.first, link.second);

        ImNodes::EndNodeEditor();

        // Handle new link creation
        int startAttr, endAttr;
        if (ImNodes::IsLinkCreated(&startAttr, &endAttr))
        {
            auto is_output = [](int attr) { return (attr % 10) == 0; };
            auto is_input = [](int attr) { return (attr % 10) == 1; };

            int fromAttr = startAttr;
            int toAttr = endAttr;
            if (is_input(fromAttr) && is_output(toAttr))
                std::swap(fromAttr, toAttr);

            int fromId = fromAttr / 10;
            int toId = (toAttr - 1) / 10;

            ScriptNode* fromScriptNode = nullptr;
            ScriptNode* toScriptNode = nullptr;

            for (auto& sPtr : fsm.m_Nodes)
            {
                if (sPtr->id == fromId) fromScriptNode = sPtr.get();
                if (sPtr->id == toId)   toScriptNode = sPtr.get();
            }

            if (fromScriptNode && toScriptNode)
                fsm.scriptTransitions[fromScriptNode] = toScriptNode;

            fsm.m_Links.emplace_back(fromAttr, toAttr);
        }

        ImGui::End();
    }
}