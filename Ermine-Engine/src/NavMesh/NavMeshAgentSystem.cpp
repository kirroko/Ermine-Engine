#include "PreCompile.h"
#include "NavMeshAgentSystem.h"

namespace Ermine
{
    // ------------------------------------------------------
    // Public convenience API
    // ------------------------------------------------------
    bool RequestPathForAgent(EntityID agentEntity, const Ermine::Vec3& destination)
    {
        auto& ecs = ECS::GetInstance();
        if (!ecs.HasComponent<NavMeshAgent>(agentEntity) ||
            !ecs.HasComponent<Transform>(agentEntity))
            return false;

        EE_CORE_INFO("[NavMeshAgentSystem] RequestPathForAgent called for entity %llu", agentEntity);

        auto& agent = ecs.GetComponent<NavMeshAgent>(agentEntity);
        auto& trans = ecs.GetComponent<Transform>(agentEntity);

        auto navAgentSys = ecs.GetSystem<NavMeshAgentSystem>();
        if (!navAgentSys)
            return false;

        std::vector<Ermine::Vec3> path;
        if (!navAgentSys->FindPath(agentEntity, trans.position, destination, path))
            return false;

        agent.path = path;
        EE_CORE_INFO("[NavMeshAgentSystem] Path assigned: %zu points", path.size());
        for (size_t i = 0; i < path.size(); ++i)
        {
            auto& p = path[i];
            EE_CORE_INFO("  [%zu] %.3f %.3f %.3f", i, p.x, p.y, p.z);
        }
        agent.currentCorner = 0;
        agent.hasPath = true;
        return true;
    }

    void NavMeshAgentSystem::Init()
    {
        EE_CORE_INFO("[NavMeshAgentSystem] Initialized");
    }

    void NavMeshAgentSystem::Update(float dt)
    {
        auto& ecs = ECS::GetInstance();
        auto renderer = ecs.GetSystem<graphics::Renderer>();

        for (auto e : m_Entities)
        {
            if (!ecs.HasComponent<NavMeshAgent>(e) || !ecs.HasComponent<Transform>(e))
                continue;

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

            float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);

            // if very close to the corner, go to next one
            if (dist < agent.stoppingDistance)
            {
                if (++agent.currentCorner >= agent.path.size())
                {
                    agent.hasPath = false; // Path complete
                    EE_CORE_INFO("[NavMeshAgentSystem] Entity %u reached destination", e);
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
                pos += dir * agent.speed * dt;
                trans.position = pos;

                if (ecs.HasComponent<GlobalTransform>(e))
                {
                    auto& global = ecs.GetComponent<GlobalTransform>(e);
                    global.worldMatrix = trans.GetLocalMatrix(); // build from updated Transform
                    global.isDirty = true; // mark as changed for renderer
                }

                if (ecs.HasComponent<HierarchyComponent>(e))
                {
                    auto& h = ecs.GetComponent<HierarchyComponent>(e);
                    h.worldTransformDirty = true;
                }
            }

            // Debug info
            EE_CORE_TRACE("[NavMeshAgentSystem] Entity %u -> (%.2f, %.2f, %.2f)", e, trans.position.x, trans.position.y, trans.position.z);

#if defined(EE_EDITOR)
            // Draw the current path if debugDrawPath is true
            if (agent.debugDrawPath && !agent.path.empty())
            {
                Vec3 color = { 0.0f, 0.4f, 1.0f };
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

    // ------------------------------------------------------
    // FindPath — delegates Detour work to NavMeshSystem
    // ------------------------------------------------------
    bool NavMeshAgentSystem::FindPath(EntityID /*agentEntity*/,
        const Ermine::Vec3& startPos,
        const Ermine::Vec3& endPos,
        std::vector<Ermine::Vec3>& outPath)
    {
        auto& ecs = ECS::GetInstance();

        EntityID nearestNav = FindNearestNavMeshEntity(startPos);
        if (nearestNav == 0 || !ecs.HasComponent<NavMeshComponent>(nearestNav))
            return false;

        auto navSystem = ecs.GetSystem<NavMeshSystem>();
        if (!navSystem)
            return false;

        EE_CORE_INFO("[NavMeshAgentSystem] FindPath start=(%.2f, %.2f, %.2f) end=(%.2f, %.2f, %.2f)",
            startPos.x, startPos.y, startPos.z, endPos.x, endPos.y, endPos.z);

        outPath.clear();
        // NEW: ask NavMeshSystem to compute straight path; it can
        // access its private runtime safely inside its TU.
        return navSystem->ComputeStraightPath(nearestNav, startPos, endPos, outPath);
    }

    // ------------------------------------------------------
    // FindNearestNavMeshEntity — scan valid entities that have NavMeshComponent
    // ------------------------------------------------------
    EntityID NavMeshAgentSystem::FindNearestNavMeshEntity(const Ermine::Vec3& pos)
    {
        auto& ecs = ECS::GetInstance();

        EntityID best = 0;
        float bestDistSq = FLT_MAX;

        // Brute force scan; consistent with your existing style (see Engine.cpp)
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
