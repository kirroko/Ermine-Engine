/* Start Header ************************************************************************/
/*!
\file       Transform.cs
\author     WONG JUN YU, Kean, junyukean.wong, 2301234, junyukean.wong@digipen.edu
\date       02/09/2025
\brief      Managed Transform component API (Unity‑style) exposing position, rotation (Euler degrees), and scale.
            Backed by native ECS Transform (position, rotation, scale, matrix).
            Internal calls (get_/set_) must be registered on the C++ side (ScriptEngine::RegisterInternalCalls).

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the
prior written consent of DigiPen Institute of Technology is prohibited.
*/
/* End Header **************************************************************************/

using System.Runtime.CompilerServices;

namespace ErmineEngine
{
    public class Transform : Component
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

        public Vector3 scale
        {
            [MethodImpl(MethodImplOptions.InternalCall)]
            get;
            [MethodImpl(MethodImplOptions.InternalCall)]
            set;
        }

        const float Deg2Rad = (float)(System.Math.PI / 180.0);

        public Vector3 forward
        {
            get
            {
                Vector3 e = rotation.eulerAngles;
                float cx = (float)System.Math.Cos(e.x * Deg2Rad);
                float sx = (float)System.Math.Sin(e.x * Deg2Rad);
                float cy = (float)System.Math.Cos(e.y * Deg2Rad);
                float sy = (float)System.Math.Sin(e.y * Deg2Rad);
                return new Vector3(sy * cx, -sx, cy * cx).Normalized;
            }
        }

        public Vector3 up
        {
            get
            {
                Vector3 f = forward;
                Vector3 r = right;
                return Vector3.Cross(r, f).Normalized;
            }
        }

        public Vector3 right
        {
            get
            {
                Vector3 e = rotation.eulerAngles;
                float cx = (float)System.Math.Cos(e.x * Deg2Rad);
                float sx = (float)System.Math.Sin(e.x * Deg2Rad);
                float cy = (float)System.Math.Cos(e.y * Deg2Rad);
                float sy = (float)System.Math.Sin(e.y * Deg2Rad);
                Vector3 r = new Vector3(cy, 0f, -sy);
                if (System.Math.Abs(sx) > 1e-6f)
                    r = (r + new Vector3(0f, sx, 0f)).Normalized;
                return r.Normalized;
            }
        }

        public void Translate(Vector3 delta) => position += delta;

        public void Rotate(Vector3 deltaEuler)
        {
            Quaternion q = rotation;
            q.eulerAngles = q.eulerAngles + (deltaEuler * Deg2Rad);
            rotation = q.normalized;
        }

        public void LookAt(Vector3 target)
        {
            Vector3 dir = (target - position).Normalized;
            if (dir.SqrMagnitude < 1e-8f) return;

            float yaw = (float)System.Math.Atan2(dir.x, dir.z) / Deg2Rad;
            float pitch = (float)System.Math.Asin(-dir.y) / Deg2Rad;

            // Persist the roll (z) component of the current rotation
            Quaternion q = rotation;
            Vector3 currentEuler = q.eulerAngles;
            q.eulerAngles = new Vector3(pitch, yaw, currentEuler.z);
            rotation = q.normalized;
        }
    }
}
