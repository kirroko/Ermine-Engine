#include "PreCompile.h"
#include "PhysicDebugRenderer.h"
#include "MathVector.h"
#include <glad/glad.h>

using namespace JPH;
namespace Ermine
{
    // Draw a line in 3D space
    void MyDebugRenderer::DrawLine(JPH::RVec3Arg from, JPH::RVec3Arg to, JPH::ColorArg color)
    {
        glLineWidth(2.0f); // thicker lines
        glDisable(GL_DEPTH_TEST); // optional: draw on top
        glBegin(GL_LINES);
        glColor3f(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f);
        glVertex3f((float)from.GetX(), (float)from.GetY(), (float)from.GetZ());
        glVertex3f((float)to.GetX(), (float)to.GetY(), (float)to.GetZ());
        glEnd();
        glEnable(GL_DEPTH_TEST);
    }

    // Draw a triangle (optional for wireframe)
    void MyDebugRenderer::DrawTriangle(JPH::RVec3Arg v1, JPH::RVec3Arg v2, JPH::RVec3Arg v3, JPH::ColorArg color, ECastShadow castShadow)
    {
        // Draw edges of triangle as lines
        DrawLine(v1, v2, color);
        DrawLine(v2, v3, color);
        DrawLine(v3, v1, color);
    }


    // Draw 3D text (optional)
    void MyDebugRenderer::DrawText3D(RVec3Arg, const std::string_view&, ColorArg, float)
    {
        // Optional: ignore
    }

    // Create a triangle batch from an array of triangles
    DebugRenderer::Batch MyDebugRenderer::CreateTriangleBatch(const Triangle*, int)
    {
        return {}; // empty batch
    }
    DebugRenderer::Batch MyDebugRenderer::CreateTriangleBatch(const Vertex* inVertices, int inVertexCount, const JPH::uint32* inIndices, int inIndexCount)
    {
        return {};
    }
    void MyDebugRenderer::DrawGeometry(JPH::RMat44Arg inModelMatrix, const JPH::AABox& inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef& inGeometry, ECullMode inCullMode, ECastShadow inCastShadow, EDrawMode inDrawMode)
    {
    }

    MyDebugRenderer& MyDebugRenderer::GetInstance()
    {
        static MyDebugRenderer instance;
        return instance;
    }
}