using System;
using System.Runtime.CompilerServices;

namespace ErmineEngine
{
    /// <summary>
    /// Wrapper for UIHealthbarComponent - provides access to health bar functionality
    /// </summary>
    public class UIHealthbar
    {
        private ulong entityID;

        public UIHealthbar(ulong entityID)
        {
            this.entityID = entityID;
        }

        /// <summary>
        /// Get current health value
        /// </summary>
        public float Health
        {
            get => Internal_GetHealth(entityID);
            set => Internal_SetHealth(entityID, value);
        }

        /// <summary>
        /// Get maximum health value
        /// </summary>
        public float MaxHealth => Internal_GetMaxHealth(entityID);

        /// <summary>
        /// Get or set health regeneration rate (health per second)
        /// </summary>
        public float RegenRate
        {
            get => Internal_GetRegenRate(entityID);
            set => Internal_SetRegenRate(entityID, value);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern float Internal_GetHealth(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void Internal_SetHealth(ulong entityID, float value);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern float Internal_GetMaxHealth(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern float Internal_GetRegenRate(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void Internal_SetRegenRate(ulong entityID, float value);
    }
}
