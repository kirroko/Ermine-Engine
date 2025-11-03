#pragma once
#include "ECS.h"
#include "Components.h"
#include "NavMesh.h"
#include "Logger.h"
#include "Renderer.h"

#include <glm/glm.hpp>
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>

#include <vector>
#include <cfloat>

namespace Ermine
{
    // Convenience API callable from other systems / C# bindings
    bool RequestPathForAgent(EntityID agentEntity, const Ermine::Vec3& destination);

    class NavMeshAgentSystem : public System
    {
    public:
        void Init();
        void Update(float dt);

        // Computes a path for an agent (uses NavMeshSystem internally)
        bool FindPath(EntityID agentEntity,
            const Ermine::Vec3& startPos,
            const Ermine::Vec3& endPos,
            std::vector<Ermine::Vec3>& outPath);

    private:
        // Find the nearest entity that has a NavMeshComponent
        EntityID FindNearestNavMeshEntity(const Ermine::Vec3& pos);
    };
}