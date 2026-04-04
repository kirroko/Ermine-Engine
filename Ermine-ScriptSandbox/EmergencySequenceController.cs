using ErmineEngine;
using System;

public class EmergencySequenceController : MonoBehaviour
{
    public static EmergencySequenceController Instance;

    private GameObject killArea;

    private bool zoneActive = true;
    private bool sequenceStarted = false;
    private bool sequenceFinished = false;

    private bool alarmTriggered = false;
    private bool rumbleTriggered = false;
    private bool bgmTriggered = false;
    private bool killAreaTriggered = false;

    private float timer = 0.0f;

    public float killAreaRiseSpeed = 0.5f;
    private float safeOffsetBelowRespawn = 15.0f;
    private float respawnPauseTime = 3.0f;
    private float respawnPauseTimer = 0.0f;

    private float startKillAreaY = 0.0f;
    private float minKillAreaY = -9999.0f;



    

    private Random rand = new Random();

    public EmergencySequenceController()
    {
        Instance = this;
        Debug.Log("[Emergency] Constructor called, instance set");
    }

    public void Init()
    {
        Debug.Log("[Emergency] Init called");

        killArea = GameObject.Find("EmergencyKillArea");

        if (killArea != null)
        {
            Debug.Log("[Emergency] Kill area FOUND: " + killArea.name);

            startKillAreaY = killArea.transform.position.y;
            minKillAreaY = startKillAreaY;

            Debug.Log("[Emergency] Kill area start Y = " + startKillAreaY);
        }
        else
        {
            Debug.Log("[Emergency] Kill area NOT FOUND");
        }
    }

    public void SetZoneActive(bool active)
    {
        zoneActive = active;
        Debug.Log("[Emergency] Zone active set to: " + zoneActive);

        if (!zoneActive)
        {
            Debug.Log("[Emergency] Zone deactivated → resetting sequence");
            ResetSequenceCompletely();
        }
    }

    public void StartSequence()
    {
        Debug.Log("[Emergency] StartSequence called");

        if (!zoneActive)
        {
            Debug.Log("[Emergency] StartSequence BLOCKED: zone not active");
            return;
        }

        if (sequenceStarted)
        {
            Debug.Log("[Emergency] StartSequence BLOCKED: already started");
            return;
        }

        Debug.Log("[Emergency] SEQUENCE STARTED");

        sequenceStarted = true;
        sequenceFinished = false;

        timer = 0.0f;

        alarmTriggered = false;
        rumbleTriggered = false;
        bgmTriggered = false;
        killAreaTriggered = false;

        respawnPauseTimer = 0.0f;
    }

    void Update()
    {
        float dt = Time.deltaTime;

        if (!zoneActive)
        {
            // Debug.Log("[Emergency] Update skipped: zone inactive");
            return;
        }

        if (!sequenceStarted)
        {
            // Debug.Log("[Emergency] Update skipped: sequence not started");
            return;
        }

        if (sequenceFinished)
        {
            Debug.Log("[Emergency] Update skipped: sequence finished");
            return;
        }

        timer += dt;

        // Debug timer occasionally
        if ((int)(timer * 10) % 10 == 0)
        {
            Debug.Log("[Emergency] Timer = " + timer);
        }

        if (!alarmTriggered && timer >= 0.2f)
        {
            alarmTriggered = true;
            Debug.Log("[Emergency] Alarm triggered at time: " + timer);
            TriggerAlarm();
        }

        if (!rumbleTriggered && timer >= 0.5f)
        {
            rumbleTriggered = true;
            Debug.Log("[Emergency] Rumble triggered at time: " + timer);
            TriggerRumble();
        }

        if (!bgmTriggered && timer >= 1.2f)
        {
            bgmTriggered = true;
            Debug.Log("[Emergency] BGM triggered at time: " + timer);
            TriggerDangerBGM();
        }

        if (!killAreaTriggered && timer >= 1.5f)
        {
            killAreaTriggered = true;
            Debug.Log("[Emergency] Kill area movement STARTED at time: " + timer);
        }

        if (respawnPauseTimer > 0.0f)
        {
            respawnPauseTimer -= dt;

            Debug.Log("[Emergency] Kill area paused, remaining: " + respawnPauseTimer);

            if (respawnPauseTimer < 0.0f)
            {
                respawnPauseTimer = 0.0f;
                Debug.Log("[Emergency] Kill area pause ended");
            }

            return;
        }

        if (killAreaTriggered)
        {
            UpdateKillArea(dt);
        }



        
    }

    private void TriggerAlarm()
    {
        Debug.Log("[Emergency] TriggerAlarm()");
        GameObject.Find("EmergencyAlarm");
    }

    private void TriggerRumble() // Camera shake and sfx
    {
        Debug.Log("[Emergency] TriggerRumble()");

        GameManager.I.player.GetComponent<PlayerController2>()?.TriggerRumble();
    }

    private float RandomRange(float min, float max)
    {
        return (float)(min + rand.NextDouble() * (max - min));
    }

    private void TriggerDangerBGM()
    {
        Debug.Log("[Emergency] TriggerDangerBGM()");
    }

    private void UpdateKillArea(float dt)
    {
        if (killArea == null)
        {
            Debug.Log("[Emergency] UpdateKillArea FAILED: killArea is NULL");
            return;
        }

        Vector3 pos = killArea.transform.position;
        pos.y += killAreaRiseSpeed * dt;
        killArea.transform.position = pos;
        Physics.SetPosition((ulong)killArea.GetInstanceID(), pos);

        Debug.Log("[Emergency] Kill area Y = " + pos.y);
    }

    public void OnPlayerRespawn(Vector3 respawnPos)
    {
        Debug.Log("[Emergency] OnPlayerRespawn called at Y = " + respawnPos.y);

        if (!zoneActive)
        {
            Debug.Log("[Emergency] Respawn ignored: zone inactive");
            return;
        }

        if (!sequenceStarted)
        {
            Debug.Log("[Emergency] Respawn ignored: sequence not started");
            return;
        }

        if (killArea == null)
        {
            Debug.Log("[Emergency] Respawn FAILED: killArea is NULL");
            return;
        }

        float targetY = respawnPos.y - safeOffsetBelowRespawn;

        if (targetY < minKillAreaY)
        {
            targetY = minKillAreaY;
        }

        Vector3 pos = killArea.transform.position;
        if (pos.y > targetY)
        {
            pos.y = targetY;
            killArea.transform.position = pos;
            Physics.SetPosition((ulong)killArea.GetInstanceID(), pos);
            Debug.Log("[Emergency] Kill area moved to Y = " + pos.y);

            respawnPauseTimer = respawnPauseTime;
            Debug.Log("[Emergency] Kill area paused for " + respawnPauseTime + " seconds");
        }
    }

    public void ResetSequenceCompletely()
    {
        Debug.Log("[Emergency] ResetSequenceCompletely called");

        sequenceStarted = false;
        sequenceFinished = false;

        timer = 0.0f;

        alarmTriggered = false;
        rumbleTriggered = false;
        bgmTriggered = false;
        killAreaTriggered = false;

        respawnPauseTimer = 0.0f;

        if (killArea != null)
        {
            Vector3 pos = killArea.transform.position;
            pos.y = startKillAreaY;
            killArea.transform.position = pos;

            Debug.Log("[Emergency] Kill area reset to Y = " + pos.y);
        }
        else
        {
            Debug.Log("[Emergency] Reset FAILED: killArea is NULL");
        }
    }

    public bool IsSequenceRunning()
    {
        Debug.Log("[Emergency] IsSequenceRunning = " + (sequenceStarted && !sequenceFinished));
        return sequenceStarted && !sequenceFinished;
    }

    public bool IsZoneActive()
    {
        Debug.Log("[Emergency] IsZoneActive = " + zoneActive);
        return zoneActive;
    }
}