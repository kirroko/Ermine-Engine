using ErmineEngine;
using System;

public class Patrol : MonoBehaviour
{
    public float radius = 5f;
    public int pointCount = 16;
    public float reachDist = 0.5f;

    // If we’re not getting closer for this long, we give up and pick the next point.
    public float stuckTime = 0.75f;
    public float minProgressEpsilon = 0.02f;

    public float recenterDelay = 1.0f;

    public string playerName = "Player";
    public float detectPlayerDistance = 6.0f;

    private GameObject playerGO;

    private Vector3[] patrolPoints;
    private int currentIndex = -1;

    private float stuckTimer = 0f;
    private float lastDist = float.MaxValue;

    private ulong entityID;

    private bool jumping = false;
    private ulong jumpLinkEntityID = 0;

    private bool pendingRecenter = false;
    private float recenterTimer = 0f;

    private void CachePlayerIfNeeded()
    {
        if (playerGO == null)
            playerGO = GameObject.Find(playerName);
    }

    private bool PlayerCloseEnoughToChase()
    {
        CachePlayerIfNeeded();
        if (playerGO == null) return false;

        float d = (playerGO.transform.position - transform.position).Magnitude;
        return d <= detectPlayerDistance;
    }

    private void MoveToNextPoint()
    {
        if (patrolPoints == null || patrolPoints.Length == 0)
            return;

        currentIndex = (currentIndex + 1) % patrolPoints.Length;

        stuckTimer = 0f;
        lastDist = float.MaxValue;

        NavAgent.SetDestination(entityID, patrolPoints[currentIndex]);
    }

    private void BuildPatrolPoints(Vector3 center)
    {
        if (pointCount < 2) pointCount = 2;

        patrolPoints = new Vector3[pointCount];

        for (int i = 0; i < pointCount; i++)
        {
            float t = (float)i / (float)pointCount;
            float ang = t * 6.28318530718f; // 2*pi

            patrolPoints[i] = new Vector3(
                center.x + (float)Math.Cos(ang) * radius,
                center.y,
                center.z + (float)Math.Sin(ang) * radius
            );
        }

        currentIndex = -1;
        MoveToNextPoint();
    }

    void Start()
    {
        entityID = (ulong)gameObject.GetInstanceID();
        CachePlayerIfNeeded();

        // Build patrol points around the spawn position
        BuildPatrolPoints(transform.position);
    }

    void Update()
    {
        if (PlayerCloseEnoughToChase())
        {
            StateMachine.RequestNextState(entityID);
            return;
        }

        // If we just requested a jump, call StartJump once.
        if (jumping)
        {
            Debug.Log("CALL StartJump: me=" + entityID + " link=" + jumpLinkEntityID);
            NavAgent.StartJump(entityID, jumpLinkEntityID);

            jumping = false;
            jumpLinkEntityID = 0; // clear after use

            // Schedule a patrol recenter after the jump likely finishes
            pendingRecenter = true;
            recenterTimer = recenterDelay;

            return;
        }

        // After landing (likely), rebuild patrol points around current position (new platform)
        if (pendingRecenter)
        {
            recenterTimer -= Time.deltaTime;
            if (recenterTimer <= 0f)
            {
                pendingRecenter = false;
                BuildPatrolPoints(transform.position);
                return; // let destination update settle this frame
            }
        }

        if (patrolPoints == null || patrolPoints.Length == 0 || currentIndex < 0)
            return;

        Vector3 pos = transform.position;
        Vector3 target = patrolPoints[currentIndex];

        float dist = (target - pos).Magnitude;

        // Reached target -> go next
        if (dist <= reachDist)
        {
            MoveToNextPoint();
            return;
        }

        // Stuck detection: if we’re not getting closer, count time
        if (dist >= lastDist - minProgressEpsilon)
            stuckTimer += Time.deltaTime;
        else
            stuckTimer = 0f;

        lastDist = dist;

        if (stuckTimer >= stuckTime)
        {
            // Give up on this point and try the next one
            MoveToNextPoint();
        }
    }

    void OnCollisionEnter(Collision col)
    {
        if (jumping) return;

        if (col.gameObject.name == "JumpArea")
        {
            jumping = true;
            jumpLinkEntityID = (ulong)col.gameObject.GetInstanceID();
        }
    }

    void OnCollisionStay(Collision col)
    {
        if (jumping) return;

        if (col.gameObject.name == "JumpArea")
        {
            jumping = true;
            jumpLinkEntityID = (ulong)col.gameObject.GetInstanceID();
        }
    }

    void OnCollisionExit(Collision col)
    {
        if (jumping) return;

        if (col.gameObject.name == "JumpArea")
        {
            jumping = true;
            jumpLinkEntityID = (ulong)col.gameObject.GetInstanceID();
        }
    }
}

