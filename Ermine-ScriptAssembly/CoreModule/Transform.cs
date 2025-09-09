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
    public struct Vector3
    {
        public float x;
        public float y;
        public float z;

        public Vector3(float v)
        {
            x = y = z = v;
        }
        
        public Vector3(float x, float y, float z)         
        {
            this.x = x;
            this.y = y;
            this.z = z;
        }

        public static readonly Vector3 zero = new Vector3(0f, 0f, 0f);
        public static readonly Vector3 one = new Vector3(1f, 1f, 1f);
        public static readonly Vector3 up = new Vector3(0f, 1f, 0f);
        public static readonly Vector3 down = new Vector3(0f, -1f, 0f);
        public static readonly Vector3 right = new Vector3(1f, 0f, 0f);
        public static readonly Vector3 left = new Vector3(-1f, 0f, 0f);
        public static readonly Vector3 forward = new Vector3(0f, 0f, 1f);
        public static readonly Vector3 back = new Vector3(0f, 0f, -1f);

        public float Magnitude => (float)System.Math.Sqrt(x * x + y * y + z * z);
        public float SqrMagnitude => x * x + y * y + z * z;

        public Vector3 Normalized
        {
            get
            {
                float mag = Magnitude;
                return mag > 1e-6f ? this / mag : zero;
            }
        }

        public static float Dot(Vector3 a, Vector3 b) => a.x * b.x + a.y * b.y + a.z * b.z;
        public static Vector3 Cross(Vector3 a, Vector3 b) =>
        new Vector3(a.y * b.z - a.z * b.y, 
            a.z * b.x - a.x * b.z, 
            a.x * b.y - a.y * b.x);

        public static Vector3 operator +(Vector3 a, Vector3 b) => new Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
        public static Vector3 operator -(Vector3 a, Vector3 b) => new Vector3(a.x - b.x, a.y - b.y, a.z - b.z);
        public static Vector3 operator -(Vector3 v) => new Vector3(-v.x, -v.y, -v.z);
        public static Vector3 operator *(Vector3 v, float scalar) => new Vector3(v.x * scalar, v.y * scalar, v.z * scalar);
        public static Vector3 operator *(float scalar, Vector3 v) => new Vector3(v.x * scalar, v.y * scalar, v.z * scalar);
        public static Vector3 operator /(Vector3 v, float scalar) => new Vector3(v.x / scalar, v.y / scalar, v.z / scalar);

        public override string ToString() => $"({x:0.###}, {y:0.###}, {z:0.###})";
    }

    public class Transform : Component
    {
        public Vector3 position
        {
            [MethodImpl(MethodImplOptions.InternalCall)]
            get;
            [MethodImpl(MethodImplOptions.InternalCall)]
            set;
        }

        public Vector3 rotation
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
                Vector3 e = rotation;
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
                Vector3 e = rotation;
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
        public void Rotate(Vector3 deltaEuler) => rotation += deltaEuler;

        public void LookAt(Vector3 target)
        {
            Vector3 dir = (target - position).Normalized;
            if (dir.SqrMagnitude < 1e-8f) return;

            float yaw = (float)System.Math.Atan2(dir.x, dir.z) / Deg2Rad;
            float pitch = (float)System.Math.Asin(-dir.y) / Deg2Rad;
            Vector3 e = rotation;
            rotation = new Vector3(pitch, yaw, e.z);
        }
    }
}
