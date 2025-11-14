/* Start Header ************************************************************************/
/*!
\file       LightConeSystem.h
\author     Jeremy
\date       2025
\brief      System to update light cone visualizations to match spotlight projections

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

#pragma once
#include "Systems.h"
#include "Components.h"
#include "HierarchySystem.h"
#include "Physics.h"
#include <cmath>

namespace Ermine
{
    class LightConeSystem : public System
    {
    public:
        void Update()
        {
            auto& ecs = ECS::GetInstance();
            auto hierarchySystem = ecs.GetSystem<HierarchySystem>();
            auto physicsSystem = ecs.GetSystem<Physics>();
            
            if (!hierarchySystem)
                return;

            for (EntityID lightEntity : m_Entities)
            {
                if (!ecs.IsEntityValid(lightEntity))
                    continue;

                if (!ecs.HasComponent<LightConeComponent>(lightEntity) ||
                    !ecs.HasComponent<Transform>(lightEntity) ||
                    !ecs.HasComponent<HierarchyComponent>(lightEntity))
                    continue;

                auto& coneComp = ecs.GetComponent<LightConeComponent>(lightEntity);
                const auto& lightTransform = ecs.GetComponent<Transform>(lightEntity);
                const auto& hierarchy = ecs.GetComponent<HierarchyComponent>(lightEntity);

                // Get light's world position
                Vec3 lightWorldPos = hierarchySystem->GetWorldPosition(lightEntity);

                // Perform physics raycast to detect ground
                float distanceToGround = 0.0f;
                bool hitGround = false;

                if (physicsSystem)
                {
                    // Cast ray downward from light position
                    JPH::RVec3 rayOrigin(lightWorldPos.x, lightWorldPos.y, lightWorldPos.z);
                    JPH::RVec3 rayDirection(0.0f, -1.0f, 0.0f); // Straight down
                    float maxDistance = 100.0f; // Default max distance for raycast
                    
                    JPH::RayCastResult hitResult;
                    hitGround = physicsSystem->Raycast(rayOrigin, rayDirection, maxDistance, hitResult);

                    if (hitGround)
                    {
                        // Calculate actual distance to ground from hit point
                        Vec3 hitPoint(hitResult.mFraction * maxDistance * rayDirection.GetX() + rayOrigin.GetX(),
                                     hitResult.mFraction * maxDistance * rayDirection.GetY() + rayOrigin.GetY(),
                                     hitResult.mFraction * maxDistance * rayDirection.GetZ() + rayOrigin.GetZ());
                        
                        distanceToGround = lightWorldPos.y - hitPoint.y;
                    }
                }

                // Fallback to using groundY from component if no physics raycast hit
                if (!hitGround)
                {
                    distanceToGround = lightWorldPos.y - coneComp.groundY;
                }

                // Hide cone if light is at or below ground
                if (distanceToGround <= 0.1f)
                {
                    // Light is at or below ground - hide cone by scaling to zero
                    if (coneComp.coneEntityID != 0 && ecs.IsEntityValid(coneComp.coneEntityID))
                    {
                        auto& coneTransform = ecs.GetComponent<Transform>(coneComp.coneEntityID);
                        coneTransform.scale = Vec3(0.0f, 0.0f, 0.0f);
                        hierarchySystem->MarkDirty(coneComp.coneEntityID);
                    }
                    continue;
                }

                // Find or cache the cone child entity
                if (coneComp.coneEntityID == 0 || !ecs.IsEntityValid(coneComp.coneEntityID))
                {
                    // Search for first child with a Mesh component
                    const auto& children = hierarchy.children;
                    for (EntityID childID : children)
                    {
                        if (ecs.IsEntityValid(childID) && ecs.HasComponent<Mesh>(childID))
                        {
                            coneComp.coneEntityID = childID;
                            break;
                        }
                    }
                }

                // If we found a cone entity, update its transform
                if (coneComp.coneEntityID != 0 && ecs.IsEntityValid(coneComp.coneEntityID))
                {
                    // Calculate cone dimensions based on spotlight angle
                    // IMPORTANT: coneAngleDegrees is the FULL cone angle, but tan() needs HALF the angle
                    float halfAngleRad = (coneComp.coneAngleDegrees * 0.5f) * 0.0174533f; // Convert to radians
                    float baseRadius = distanceToGround * std::tan(halfAngleRad);

                    // Get cone's transform
                    auto& coneTransform = ecs.GetComponent<Transform>(coneComp.coneEntityID);

                    // Update local position (cone should be positioned halfway between light and ground)
                    // Since it's a child, this is in the light's local space
                    coneTransform.position = Vec3(0.0f, -distanceToGround / 2.0f, 0.0f);

                    // Update scale
                    // Default cone primitive is 1 unit radius, 2 units height
                    coneTransform.scale = Vec3(
                        baseRadius,              // X scale (radius)
                        distanceToGround / 2.0f, // Y scale (half-height because default cone is 2 units tall)
                        baseRadius               // Z scale (radius)
                    );

                    // Ensure cone points down (identity rotation in parent's space)
                    coneTransform.rotation = Quaternion(0.0f, 0.0f, 0.0f, 1.0f);

                    // Mark the cone transform as dirty to trigger hierarchy update
                    hierarchySystem->MarkDirty(coneComp.coneEntityID);
                }
            }
        }
    };
}
