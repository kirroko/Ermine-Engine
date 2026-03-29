using System.Runtime.CompilerServices;

namespace ErmineEngine
{
    /// <summary>
    /// Script wrapper for native Material component.
    /// </summary>
    public class Material : Component
    {
        /// <summary>
        /// Mesh fill amount in [0, 1]. 1 = fully visible, 0 = fully unfilled.
        /// </summary>
        public float fill
        {
            get => Internal_GetFill(this);
            set => Internal_SetFill(this, value);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern float Internal_GetFill(Material self);

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void Internal_SetFill(Material self, float value);
    }
}
