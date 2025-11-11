using System;
using ErmineEngine;

public class Switch : MonoBehaviour
{
    public bool enabled = false;
    public RotatingPlatform target;

    void Start()
    {
        
    }

    void Update()
    {
        if (enabled)
        {
            target.IsActive(enabled);
        }
    }
}

