using System;
using System.Management.Instrumentation;

namespace ErmineEngine
{
    internal class MonoBehaviour
    {
        protected internal virtual void Awake()
        {
            // This method is called when the script instance is being loaded.
        }
        protected internal virtual void OnEnable()
        {
            // This method is called when the script instance is enabled.
        }
        protected internal virtual void Start()
        {
            // This method is called on the frame when a script is enabled just before any of the Update methods are called the first time.
        }
        protected internal virtual void Update()
        {
            // This method is called every frame, if the MonoBehaviour is enabled.
        }
        protected internal virtual void FixedUpdate()
        {
            // This method is called every fixed framerate frame, if the MonoBehaviour is enabled.
        }
        protected internal virtual void OnDisable()
        {
            // This method is called when the behaviour becomes disabled or inactive.
        }
        protected internal virtual void OnDestroy()
        {
            // This method is called when the MonoBehaviour will be destroyed.
        }

        public struct Entity
        {
            internal ulong Id; // ECS EntityID
        }

        public abstract class Component
        {
        }

        public sealed class Transform : Component
        {
            internal ulong EntityId;
        }
    }
}
