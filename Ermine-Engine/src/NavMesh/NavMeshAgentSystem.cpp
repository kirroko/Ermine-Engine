/* Start Header ************************************************************************/
/*!
\file       NavMeshAgentSystem.cpp
\author     LEE Wen Jie, Brian, wenjiebrian.lee, 2301261, wenjiebrian.lee\@digipen.edu
\date       03/11/2025
\brief      This file defines the NavMeshAgentSystem class, which manages
            navigation agents that move across baked navigation meshes.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "NavMeshAgentSystem.h"
#include "Physics.h"

namespace Ermine
{
    bool RequestPathForAgent(EntityID agentEntity, const Ermine::Vec3& destination)
    {
        auto& ecs = ECS::GetInstance();
        if (!ecs.HasComponent<NavMeshAgent>(agentEntity) ||
            !ecs.HasComponent<Transform>(agentEntity))
            return false;

        auto& agent = ecs.GetComponent<NavMeshAgent>(agentEntity);
        auto& trans = ecs.GetComponent<Transform>(agentEntity);

        auto navAgentSys = ecs.GetSystem<NavMeshAgentSystem>();
        if (!navAgentSys)
            return false;

        std::vector<Ermine::Vec3> path;
        if (!navAgentSys->FindPath(agentEntity, trans.position, destination, path))
            return false;

        agent.path = path;
        //for (size_t i = 0; i < path.size(); ++i)
        //{
        //    auto& p = path[i];
        //}
        agent.currentCorner = 0;
        agent.hasPath = true;
        return true;
    }

    void NavMeshAgentSystem::Init()
    {
    }

    void NavMeshAgentSystem::Update(float dt)
    {
        auto& ecs = ECS::GetInstance();
        auto renderer = ecs.GetSystem<graphics::Renderer>();

        for (auto e : m_Entities)
        {
            if (!ecs.HasComponent<NavMeshAgent>(e) || !ecs.HasComponent<Transform>(e))
                continue;

            if (ECS::GetInstance().HasComponent<ObjectMetaData>(e))
            {
                const auto& meta = ECS::GetInstance().GetComponent<ObjectMetaData>(e);
                if (!meta.selfActive)
                    continue;
            }

            auto& agent = ecs.GetComponent<NavMeshAgent>(e);
            auto& trans = ecs.GetComponent<Transform>(e);

            // skip if no path
            if (!agent.hasPath)
                continue;

            if (agent.path.empty())
            {
                agent.hasPath = false;
                continue;
            }

            // Move toward current path corner
            Vec3 target = agent.path[agent.currentCorner];
            Vec3 pos = trans.position;
            Vec3 dir = target - pos;
            dir.y = 0.0f;

            //float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
            float dist = std::sqrt(dir.x * dir.x + dir.z * dir.z);

            // if very close to the corner, go to next one
            if (dist < agent.stoppingDistance)
            {
                if (++agent.currentCorner >= agent.path.size())
                {
                    agent.hasPath = false;
                    continue;
                }
                else
                {
                    target = agent.path[agent.currentCorner];
                    dir = target - pos;
                    dist = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
                }
            }

            if (dist > 1e-3f)
            {
                dir.x /= dist;
                dir.y /= dist;
                dir.z /= dist;

                // Move
                float step = agent.speed * dt;
                if (step > dist) step = dist; // prevents overshoot/corner cutting

                pos += dir * step;

                // Clamp to navmesh as you already do
                EntityID navE = FindNearestNavMeshEntity(pos);
                if (navE != 0)
                {
                    float ext[3] = { agent.radius * 2.0f, agent.height * 0.5f + 0.5f, agent.radius * 2.0f };

                    Vec3 clamped;
                    auto navSys = ecs.GetSystem<NavMeshSystem>();
                    if (navSys && navSys->ClampToNavMesh(navE, pos, ext, clamped))
                    {
                        pos = clamped;

                        // Navmesh point is on the floor. Your physics body wants its CENTER.
                        pos.y += agent.centerYOffset;
                    }
                }

                // move using physics so collisions resolve
                auto phys = ecs.GetSystem<Physics>();
                if (phys && ecs.HasComponent<PhysicComponent>(e))
                {
                    phys->SetPosition(e, pos);

                    // If you want rotation too (optional), keep it synced:
                    // phys->SetRotation(e, trans.rotation);
                }
                else
                {
                    // fallback if no physics body
                    trans.position = pos;
                }

                //if (ecs.HasComponent<GlobalTransform>(e))
                //{
                //    auto& global = ecs.GetComponent<GlobalTransform>(e);
                //    global.worldMatrix = trans.GetLocalMatrix();
                //    global.isDirty = true;
                //}

                //if (ecs.HasComponent<HierarchyComponent>(e))
                //{
                //    auto& h = ecs.GetComponent<HierarchyComponent>(e);
                //    h.worldTransformDirty = true;
                //}
            }

#if defined(EE_EDITOR)

            if (agent.debugDrawPath && !agent.path.empty())
            {
                Vec3 color = { 0.0f, 0.0f, 1.0f };
                Vec3 offset = { 0.0f, 0.05f, 0.0f };

                for (size_t i = 1; i < agent.path.size(); ++i)
                {
                    const Vec3& a = agent.path[i - 1];
                    const Vec3& b = agent.path[i];
                    renderer->SubmitDebugLine(
                        glm::vec3(a.x + offset.x, a.y + offset.y, a.z + offset.z),
                        glm::vec3(b.x + offset.x, b.y + offset.y, b.z + offset.z),
                        glm::vec3(color.x, color.y, color.z)
                    );
                }
            }
#endif
        }
    }

    //bool NavMeshAgentSystem::FindPath(EntityID /*agentEntity*/, const Ermine::Vec3& startPos, const Ermine::Vec3& endPos, std::vector<Ermine::Vec3>& outPath)
    //{
    //    auto& ecs = ECS::GetInstance();

    //    EntityID nearestNav = FindNearestNavMeshEntity(startPos);
    //    if (nearestNav == 0 || !ecs.HasComponent<NavMeshComponent>(nearestNav))
    //        return false;

    //    auto navSystem = ecs.GetSystem<NavMeshSystem>();
    //    if (!navSystem)
    //        return false;

    //    outPath.clear();
    //    return navSystem->ComputeStraightPath(nearestNav, startPos, endPos, outPath);
    //}

    bool NavMeshAgentSystem::FindPath(EntityID agentEntity,
        const Ermine::Vec3& startPos,
        const Ermine::Vec3& endPos,
        std::vector<Ermine::Vec3>& outPath)
    {
        auto& ecs = ECS::GetInstance();

        if (!ecs.HasComponent<NavMeshAgent>(agentEntity))
            return false;

        const auto& agent = ecs.GetComponent<NavMeshAgent>(agentEntity);

        EntityID nearestNav = FindNearestNavMeshEntity(startPos);
        if (nearestNav == 0 || !ecs.HasComponent<NavMeshComponent>(nearestNav))
            return false;

        const auto& navComp = ecs.GetComponent<NavMeshComponent>(nearestNav);
        const float bakedR = navComp.bakedAgentRadius;
        const float bakedH = navComp.bakedAgentHeight;

        if (bakedR > 0.0f && agent.radius > bakedR + 1e-4f)
        {
            EE_CORE_ERROR("[NavMeshAgentSystem] Agent radius (%.3f) > navmesh baked radius (%.3f). Re-bake navmesh for this agent size.",
                agent.radius, bakedR);
            return false;
        }
        if (bakedH > 0.0f && agent.height > bakedH + 1e-4f)
        {
            EE_CORE_ERROR("[NavMeshAgentSystem] Agent height (%.3f) > navmesh baked height (%.3f). Re-bake navmesh for this agent size.",
                agent.height, bakedH);
            return false;
        }

        auto navSystem = ecs.GetSystem<NavMeshSystem>();
        if (!navSystem)
            return false;

        // extents based on the agent size (NOT 10,20,10)
        float extents[3] =
        {
            agent.radius * 2.0f,
            agent.height,
            agent.radius * 2.0f
        };

        outPath.clear();
        return navSystem->ComputeStraightPath(nearestNav, startPos, endPos, extents, outPath);
    }


    EntityID NavMeshAgentSystem::FindNearestNavMeshEntity(const Ermine::Vec3& pos)
    {
        auto& ecs = ECS::GetInstance();

        EntityID best = 0;
        float bestDistSq = FLT_MAX;

        for (EntityID e = 1; e < MAX_ENTITIES; ++e)
        {
            if (!ecs.IsEntityValid(e)) continue;
            if (!ecs.HasComponent<NavMeshComponent>(e)) continue;
            if (!ecs.HasComponent<Transform>(e)) continue;

            const auto& t = ecs.GetComponent<Transform>(e);
            const float dx = t.position.x - pos.x;
            const float dy = t.position.y - pos.y;
            const float dz = t.position.z - pos.z;
            const float d2 = dx * dx + dy * dy + dz * dz;

            if (d2 < bestDistSq)
            {
                bestDistSq = d2;
                best = e;
            }
        }
        return best;
    }
}
