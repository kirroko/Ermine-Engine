using System;
using ErmineEngine;

public class RotatingPlatform : MonoBehaviour
{
    public float speed = 2.0f;
    public bool active = true;
    
    // Audio activation distance (slightly larger than maxDistance for smooth fade)
    public float audioActivationRange = 120f;

    private Rigidbody rb;
    private AudioComponent audioComp;
    private GameObject player;

    void Start()
    {
        rb = GetComponent<Rigidbody>();
        audioComp = GetComponent<AudioComponent>();
        player = GameObject.Find("Player");
        
        if (audioComp == null)
        {
            Console.WriteLine("Warning: No AudioComponent found on rotating platform!");
        }
        
        if (player == null)
        {
            Console.WriteLine("Warning: Player not found!");
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

        // Apply rotation
        transform.Rotate(new Vector3(0f, speed * Time.deltaTime, 0f));
        Physics.SetRotationQuat((ulong)gameObject.GetInstanceID(), transform.rotation);
        
        // Handle audio based on player distance
        HandleAudioByDistance();
    }

    private void HandleAudioByDistance()
    {
        if (audioComp == null || player == null) return;
        
        // Calculate distance manually
        Vector3 diff = player.transform.position - transform.position;
        float distance = Mathf.Sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
        
        // Start audio when player is close enough
        if (distance < audioActivationRange && !audioComp.isPlaying)
        {
            audioComp.shouldPlay = true;
        }
        // Stop audio when player is too far
        else if (distance >= audioActivationRange && audioComp.isPlaying)
        {
            audioComp.shouldStop = true;
        }
    }

    public void IsActive(bool state)
    {
        active = state;
        
        // Control audio based on active state
        if (audioComp != null)
        {
            if (state)
            {
                // Don't immediately play - let HandleAudioByDistance decide based on player proximity
                // Audio will start playing in Update() if player is close enough
            }
            else
            {
                audioComp.shouldStop = true;
            }
        }
    }
}