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

    private float lastYRot;
    private bool PlayerIsOnPlatform = false;

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
        lastYRot = transform.rotation.eulerAngles.y;
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

        float prevY = transform.rotation.eulerAngles.y;

        // Apply rotation
        transform.Rotate(new Vector3(0f, speed * Time.deltaTime, 0f));
        Physics.SetRotationQuat((ulong)gameObject.GetInstanceID(), transform.rotation);

        float currentY = transform.rotation.eulerAngles.y;
        float deltaAngle = currentY - lastYRot;

        // Wrap around 360
        if (deltaAngle > 180) deltaAngle -= 360;
        if (deltaAngle < -180) deltaAngle += 360;

        float radians = -deltaAngle * Mathf.Deg2Rad;

        if (PlayerIsOnPlatform)
        {
            Vector3 pos = player.transform.position;
            Vector3 center = transform.position;

            float dx = pos.x - center.x;
            float dz = pos.z - center.z;

            float cos = Mathf.Cos(radians);
            float sin = Mathf.Sin(radians);

            float newX = dx * cos - dz * sin;
            float newZ = dx * sin + dz * cos;

            pos.x = center.x + newX;
            pos.z = center.z + newZ;

            player.transform.position = pos;
            Physics.SetPosition((ulong)player.GetInstanceID(), pos);
        }

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
    void OnCollisionEnter(Collider other)
    {

    }
    void OnCollisionStay(Collider other)
    {
        if (other.gameObject.name == "Player")
        {
            PlayerIsOnPlatform = true;
        }
    }


    void OnCollisionExit(Collider other)
    {
        if (other.gameObject.name == "Player")
        {
            PlayerIsOnPlatform = false;
            Debug.Log("PlayerIsOnPlatform false");
        }
    }

}