using System;
using System.Runtime.CompilerServices;

namespace ErmineEngine
{
    public static class NavAgent
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetDestination(ulong entityID, Vector3 destination);
    }
}
