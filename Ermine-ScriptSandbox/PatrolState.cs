using ErmineEngine;
using System;

public class Patrol : MonoBehaviour
{
    public float radius = 30f;
    public int pointCount = 16;
    public float reachDist = 0.5f;

    // If we’re not getting closer for this long, we give up and pick the next point.
    public float stuckTime = 0.75f;
    public float minProgressEpsilon = 0.02f;

    //public float recenterDelay = 1.0f;

    public string playerName = "Player";

    private GameObject playerGO;

    private Vector3[] patrolPoints;
    private int currentIndex = -1;

    private float stuckTimer = 0f;
    private float lastDist = float.MaxValue;

    private ulong entityID;

    private ulong jumpLinkEntityID = 0;
    private bool insideJumpArea = false;
    public float jumpCooldown = 3.0f;
    private float jumpCooldownTimer = 0.0f;

    // stun guard
    public float stunDuration = 5.0f;
    private bool isStunned = false;
    private float stunTimer = 0.0f;
    public static bool RightClickStunArmed = false;
    private float armTimer = 0.0f;

    public float viewDistance = 15.0f;
    public float rayHeight = 5.5f;
    public float rayForwardOffset = 2.0f;
    public float closeDetectDistance = 2.0f;

    private Vector3 patrolCenter;

    private Animator anim;
    public float stunRecoverDelay = 8.0f;
    private float recoverTimer = 0.0f;

    private void CachePlayerIfNeeded()
    {
        if (playerGO == null)
            playerGO = GameObject.Find(playerName);
    }

    private bool HasLineOfSightToPlayer()
    {
        CachePlayerIfNeeded();
        if (playerGO == null) return false;

        Vector3 enemyPos = transform.position;
        Vector3 playerPoint = playerGO.transform.position;

        // fallback for very close targets on tiny platforms
        Vector3 flatToPlayer = playerPoint - enemyPos;
        flatToPlayer.y = 0f;
        if (flatToPlayer.Magnitude <= closeDetectDistance)
            return true;

        Vector3 origin = enemyPos
                       + new Vector3(0f, rayHeight, 0f)
                       + transform.forward * rayForwardOffset;

        Vector3 toPlayer = playerPoint - origin;

        float dist = toPlayer.Magnitude;
        if (dist <= 0.0001f) return true;
        if (dist > viewDistance) return false;

        Vector3 dirToPlayer = toPlayer / dist;

        RaycastHit hit;
        bool didHit = Physics.Raycast(origin, dirToPlayer, out hit, dist);
        if (!didHit) return false;

        var hitGO = hit.transform.gameObject;

        // Ignore self-hit
        ulong hitID = (ulong)hitGO.GetInstanceID();
        if (hitID == entityID) return false;

        string n = hitGO.name;
        if (n.StartsWith("SpawnPoint_")) return false;

        return hit.transform != null &&
               hit.transform.gameObject != null &&
               hit.transform.gameObject.name == playerName;
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
            float ang = t * 6.28318530718f;

            patrolPoints[i] = new Vector3(
                center.x + (float)Math.Cos(ang) * radius,
                center.y,
                center.z + (float)Math.Sin(ang) * radius
            );
        }

        currentIndex = -1;
        MoveToNextPoint();
    }

    private void TryStun()
    {
        if (isStunned)
            return;

        isStunned = true;
        stunTimer = stunDuration;

        if (anim != null)
        {
            anim.SetBool("IsMoving", false);
            anim.SetBool("IsHit", true);
        }

        // stop immediately while stunned
        NavAgent.SetDestination(entityID, transform.position);
    }

    void Start()
    {
        entityID = (ulong)gameObject.GetInstanceID();
        anim = GetComponent<Animator>();
        CachePlayerIfNeeded();

        // Build patrol points around the spawn position
        patrolCenter = transform.position;
        BuildPatrolPoints(patrolCenter);
    }

    void Update()
    {
        if (Input.GetMouseButtonDown(1))
            armTimer = 0.3f;

        if (armTimer > 0.0f)
            armTimer -= Time.deltaTime;

        RightClickStunArmed = armTimer > 0f;

        if (jumpCooldownTimer > 0.0f)
            jumpCooldownTimer -= Time.deltaTime;

        if (isStunned)
        {
            if (anim != null)
            {
                anim.SetBool("IsMoving", false);
                anim.SetBool("IsHit", true);
            }

            //Debug.Log("stunned");
            stunTimer -= Time.deltaTime;
            if (stunTimer <= 0.0f)
            {
                isStunned = false;
                recoverTimer = stunRecoverDelay;

                if (anim != null)
                    anim.SetBool("IsHit", false);

                // Resume the current target after stun ends
                if (patrolPoints != null && patrolPoints.Length > 0 && currentIndex >= 0)
                    NavAgent.SetDestination(entityID, patrolPoints[currentIndex]);
            }
            return; // do NOTHING while stunned
        }
        if (recoverTimer > 0.0f)
        {
            recoverTimer -= Time.deltaTime;

            if (anim != null)
                anim.SetBool("IsMoving", false);

            NavAgent.SetDestination(entityID, transform.position);
            return;
        }

        if (anim != null)
            anim.SetBool("IsMoving", true);

        if (HasLineOfSightToPlayer())
        {
            StateMachine.RequestNextState(entityID);
            return;
        }

        if (insideJumpArea && jumpCooldownTimer <= 0.0f)
        {
            NavAgent.StartJump(entityID, jumpLinkEntityID);
            jumpCooldownTimer = jumpCooldown;
            return;
        }

        if (patrolPoints == null || patrolPoints.Length == 0 || currentIndex < 0)
        {
            BuildPatrolPoints(patrolCenter);
            return;
        }

        Vector3 pos = transform.position;
        Vector3 target = patrolPoints[currentIndex];

        float dist = (target - pos).Magnitude;

        // reached target, go next
        if (dist <= reachDist)
        {
            MoveToNextPoint();
            return;
        }

        // stuck detection if we’re not getting closer, count time
        if (dist >= lastDist - minProgressEpsilon)
            stuckTimer += Time.deltaTime;
        else
            stuckTimer = 0f;

        lastDist = dist;

        if (stuckTimer >= stuckTime)
        {
            // give up on this point and try the next one
            MoveToNextPoint();
        }
    }

    void OnCollisionEnter(Collision col)
    {
        if (col.gameObject.name == "JumpArea")
        {
            insideJumpArea = true;
            jumpLinkEntityID = (ulong)col.gameObject.GetInstanceID();
        }

        if (!RightClickStunArmed) return;
        if (col.gameObject.name == "Sphere")
        {
            Debug.Log("Patrol: Hit by sphere! Attempting stun.");
            TryStun();
            armTimer = 0.0f;
            RightClickStunArmed = false;
        }
    }

    void OnCollisionStay(Collision col)
    {
        if (col.gameObject.name == "JumpArea")
        {
            insideJumpArea = true;
            jumpLinkEntityID = (ulong)col.gameObject.GetInstanceID();
        }

        if (!RightClickStunArmed) return;
        if (col.gameObject.name == "Sphere")
        {
            Debug.Log("Patrol: Hit by sphere! Attempting stun.");
            TryStun();
            armTimer = 0.0f;
            RightClickStunArmed = false;
        }
    }

    void OnCollisionExit(Collision col)
    {
        if (col.gameObject.name == "JumpArea")
        {
            if (jumpLinkEntityID == (ulong)col.gameObject.GetInstanceID())
            {
                insideJumpArea = false;
                jumpLinkEntityID = 0;
            }
        }
    }
}

