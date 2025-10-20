using System;
using System.Runtime.CompilerServices;

namespace ErmineEngine
{
    public static class StateMachine
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void RequestNextState(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void RequestPreviousState(ulong entityID);
    }
}

