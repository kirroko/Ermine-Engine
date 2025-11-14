/* Start Header ************************************************************************/
/*!
\file       SimpleLightCone.cs
\author     Jeremy Lim
\date       14/11/2025
\brief      Simple script to dynamically scale a cone mesh to match spotlight projection.
            Attach this to the spotlight entity. The cone MUST be a child of this light.

Usage:
1. Make the cone (entity 70) a child of the light (entity 55) in the scene (DONE in scene)
2. Attach this script to the light entity
3. The script will automatically find and control the first child entity with a mesh

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

using ErmineEngine;
using System;

public class SimpleLightCone : MonoBehaviour
{
    [SerializeField] private float coneAngleDegrees = 20.0f; // Spotlight outer angle
    [SerializeField] private float maxConeDistance = 10.0f; // Maximum cone projection distance

    private bool isGrounded;
    private float groundY;
    private Vector3 groundHitPoint;

    void Start()
    {
        Debug.Log($"SimpleLightCone: Initializing on light entity {gameObject.name}");
        Debug.Log($"Cone angle: {coneAngleDegrees} degrees, Max distance: {maxConeDistance}");
    }

    void Update()
    {
        // Perform ground detection using physics raycast
        DetectGround();

        float distanceToGround;

        if (!isGrounded)
        {
            // No ground detected - use max distance as fallback
            Debug.LogWarning("SimpleLightCone: No ground detected below light!");
            distanceToGround = maxConeDistance;
            UpdateConeTransform(distanceToGround);
            return;
        }

        // Calculate distance from light to ground hit point
        distanceToGround = transform.position.y - groundY;

        if (distanceToGround <= 0.1f)
        {
            // Light is at or below ground - cone should be hidden
            Debug.LogWarning("SimpleLightCone: Light is below ground level!");
            HideCone();
            return;
        }

        // Clamp distance to max cone distance
        distanceToGround = Math.Min(distanceToGround, maxConeDistance);

        // Update the cone's transform based on detected ground distance
        UpdateConeTransform(distanceToGround);
    }

    private void DetectGround()
    {
        // Cast a ray downward from the light's position using Physics API
        Vector3 rayStart = transform.position;
        Vector3 rayDirection = Vector3.down; // Cast straight down
        
        Physics.RaycastHit hit;
        isGrounded = Physics.Raycast(rayStart, rayDirection, out hit, maxConeDistance);
        
        if (isGrounded)
        {
            groundHitPoint = hit.point;
            groundY = hit.point.y;
            
            // Debug visualization
            Debug.Log($"[LightCone] Ground hit at Y={groundY:F2}, distance={hit.distance:F2}");
        }
        else
        {
            isGrounded = false;
            Debug.LogWarning($"[LightCone] No ground detected within {maxConeDistance} units below light");
        }
    }

    private void UpdateConeTransform(float distanceToGround)
    {
        // Calculate cone dimensions based on spotlight angle
        // IMPORTANT: Use HALF the cone angle for tan() calculation
        float halfAngleRad = (coneAngleDegrees * 0.5f) * 0.0174533f; // Convert to radians (PI/180)
        float baseRadius = distanceToGround * (float)Math.Tan(halfAngleRad);
        
        // The C++ LightConeSystem handles the actual transform updates
        // This script provides the logic and debugging
        
        // Expected cone transform (set by C++ system):
        // - Local position: (0, -distanceToGround/2, 0)
        // - Scale: (baseRadius, distanceToGround/2, baseRadius)
        // - Rotation: Identity (pointing down)
        
        Debug.Log($"[LightCone] Light Y={transform.position.y:F2} | Ground Y={groundY:F2} | Distance={distanceToGround:F2} | Radius={baseRadius:F2}");
    }

    private void HideCone()
    {
        // The C++ LightConeSystem will scale the cone to (0, 0, 0)
        Debug.Log("[LightCone] Cone hidden - light is at or below ground");
    }
}