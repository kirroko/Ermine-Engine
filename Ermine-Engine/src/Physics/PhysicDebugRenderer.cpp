#include "PreCompile.h"
#include "PhysicDebugRenderer.h"
#include "MathVector.h"
#include "Renderer.h"
#include "ECS.h"

using namespace JPH;
namespace Ermine
{
    void MyDebugRenderer::DrawLine(JPH::RVec3Arg from, JPH::RVec3Arg to, JPH::ColorArg color)
    {
        const auto& renderer = ECS::GetInstance().GetSystem<graphics::Renderer>();
        glm::vec3 col(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f);
        renderer->SubmitDebugLine(
            glm::vec3((float)from.GetX(), (float)from.GetY(), (float)from.GetZ()),
            glm::vec3((float)to.GetX(), (float)to.GetY(), (float)to.GetZ()),
            col
        );
    }

    static inline JPH::RVec3 Txf(JPH::RMat44Arg M, const JPH::Float3& p) {
        return M * JPH::RVec3((double)p.x, (double)p.y, (double)p.z);
    }

    void MyDebugRenderer::DrawTriangle(JPH::RVec3Arg v1, JPH::RVec3Arg v2, JPH::RVec3Arg v3, JPH::ColorArg color, ECastShadow castShadow)
    {
        // Draw edges of triangle as lines
        DrawLine(v1, v2, color);
        DrawLine(v2, v3, color);
        DrawLine(v3, v1, color);
    }


    void MyDebugRenderer::DrawText3D(RVec3Arg, const std::string_view&, ColorArg, float)
    {
     
    }

    JPH::DebugRenderer::Batch
        MyDebugRenderer::CreateTriangleBatch(const Triangle* t, int n)
    {
        return {};
    }

    JPH::DebugRenderer::Batch
        MyDebugRenderer::CreateTriangleBatch(const Vertex* v, int vcount,
            const JPH::uint32* idx, int icount)
    {
        return {};
    }

    void MyDebugRenderer::DrawGeometry(JPH::RMat44Arg model,
        const JPH::AABox&, float,
        JPH::ColorArg color, const GeometryRef& geom,
        ECullMode, ECastShadow, EDrawMode)
    {

    }
}