using System.Runtime.CompilerServices;

namespace ErmineEngine
{
    /// <summary>
    /// Provides access to UI system for skill effects and animations
    /// </summary>
    public static class UISystem
    {
        /// <summary>
        /// Triggers a skill cast which activates the UI flash animation.
        /// This will check if entity has enough health, deduct cost, start cooldown, and play flash effect.
        /// </summary>
        /// <param name="entity">The entity with UIComponent (usually the HUD entity)</param>
        /// <param name="skillIndex">Skill slot index: 0=LMB/ShootOrb, 1=LMB2/Teleport, 2=RMB/BlindBurst, 3=R/Recall</param>
        /// <returns>True if skill was successfully cast, false if on cooldown or insufficient health</returns>
        public static bool CastSkill(GameObject entity, int skillIndex)
        {
            if (entity == null)
                return false;

            return Internal_CastSkill((ulong)entity.GetInstanceID(), skillIndex);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool Internal_CastSkill(ulong entityID, int skillIndex);
    }
}
