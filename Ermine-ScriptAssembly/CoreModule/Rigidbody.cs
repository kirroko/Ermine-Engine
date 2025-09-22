/* Start Header ************************************************************************/
/*!
\file       Rigidbody.cs
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong@digipen.edu
\date       16/09/2025
\brief      Managed Rigidbody component API (Unity‑style) exposing position.
            Backed by native ECS Rigidbody (position, velocity, mass, etc).
            Internal calls (get_/set_) must be registered on the C++ side (ScriptEngine::RegisterInternalCalls).

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

using System.Runtime.CompilerServices;

namespace ErmineEngine
{
    public class Rigidbody : Component
    {
        public Vector3 position
        {
            [MethodImpl(MethodImplOptions.InternalCall)]
            get;
            [MethodImpl(MethodImplOptions.InternalCall)]
            set;
        }

        public Quaternion rotation
        {
            [MethodImpl(MethodImplOptions.InternalCall)]
            get;
            [MethodImpl(MethodImplOptions.InternalCall)]
            set;
        }
    }
}
