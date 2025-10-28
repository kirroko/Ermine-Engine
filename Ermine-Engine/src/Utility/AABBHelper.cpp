/* Start Header ************************************************************************/
/*!
\file       AABBHelper.cpp
\author     Edwin Lee Zirui, edwinzirui.lee, 2301299, edwinzirui.lee@digipen.edu
\date       Oct 25, 2025
\brief      Implementation of AABBHelper utility class. Provides methods for calculating
            Axis-Aligned Bounding Boxes from mesh primitives, model components, and 
            entity transforms. Handles world-space transformations accounting for
            hierarchical position, rotation, and scale.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#include "PreCompile.h"
#include "AABBHelper.h"
#include "HierarchySystem.h"
#include "GeometryFactory.h"

namespace Ermine
{
    Vec3 AABBHelper::TransformVertex(const Vec3& vertex, 
                                    const Vec3& pos, 
                                    const Quaternion& rot, 
                                    const Vec3& scale)
    {
        // Scale
        Vec3 scaled(vertex.x * scale.x, vertex.y * scale.y, vertex.z * scale.z);

        // Rotate using quaternion
        float qx = rot.x, qy = rot.y, qz = rot.z, qw = rot.w;
        float len = std::sqrt(qx*qx + qy*qy + qz*qz + qw*qw);
        if (len > 0.0001f) {
            qx /= len; qy /= len; qz /= len; qw /= len;
        }

        float tx = 2.0f * (qy * scaled.z - qz * scaled.y);
        float ty = 2.0f * (qz * scaled.x - qx * scaled.z);
        float tz = 2.0f * (qx * scaled.y - qy * scaled.x);

        Vec3 rotated;
        rotated.x = scaled.x + qw * tx + (qy * tz - qz * ty);
        rotated.y = scaled.y + qw * ty + (qz * tx - qx * tz);
        rotated.z = scaled.z + qw * tz + (qx * ty - qy * tx);

        // Translate
        return Vec3(rotated.x + pos.x, rotated.y + pos.y, rotated.z + pos.z);
    }

    AABB AABBHelper::CalculateLocalAABB(const Mesh& mesh)
    {
        if (mesh.kind == MeshKind::Primitive) {
            Vec3 halfSize = mesh.primitive.size * 0.5f;
            return AABB(Vec3(-halfSize.x, -halfSize.y, -halfSize.z),
                       Vec3(halfSize.x, halfSize.y, halfSize.z));
        }
        return AABB(); // Invalid for non-primitives
    }

    AABB AABBHelper::CalculateLocalAABB(const ModelComponent& model)
    {
        if (!model.m_model) return AABB();

        AABB localAABB;
        auto vertices = model.m_model->GetMeshVertices();
        
        for (const auto& vertex : vertices) {
            localAABB.Encapsulate(Vec3(vertex.x, vertex.y, vertex.z));
        }
        
        return localAABB;
    }

    AABB AABBHelper::CalculateWorldAABB(EntityID entity)
    {
        auto& ecs = ECS::GetInstance();
        if (!ecs.IsEntityValid(entity)) return AABB();

        // Get world transform from HierarchySystem
        auto hierarchySystem = ecs.GetSystem<HierarchySystem>();
        Vec3 worldPos = hierarchySystem->GetWorldPosition(entity);
        Quaternion worldRot = hierarchySystem->GetWorldRotation(entity);
        Vec3 worldScale = hierarchySystem->GetWorldScale(entity);

        AABB worldAABB;

        // Handle Mesh component (primitives)
        if (ecs.HasComponent<Mesh>(entity)) {
            const auto& mesh = ecs.GetComponent<Mesh>(entity);
            if (mesh.kind == MeshKind::Primitive) {
                Vec3 halfSize = mesh.primitive.size * 0.5f;
                
                // Transform 8 corners
                Vec3 corners[8] = {
                    Vec3(-halfSize.x, -halfSize.y, -halfSize.z),
                    Vec3( halfSize.x, -halfSize.y, -halfSize.z),
                    Vec3(-halfSize.x,  halfSize.y, -halfSize.z),
                    Vec3( halfSize.x,  halfSize.y, -halfSize.z),
                    Vec3(-halfSize.x, -halfSize.y,  halfSize.z),
                    Vec3( halfSize.x, -halfSize.y,  halfSize.z),
                    Vec3(-halfSize.x,  halfSize.y,  halfSize.z),
                    Vec3( halfSize.x,  halfSize.y,  halfSize.z)
                };

                for (int i = 0; i < 8; ++i) {
                    Vec3 worldCorner = TransformVertex(corners[i], worldPos, worldRot, worldScale);
                    worldAABB.Encapsulate(worldCorner);
                }
                
                return worldAABB;
            }
        }

        // Handle ModelComponent
        if (ecs.HasComponent<ModelComponent>(entity)) {
            const auto& modelComp = ecs.GetComponent<ModelComponent>(entity);
            if (modelComp.m_model) {
                auto vertices = modelComp.m_model->GetMeshVertices();
                for (const auto& vertex : vertices) {
                    Vec3 localVertex(vertex.x, vertex.y, vertex.z);
                    Vec3 worldVertex = TransformVertex(localVertex, worldPos, worldRot, worldScale);
                    worldAABB.Encapsulate(worldVertex);
                }
                
                return worldAABB;
            }
        }

        // Fallback: AABB around pivot point
        worldAABB.Encapsulate(worldPos);
        return worldAABB;
    }
}