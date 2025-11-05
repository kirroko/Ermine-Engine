#pragma once
#include "Entity.h"
#include "ECS.h"
#include "Components.h"

struct rcContext; struct rcHeightfield; struct rcCompactHeightfield;
struct rcContourSet; struct rcPolyMesh; struct rcPolyMeshDetail;
struct duDebugDraw; struct dtNavMesh; struct dtNavMeshQuery;

namespace Ermine {

    class NavMeshSystem : public System
    {
    public:
        void Init();
        void Shutdown();
        bool BakeTopOfCube(EntityID e);
        void DebugDraw();
        void DebugHighLight();

        bool ComputeStraightPath(EntityID navEntity, const Vec3& start, const Vec3& end, std::vector<Vec3>& outPath);
        void RemoveEntity(EntityID e);

        void DestroyBuild(NavMeshComponent& c);
        void DestroyRuntime(NavMeshComponent& c);
    private:
        bool BuildFromTriangles(NavMeshComponent& c, const float* verts, int nverts, const int* tris, int ntris);
        duDebugDraw* m_dd = nullptr;
    };
}