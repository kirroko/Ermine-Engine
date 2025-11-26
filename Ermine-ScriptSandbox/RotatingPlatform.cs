using System;
using ErmineEngine;

public class RotatingPlatform : MonoBehaviour
{
    public float speed = 2.0f;
    public bool active = true;

    private Rigidbody rb;
    private AudioComponent audioComp;

    void Start()
    {
        rb = GetComponent<Rigidbody>();
        audioComp = GetComponent<AudioComponent>();
        
        if (audioComp == null)
        {
            Console.WriteLine("Warning: No AudioComponent found on rotating platform!");
        }
        else
        {
            // Start playing the rotation sound when platform starts
            if (active)
            {
                audioComp.shouldPlay = true;
            }
        }
    }

    void Update()
    {
        if (!active)
        {
            // Stop audio when platform stops
            if (audioComp != null && audioComp.isPlaying)
            {
                audioComp.shouldStop = true;
            }
            return;
        }

        // Ensure audio is playing while platform rotates
        if (audioComp != null && !audioComp.isPlaying)
        {
            audioComp.shouldPlay = true;
        }

        // Apply rotation
        transform.Rotate(new Vector3(0f, speed * Time.deltaTime, 0f));
    }

    public void IsActive(bool state)
    {
        active = state;
        
        // Control audio based on active state
        if (audioComp != null)
        {
            if (state)
            {
                audioComp.shouldPlay = true;
            }
            else
            {
                audioComp.shouldStop = true;
            }
        }
    }
}