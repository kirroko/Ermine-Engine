using System;
using System.Runtime.CompilerServices;

namespace ErmineEngine
{
    public static class GameplayHUD
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern float Internal_GetHealth(ulong entity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Internal_SetHealth(ulong entity, float value);

        // temporary reference to health bar, to be removed
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern ulong Internal_GetHealthBar();

        public static float GetHealth(GameObject obj)
            => Internal_GetHealth((ulong)obj.GetInstanceID());

        public static void SetHealth(GameObject obj, float value)
            => Internal_SetHealth((ulong)obj.GetInstanceID(), value);

        public static GameObject GetHealthBar()
        {
            ulong id = Internal_GetHealthBar();
            return GameObject.FromEntityID(id);
        }
    }
}