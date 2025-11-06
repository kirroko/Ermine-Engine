/* Start Header ************************************************************************/
/*!
\file       Physics.cs
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong\@digipen.edu
\date       04/11/2025
\brief      This file contains the Physics class which serves as a container for physics-related functionalities.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

using System.Runtime.CompilerServices;

namespace ErmineEngine
{
    public class Physics
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool Internal_Raycast(Vector3 origin, Vector3 direction, out RaycastHit hitInfo, float maxDistance);

        public struct RaycastHit
        {
            public Vector3 point;      // The impact point in world space where the ray hit the collider.
            public Vector3 normal;     // The normal of the surface the ray hit.
            public float distance;     // The distance from the ray's origin to the impact point.
            public Collider collider;  // The collider that was hit.
        }

        public static bool Raycast(Vector3 origin, Vector3 direction, out RaycastHit hitInfo, float maxDistance)
        {
            hitInfo = new RaycastHit();
            return Internal_Raycast(origin, direction, out hitInfo, maxDistance);
        }
    }
}
