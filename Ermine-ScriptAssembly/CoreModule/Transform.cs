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

using System;
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

        //// Internal call to fetch global matrix; implement in native scripting bridge.
        //[MethodImpl(MethodImplOptions.InternalCall)]
        //private static extern bool Internal_GetGlobalMatrix(IntPtr nativeHandle, out Matrix4x4 matrix);

        //// Cache native pointer/handle if you already store it; placeholder:
        //private IntPtr m_NativeHandle;

        //private bool TryGetGlobalMatrix(out Matrix4x4 m) => Internal_GetGlobalMatrix(m_NativeHandle, out m);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern Vector3 Internal_GetWorldForward();
        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern Vector3 Internal_GetWorldRight();
        [MethodImpl(MethodImplOptions.InternalCall)]
        private extern Vector3 Internal_GetWorldUp();

        public Vector3 forward => Internal_GetWorldForward();
        public Vector3 right => Internal_GetWorldRight();
        public Vector3 up => Internal_GetWorldUp();

        //public Vector3 forward
        //{
        //    get
        //    {
        //        Vector3 e = rotation.eulerAngles;
        //        float cx = (float)System.Math.Cos(e.x * Mathf.Deg2Rad);
        //        float sx = (float)System.Math.Sin(e.x * Mathf.Deg2Rad);
        //        float cy = (float)System.Math.Cos(e.y * Mathf.Deg2Rad);
        //        float sy = (float)System.Math.Sin(e.y * Mathf.Deg2Rad);
        //        return new Vector3(sy * cx, -sx, cy * cx).normalized;
        //    }
        //}

        //public Vector3 up
        //{
        //    get
        //    {
        //        Vector3 f = forward;
        //        Vector3 r = right;
        //        return Vector3.Cross(r, f).normalized;
        //    }
        //}

        //public Vector3 right
        //{
        //    get
        //    {
        //        Vector3 e = rotation.eulerAngles;
        //        float cx = (float)System.Math.Cos(e.x * Mathf.Deg2Rad);
        //        float sx = (float)System.Math.Sin(e.x * Mathf.Deg2Rad);
        //        float cy = (float)System.Math.Cos(e.y * Mathf.Deg2Rad);
        //        float sy = (float)System.Math.Sin(e.y * Mathf.Deg2Rad);
        //        Vector3 r = new Vector3(cy, 0f, -sy);
        //        if (System.Math.Abs(sx) > 1e-6f)
        //            r = (r + new Vector3(0f, sx, 0f)).normalized;
        //        return r.normalized;
        //    }
        //}

        public void Translate(Vector3 delta) => position += delta;

        public void Rotate(Vector3 deltaEuler)
        {
            Quaternion deltaRotation = Quaternion.Euler(deltaEuler.x, deltaEuler.y, deltaEuler.z);
            rotation *= deltaRotation;
        }

        public void LookAt(Vector3 target)
        {
            Vector3 dir = (target - position).normalized;
            if (dir.SqrMagnitude < 1e-8f) return;

            float yaw = (float)System.Math.Atan2(dir.x, dir.z) / Mathf.Deg2Rad;
            float pitch = (float)System.Math.Asin(-dir.y) / Mathf.Deg2Rad;

            // Persist the roll (z) component of the current rotation
            Quaternion q = rotation;
            Vector3 currentEuler = q.eulerAngles;
            q.eulerAngles = new Vector3(pitch, yaw, currentEuler.z);
            rotation = q.normalized;
        }
    }
}
