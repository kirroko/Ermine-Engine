#include "PreCompile.h"
#include "NavMesh.h"
#include "ECS.h"
#include "Components.h"

#include "Recast.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourNavMeshQuery.h"
#include "DebugDraw.h"
#include "DetourDebugDraw.h"
#include "RecastDebugDraw.h"
#include "EditorCamera.h"
#include "Renderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cfloat>

class DebugDrawGL : public duDebugDraw
{
public:
    DebugDrawGL() = default;
    void depthMask(bool state) override { glDepthMask(state ? GL_TRUE : GL_FALSE); }

    void begin(duDebugDrawPrimitives prim, float size = 1.0f) override
    {
        switch (prim)
        {
        case DU_DRAW_POINTS: glPointSize(size); glBegin(GL_POINTS); break;
        case DU_DRAW_LINES:  glLineWidth(size); glBegin(GL_LINES);  break;
        case DU_DRAW_TRIS:   glBegin(GL_TRIANGLES); break;
        case DU_DRAW_QUADS:  glBegin(GL_QUADS); break;
        }
    }

    void vertex(const float* pos, unsigned int color) override
    {
        unsigned char r = (color >> 24) & 0xFF;
        unsigned char g = (color >> 16) & 0xFF;
        unsigned char b = (color >> 8) & 0xFF;
        unsigned char a = (color) & 0xFF;
        glColor4ub(r, g, b, a);
        glVertex3f(pos[0], pos[1], pos[2]);
    }

    void vertex(const float x, const float y, const float z, unsigned int color) override
    {
        unsigned char r = (color >> 24) & 0xFF;
        unsigned char g = (color >> 16) & 0xFF;
        unsigned char b = (color >> 8) & 0xFF;
        unsigned char a = (color) & 0xFF;
        glColor4ub(r, g, b, a);
        glVertex3f(x, y, z);
    }

    void vertex(const float* pos, unsigned int color, const float* uv) override { vertex(pos, color); }
    void vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v) override { vertex(x, y, z, color); }
    void end() override { glEnd(); }
    void texture(bool state) override {}
};

struct Ermine::NavMeshComponent::BuildData
{
    rcContext* ctx = nullptr;
    rcHeightfield* hf = nullptr;
    rcCompactHeightfield* chf = nullptr;
    rcContourSet* cset = nullptr;
    rcPolyMesh* pmesh = nullptr;
    rcPolyMeshDetail* dmesh = nullptr;
};

struct Ermine::NavMeshComponent::Runtime
{
    dtNavMesh* nav = nullptr;
    dtNavMeshQuery* query = nullptr;
    unsigned char* navData = nullptr;
};

namespace Ermine {

    static void LogVec3(const char* name, const float* v)
    {
        //EE_CORE_INFO("  %s: %.3f %.3f %.3f", name, v[0], v[1], v[2]);
    }

    void NavMeshSystem::Init()
    {
        if (!m_dd) m_dd = new DebugDrawGL();
        EE_CORE_INFO("[NavMeshSystem] Initialized");
    }

    void NavMeshSystem::Shutdown()
    {
        //EE_CORE_INFO("[NavMeshSystem] Shutdown - freeing all navmesh data");

        auto& ecs = ECS::GetInstance();
        for (EntityID e = 1; e <= MAX_ENTITIES; ++e)
        {
            if (!ecs.IsEntityValid(e)) continue;
            if (!ecs.HasComponent<NavMeshComponent>(e)) continue;

            auto& c = ecs.GetComponent<NavMeshComponent>(e);
            DestroyBuild(c);
            DestroyRuntime(c);
        }

        delete m_dd;
        m_dd = nullptr;
    }

    void NavMeshSystem::DestroyBuild(NavMeshComponent& c)
    {
        if (!c.build) return;
        //EE_CORE_INFO("[NavMeshSystem] DestroyBuild");

        if (c.build->dmesh) { rcFreePolyMeshDetail(c.build->dmesh); c.build->dmesh = nullptr; }
        if (c.build->pmesh) { rcFreePolyMesh(c.build->pmesh); c.build->pmesh = nullptr; }
        if (c.build->cset) { rcFreeContourSet(c.build->cset); c.build->cset = nullptr; }
        if (c.build->chf) { rcFreeCompactHeightfield(c.build->chf); c.build->chf = nullptr; }
        if (c.build->hf) { rcFreeHeightField(c.build->hf); c.build->hf = nullptr; }
        if (c.build->ctx) { delete c.build->ctx; c.build->ctx = nullptr; }

        delete c.build;
        c.build = nullptr;
    }

    void NavMeshSystem::DestroyRuntime(NavMeshComponent& c)
    {
        if (!c.runtime) return;

        if (c.runtime->query) { dtFree(c.runtime->query); c.runtime->query = nullptr; }
        if (c.runtime->nav) { dtFree(c.runtime->nav);   c.runtime->nav = nullptr; }

        if (c.runtime->navData)
        {
            dtFree(c.runtime->navData);
            c.runtime->navData = nullptr;
        }

        delete c.runtime;
        c.runtime = nullptr;
    }

    bool NavMeshSystem::BuildFromTriangles(NavMeshComponent& c,
        const float* verts, int nverts,
        const int* tris, int ntris)
    {
        EE_CORE_INFO("[NavMeshSystem] Begin BuildFromTriangles (verts=%d tris=%d)", nverts, ntris);
        DestroyBuild(c);
        DestroyRuntime(c);

        if (!verts || !tris || nverts < 3 || ntris < 1)
        {
            EE_CORE_WARN("[NavMeshSystem] Invalid input geometry");
            return false;
        }

        c.build = new NavMeshComponent::BuildData();
        c.build->ctx = new rcContext();

        float bmin[3] = { FLT_MAX, FLT_MAX, FLT_MAX };
        float bmax[3] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
        for (int i = 0; i < nverts; ++i)
        {
            const float* v = &verts[i * 3];
            bmin[0] = std::min(bmin[0], v[0]);
            bmin[1] = std::min(bmin[1], v[1]);
            bmin[2] = std::min(bmin[2], v[2]);
            bmax[0] = std::max(bmax[0], v[0]);
            bmax[1] = std::max(bmax[1], v[1]);
            bmax[2] = std::max(bmax[2], v[2]);
        }

        const float pad = c.cellSize * 2.0f;
        for (int i = 0; i < 3; ++i) { bmin[i] -= pad; bmax[i] += pad; }
        LogVec3("bmin", bmin);
        LogVec3("bmax", bmax);

        rcConfig cfg{};
        cfg.cs = c.cellSize;
        cfg.ch = c.cellHeight;
        cfg.walkableSlopeAngle = c.agentMaxSlope;
        cfg.walkableHeight = (int)std::ceil(c.agentHeight / cfg.ch);
        cfg.walkableClimb = (int)std::floor(c.agentMaxClimb / cfg.ch);
        cfg.walkableRadius = (int)std::ceil(c.agentRadius / cfg.cs);
        rcVcopy(cfg.bmin, bmin);
        rcVcopy(cfg.bmax, bmax);
        rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);
        cfg.width = std::max(cfg.width, 2);
        cfg.height = std::max(cfg.height, 2);
        cfg.maxEdgeLen = 12;
        cfg.maxSimplificationError = 1.3f;
        cfg.minRegionArea = (int)rcSqr(8);
        cfg.mergeRegionArea = (int)rcSqr(20);
        cfg.maxVertsPerPoly = 6;
        cfg.detailSampleDist = 6.0f;
        cfg.detailSampleMaxError = 1.0f;

       // EE_CORE_INFO("[NavMeshSystem] cfg width=%d height=%d", cfg.width, cfg.height);

        c.build->hf = rcAllocHeightfield();
        if (!c.build->hf)
        {
            EE_CORE_ERROR("[NavMeshSystem] Failed to allocate heightfield");
            return false;
        }

        if (!rcCreateHeightfield(c.build->ctx, *c.build->hf, cfg.width, cfg.height,
            cfg.bmin, cfg.bmax, cfg.cs, cfg.ch))
        {
            EE_CORE_ERROR("[NavMeshSystem] rcCreateHeightfield failed");
            DestroyBuild(c);
            return false;
        }

        std::vector<unsigned char> areas(ntris, RC_WALKABLE_AREA);
        if (!rcRasterizeTriangles(c.build->ctx, verts, nverts,
            tris, areas.data(), ntris,
            *c.build->hf, cfg.walkableClimb))
        {
            EE_CORE_ERROR("[NavMeshSystem] rcRasterizeTriangles failed");
            DestroyBuild(c);
            return false;
        }

        c.build->chf = rcAllocCompactHeightfield();
        rcBuildCompactHeightfield(c.build->ctx, cfg.walkableHeight, cfg.walkableClimb, *c.build->hf, *c.build->chf);
        rcBuildDistanceField(c.build->ctx, *c.build->chf);
        rcBuildRegions(c.build->ctx, *c.build->chf, 0, cfg.minRegionArea, cfg.mergeRegionArea);

        c.build->cset = rcAllocContourSet();
        rcBuildContours(c.build->ctx, *c.build->chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *c.build->cset);

        c.build->pmesh = rcAllocPolyMesh();
        rcBuildPolyMesh(c.build->ctx, *c.build->cset, cfg.maxVertsPerPoly, *c.build->pmesh);

        c.build->dmesh = rcAllocPolyMeshDetail();
        rcBuildPolyMeshDetail(c.build->ctx, *c.build->pmesh, *c.build->chf,
            cfg.detailSampleDist, cfg.detailSampleMaxError, *c.build->dmesh);

        //EE_CORE_INFO("[NavMeshSystem] PolyMesh: verts=%d polys=%d", c.build->pmesh->nverts, c.build->pmesh->npolys);
        //EE_CORE_INFO("[NavMesh] PolyMesh data check: nverts=%d npolys=%d nvp=%d", c.build->pmesh->nverts, c.build->pmesh->npolys, c.build->pmesh->nvp);

        if (c.build->pmesh->nverts == 0 || c.build->pmesh->npolys == 0)
        {
            EE_CORE_ERROR("[NavMesh] Empty pmesh! Possible over-eroded geometry or agent too tall.");
            DestroyBuild(c);
            return false;
        }

        dtNavMeshCreateParams params{};
        params.verts = c.build->pmesh->verts;
        params.vertCount = c.build->pmesh->nverts;
        params.polys = c.build->pmesh->polys;
        params.polyAreas = c.build->pmesh->areas;
        //params.polyFlags = c.build->pmesh->flags;

        static const unsigned short WALKABLE = 0x01;
        std::vector<unsigned short> polyFlagsTemp(c.build->pmesh->npolys, WALKABLE);
        params.polyFlags = polyFlagsTemp.data();

        params.polyCount = c.build->pmesh->npolys;
        params.nvp = c.build->pmesh->nvp;
        params.detailMeshes = c.build->dmesh->meshes;
        params.detailVerts = c.build->dmesh->verts;
        params.detailVertsCount = c.build->dmesh->nverts;
        params.detailTris = c.build->dmesh->tris;
        params.detailTriCount = c.build->dmesh->ntris;
        rcVcopy(params.bmin, c.build->pmesh->bmin);
        rcVcopy(params.bmax, c.build->pmesh->bmax);
        params.cs = cfg.cs;
        params.ch = cfg.ch;
        params.walkableHeight = c.agentHeight;
        params.walkableRadius = c.agentRadius;
        params.walkableClimb = c.agentMaxClimb;
        params.buildBvTree = true;

        unsigned char* navData = nullptr;
        int navDataSize = 0;

        for (int i = 0; i < 3; ++i)
        {
            if (params.bmax[i] - params.bmin[i] < 0.05f)
            {
                const float pad = 0.05f;
                params.bmin[i] -= pad;
                params.bmax[i] += pad;
            }
        }

        if (params.vertCount <= 0 || params.polyCount <= 0)
        {
            EE_CORE_ERROR("[NavMesh] Detour build aborted: no vertices or polygons to process.");
            DestroyBuild(c);
            return false;
        }

        for (int i = 0; i < 3; ++i)
        {
            if (!std::isfinite(params.bmin[i])) params.bmin[i] = 0.0f;
            if (!std::isfinite(params.bmax[i])) params.bmax[i] = 1.0f;
        }

        cfg.minRegionArea = std::max(1, (int)rcSqr(8));
        cfg.mergeRegionArea = std::max(1, (int)rcSqr(20));

        EE_CORE_INFO("[NavMesh] Detour bounds check: x=%.3f y=%.3f z=%.3f",
            params.bmax[0] - params.bmin[0],
            params.bmax[1] - params.bmin[1],
            params.bmax[2] - params.bmin[2]);

        if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
        {
            EE_CORE_ERROR("[NavMeshSystem] dtCreateNavMeshData failed");
            DestroyBuild(c);
            return false;
        }

        c.runtime = new NavMeshComponent::Runtime();
        c.runtime->navData = navData;
        c.runtime->nav = dtAllocNavMesh();
        if (dtStatusFailed(c.runtime->nav->init(navData, navDataSize, 0)))
        {
            EE_CORE_ERROR("[NavMeshSystem] NavMesh init failed");
            DestroyBuild(c);
            DestroyRuntime(c);
            return false;
        }

        c.runtime->query = dtAllocNavMeshQuery();
        if (dtStatusFailed(c.runtime->query->init(c.runtime->nav, 2048)))
        {
            EE_CORE_ERROR("[NavMeshSystem] NavMeshQuery init failed");
            DestroyBuild(c);
            DestroyRuntime(c);
            return false;
        }

        EE_CORE_INFO("[NavMeshSystem] Build complete!");
        return true;
    }

    bool NavMeshSystem::BakeTopOfCube(EntityID e)
    {
        if (!ECS::GetInstance().HasComponent<NavMeshComponent>(e) ||
            !ECS::GetInstance().HasComponent<Transform>(e) ||
            !ECS::GetInstance().HasComponent<Mesh>(e))
            return false;

        auto& t = ECS::GetInstance().GetComponent<Transform>(e);
        auto& m = ECS::GetInstance().GetComponent<Mesh>(e);
        auto& nm = ECS::GetInstance().GetComponent<NavMeshComponent>(e);

        if (m.kind != MeshKind::Primitive || m.primitive.type != "Cube")
        {
            EE_CORE_WARN("[NavMeshSystem] Entity %u not a cube primitive; skipping", e);
            return false;
        }

        EE_CORE_INFO("[NavMeshSystem] Baking navmesh for cube entity %u", e);

        // Bake only the TOP face of the cube
        const float topVerts[] = {
            -0.5f, 0.5f, -0.5f,   // top left back
             0.5f, 0.5f, -0.5f,   // top right back
             0.5f, 0.5f,  0.5f,   // top right front
            -0.5f, 0.5f,  0.5f    // top left front
        };

        const int topTris[] = {
            0,1,2,
            0,2,3
        };

        std::vector<float> worldVerts;
        worldVerts.reserve(4 * 3);

        glm::quat rotQuat = glm::quat(t.rotation.w, t.rotation.x, t.rotation.y, t.rotation.z);

        glm::mat4 model =
            glm::translate(glm::mat4(1.0f), glm::vec3(t.position.x, t.position.y, t.position.z)) *
            glm::mat4_cast(rotQuat) *
            glm::scale(glm::mat4(1.0f),
                glm::vec3(t.scale.x * m.primitive.size.x,
                    t.scale.y * m.primitive.size.y,
                    t.scale.z * m.primitive.size.z));

        for (int i = 0; i < 4; ++i)
        {
            glm::vec4 v = model * glm::vec4(topVerts[i * 3], topVerts[i * 3 + 1], topVerts[i * 3 + 2], 1.0f);
            worldVerts.push_back(v.x);
            worldVerts.push_back(v.y);
            worldVerts.push_back(v.z);
        }

        bool ok = BuildFromTriangles(nm, worldVerts.data(), 4, topTris, 2);
        if (!ok)
        {
            EE_CORE_ERROR("[NavMeshSystem] Bake failed for entity %u", e);
            return false;
        }

        if (nm.build && nm.build->pmesh)
        {
            EE_CORE_INFO("[NavMeshSystem] Baked navmesh verts=%d polys=%d",
                nm.build->pmesh->nverts, nm.build->pmesh->npolys);
        }

        return true;
    }

    void NavMeshSystem::DebugDraw()
    {
        auto renderer = ECS::GetInstance().GetSystem<graphics::Renderer>();
        if (!renderer) { EE_CORE_WARN("[NavMesh] DebugDraw: no Renderer system."); return; }

        const auto& view = editor::EditorCamera::GetInstance().GetViewMatrix();
        const auto& proj = editor::EditorCamera::GetInstance().GetProjectionMatrix();

        int tilesVisited = 0;
        int polysVisited = 0;
        int linesSubmitted = 0;

        for (auto e : m_Entities)
        {
            if (!ECS::GetInstance().HasComponent<NavMeshComponent>(e)) continue;
            auto& c = ECS::GetInstance().GetComponent<NavMeshComponent>(e);
            if (!c.runtime || !c.runtime->nav) continue;

            const dtNavMesh* nav = c.runtime->nav;

            for (int ti = 0; ti < nav->getMaxTiles(); ++ti)
            {
                const dtMeshTile* tile = nav->getTile(ti);
                if (!tile || !tile->header) continue;
                ++tilesVisited;

                const dtPoly* polys = tile->polys;
                const float* verts = tile->verts;

                for (int pi = 0; pi < tile->header->polyCount; ++pi)
                {
                    const dtPoly* p = &polys[pi];
                    if (p->getType() == DT_POLYTYPE_OFFMESH_CONNECTION) continue;
                    ++polysVisited;

                    const unsigned nv = p->vertCount;
                    for (unsigned k = 0; k < nv; ++k)
                    {
                        const float* v0 = &verts[p->verts[k] * 3];
                        const float* v1 = &verts[p->verts[(k + 1) % nv] * 3];

                        // lift slightly to avoid z-fighting
                        glm::vec3 a(v0[0], v0[1] + 0.01f, v0[2]);
                        glm::vec3 b(v1[0], v1[1] + 0.01f, v1[2]);

                        renderer->SubmitDebugLine(a, b, glm::vec3(0.f, 1.f, 0.f)); // bright green
                        ++linesSubmitted;
                    }
                }
            }
        }
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        renderer->RenderDebugLines(view, proj);

        glDisable(GL_DEPTH_TEST);
    }

    void NavMeshSystem::DebugHighLight()
    {
        auto renderer = ECS::GetInstance().GetSystem<graphics::Renderer>();
        if (!renderer) return;

        const auto& view = editor::EditorCamera::GetInstance().GetViewMatrix();
        const auto& proj = editor::EditorCamera::GetInstance().GetProjectionMatrix();

        int trisSubmitted = 0;

        for (auto e : m_Entities)
        {
            if (!ECS::GetInstance().HasComponent<NavMeshComponent>(e)) continue;
            auto& c = ECS::GetInstance().GetComponent<NavMeshComponent>(e);
            if (!c.runtime || !c.runtime->nav) continue;

            const dtNavMesh* nav = c.runtime->nav;

            for (int ti = 0; ti < nav->getMaxTiles(); ++ti)
            {
                const dtMeshTile* tile = nav->getTile(ti);
                if (!tile || !tile->header) continue;

                const dtPoly* polys = tile->polys;
                const float* verts = tile->verts;

                for (int pi = 0; pi < tile->header->polyCount; ++pi)
                {
                    const dtPoly* p = &polys[pi];
                    if (p->getType() == DT_POLYTYPE_OFFMESH_CONNECTION) continue;

                    const unsigned nv = p->vertCount;
                    const float* v0 = &verts[p->verts[0] * 3];

                    for (unsigned k = 1; k + 1 < nv; ++k)
                    {
                        const float* v1 = &verts[p->verts[k] * 3];
                        const float* v2 = &verts[p->verts[k + 1] * 3];

                        glm::vec3 a(v0[0], v0[1] + 0.015f, v0[2]);
                        glm::vec3 b(v1[0], v1[1] + 0.015f, v1[2]);
                        glm::vec3 c_(v2[0], v2[1] + 0.015f, v2[2]);

                        renderer->SubmitDebugTriangle(a, b, c_, glm::vec3(0.0f, 1.0f, 0.0f));
                        ++trisSubmitted;
                    }
                }
            }
        }

        //EE_CORE_INFO("[NavMeshSystem] DebugDrawFilled: %d tris submitted", trisSubmitted);
        renderer->RenderDebugTriangles(view, proj);
    }

    bool NavMeshSystem::ComputeStraightPath(EntityID navEntity, const Vec3& start, const Vec3& end, std::vector<Vec3>& outPath)
    {
        //auto& ecs = ECS::GetInstance();
        //if (!ecs.IsEntityValid(navEntity) || !ecs.HasComponent<NavMeshComponent>(navEntity))
        //    return false;

        //EE_CORE_INFO("[NavMeshSystem] ComputeStraightPath called");

        //auto& navComp = ecs.GetComponent<NavMeshComponent>(navEntity);
        //if (!navComp.runtime) return false;

        //// Access to runtime internals is legal here (this TU defines Runtime)
        //dtNavMeshQuery* query = navComp.runtime->query;
        //if (!query) return false;

        //const float extents[3] = { 2.0f, 4.0f, 2.0f };

        //dtPolyRef startRef = 0, endRef = 0;
        //float spos[3] = { start.x, start.y, start.z };
        //float epos[3] = { end.x,   end.y,   end.z };

        //// Find nearest polys
        //if (dtStatusFailed(query->findNearestPoly(spos, extents, nullptr, &startRef, nullptr))) return false;
        //if (dtStatusFailed(query->findNearestPoly(epos, extents, nullptr, &endRef, nullptr))) return false;
        //if (!startRef || !endRef) return false;

        //// Find corridor polys
        //dtPolyRef polys[256];
        //int nPolys = 0;
        //if (dtStatusFailed(query->findPath(startRef, endRef, spos, epos, nullptr, polys, &nPolys, 256)))
        //    return false;
        //if (nPolys == 0) return false;

        //// Straight path
        //float straightPath[256 * 3];
        //unsigned char straightFlags[256];
        //dtPolyRef straightPolys[256];
        //int nStraight = 0;

        //if (dtStatusFailed(query->findStraightPath(spos, epos, polys, nPolys,
        //    straightPath, straightFlags, straightPolys,
        //    &nStraight, 256)))
        //    return false;

        //outPath.clear();
        //outPath.reserve((size_t)nStraight);
        //for (int i = 0; i < nStraight; ++i)
        //{
        //    Vec3 p;
        //    p.x = straightPath[i * 3 + 0];
        //    p.y = straightPath[i * 3 + 1];
        //    p.z = straightPath[i * 3 + 2];
        //    outPath.push_back(p);
        //}
        //return !outPath.empty();

        auto& ecs = ECS::GetInstance();
        if (!ecs.IsEntityValid(navEntity) || !ecs.HasComponent<NavMeshComponent>(navEntity))
            return false;

        auto& navComp = ecs.GetComponent<NavMeshComponent>(navEntity);
        if (!navComp.runtime)
            return false;

        // Access the Detour query object
        dtNavMeshQuery* query = navComp.runtime->query;
        if (!query)
            return false;

        dtQueryFilter filter;
        filter.setIncludeFlags(0xFFFF); // include all
        filter.setExcludeFlags(0);      // exclude none

        // Broader extents for testing — you can reduce later (e.g. 2,4,2)
        const float extents[3] = { 10.0f, 20.0f, 10.0f };

        dtPolyRef startRef = 0, endRef = 0;
        float spos[3] = { start.x, start.y, start.z };
        float epos[3] = { end.x, end.y, end.z };

        if (dtStatusFailed(query->findNearestPoly(spos, extents, &filter, &startRef, nullptr)))
        {
            EE_CORE_WARN("[NavMeshSystem] findNearestPoly failed for start point");
            return false;
        }

        if (dtStatusFailed(query->findNearestPoly(epos, extents, &filter, &endRef, nullptr)))
        {
            EE_CORE_WARN("[NavMeshSystem] findNearestPoly failed for end point");
            return false;
        }

        if (!startRef || !endRef)
        {
            EE_CORE_WARN("[NavMeshSystem] Invalid start or end poly (start=%llu end=%llu)",
                static_cast<unsigned long long>(startRef),
                static_cast<unsigned long long>(endRef));
            return false;
        }

        dtPolyRef polys[256];
        int nPolys = 0;
        if (dtStatusFailed(query->findPath(startRef, endRef, spos, epos, &filter,
            polys, &nPolys, 256)))
        {
            EE_CORE_WARN("[NavMeshSystem] findPath failed");
            return false;
        }

        if (nPolys == 0)
        {
            EE_CORE_WARN("[NavMeshSystem] No corridor polys found");
            return false;
        }

        float straightPath[256 * 3];
        unsigned char straightFlags[256];
        dtPolyRef straightPolys[256];
        int nStraight = 0;

        if (dtStatusFailed(query->findStraightPath(spos, epos, polys, nPolys,
            straightPath, straightFlags, straightPolys,
            &nStraight, 256)))
        {
            EE_CORE_WARN("[NavMeshSystem] findStraightPath failed");
            return false;
        }

        outPath.clear();
        outPath.reserve(static_cast<size_t>(nStraight));

        EE_CORE_INFO("[NavMeshSystem] Straight path points: %d", nStraight);
        for (int i = 0; i < nStraight; ++i)
        {
            Vec3 p;
            p.x = straightPath[i * 3 + 0];
            p.y = straightPath[i * 3 + 1];
            p.z = straightPath[i * 3 + 2];
            outPath.push_back(p);

            EE_CORE_INFO("  Path[%d]: %.3f %.3f %.3f", i, p.x, p.y, p.z);
        }

        return !outPath.empty();
    }
}
