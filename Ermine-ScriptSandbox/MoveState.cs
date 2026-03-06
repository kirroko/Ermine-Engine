using ErmineEngine;
using System;

public class Move : MonoBehaviour
{
    public float stepDistance = 3.0f;
    public float reachDist = 0.6f;

    public float viewDistance = 15.0f;
    public float rayHeight = 5.5f;
    public float rayForwardOffset = 2.0f;

    public float turnCooldown = 0.5f; // prevents spam turning
    public float repathInterval = 0.25f; // prevents spamming SetDestination

    private ulong entityID;

    private Vector3 dir;
    private Vector3 target;

    private float turnTimer = 0f;
    private float repathTimer = 0f;

    private bool jumping = false;
    private ulong jumpLinkEntityID = 0;

    public string playerName = "Player";

    private GameObject playerGO;

    // stun guard
    public float stunDuration = 5.0f;
    private bool isStunned = false;
    private float stunTimer = 0.0f;
    public static bool RightClickStunArmed = false;
    private float armTimer = 0.0f;

    public float edgeCheckForward = 1.0f;     // how far ahead to test for ground
    public float edgeCheckUp = 2.0f;          // how high above to start the downward ray
    public float edgeCheckDown = 5.0f;        // how far down to raycast

    public float stuckTimeToTurn = 0.6f;      // seconds stuck before turning
    public float stuckMoveEps = 0.02f;        // how little movement counts as "stuck"
    private Vector3 lastPos;
    private float stuckTimer = 0f;

    private GameObject rayDebug;
    private void CachePlayerIfNeeded()
    {
        if (playerGO == null)
            playerGO = GameObject.Find(playerName);
    }

    private bool HasLineOfSightToPlayer()
    {
        CachePlayerIfNeeded();
        if (playerGO == null) return false;

        Vector3 origin = transform.position
                       + new Vector3(0f, rayHeight, 0f)
                       + transform.forward * rayForwardOffset;

        Vector3 playerPoint = playerGO.transform.position;
        Vector3 toPlayer = playerPoint - origin;

        float dist = toPlayer.Magnitude;
        if (dist <= 0.0001f) return true;
        if (dist > viewDistance) return false;

        Vector3 dirToPlayer = toPlayer / dist;

        RaycastHit hit;
        
        bool didHit = Physics.Raycast(origin, dirToPlayer, out hit, dist);
        if (!didHit) return false;

        return hit.transform != null &&
               hit.transform.gameObject != null &&
               hit.transform.gameObject.name == playerName;
    }

    private void TryStun()
    {
        if (isStunned)
            return;

        isStunned = true;
        stunTimer = stunDuration;

        // stop immediately while stunned
        NavAgent.SetDestination(entityID, transform.position);
    }

    void Start()
    {
        entityID = (ulong)gameObject.GetInstanceID();

        lastPos = transform.position;

        // this can be used for the enemy lightcone to damage player
        //rayDebug = Prefab.Instantiate("../Resources/Prefabs/Sphere.prefab");
        //rayDebug.name = "RayDebug";
        //rayDebug.transform.scale = new Vector3(0.2f, 0.2f, 0.2f);

        Vector3 f = transform.forward;
        if (Math.Abs(f.x) >= Math.Abs(f.z))
            dir = new Vector3(f.x >= 0 ? 1f : -1f, 0f, 0f);
        else
            dir = new Vector3(0f, 0f, f.z >= 0 ? 1f : -1f);

        FaceDir();
        PushTargetForward(true);
    }

    void Update()
    {
        if (Input.GetMouseButtonDown(1))
            armTimer = 0.3f;

        if (armTimer > 0.0f)
            armTimer -= Time.deltaTime;

        RightClickStunArmed = armTimer > 0f;

        if (isStunned)
        {
            //Debug.Log("stunned");
            stunTimer -= Time.deltaTime;
            if (stunTimer <= 0.0f)
            {
                isStunned = false;
            }
            return; // do NOTHING while stunned
        }

        if (HasLineOfSightToPlayer())
        {
            StateMachine.RequestNextState(entityID);
            return;
        }

        if (jumping)
        {
            //Debug.Log("CALL StartJump: me=" + entityID + " link=" + jumpLinkEntityID);
            NavAgent.StartJump(entityID, jumpLinkEntityID);

            jumping = false;
            jumpLinkEntityID = 0;

            return;
        }

        if (turnTimer > 0f) turnTimer -= Time.deltaTime;
        if (repathTimer > 0f) repathTimer -= Time.deltaTime;

        // only check turning if cooldown is over
        //if (turnTimer <= 0f && HitsSomethingInFront())
        //{
        //    TurnAround();
        //    FaceDir();
        //    PushTargetForward(true);

        //    turnTimer = turnCooldown;  // lock turning for a moment
        //    return;
        //}

        if (turnTimer <= 0f)
        {
            bool obstacle = HitsObstacleInFront();
            bool edge = !obstacle && IsEdgeAhead(); // only treat "no hit" as bad if it's an edge
            bool stuck = IsStuck();

            if (obstacle || edge || stuck)
            {
                TurnAround();
                FaceDir();
                PushTargetForward(true);

                stuckTimer = 0f;      // reset stuck state after turning
                turnTimer = turnCooldown;
                return;
            }
        }

        float dist = (target - transform.position).Magnitude;

        if (dist <= reachDist)
        {
            PushTargetForward(true);
        }
        else if (repathTimer <= 0f)
        {
            PushTargetForward(false);
        }
    }

    //private bool HitsSomethingInFront()
    //{
    //    Vector3 origin = transform.position
    //                   + new Vector3(0f, rayHeight, 0f)
    //                   + dir * rayForwardOffset;

    //    //if (rayDebug != null)
    //    //{
    //    //    rayDebug.transform.position = origin;
    //    //}

    //    RaycastHit hit;
    //    bool didHit = Physics.Raycast(origin, dir, out hit, 0.8f);

    //    if (!didHit)
    //        return false;
    //    //else
    //    //    Debug.Log(hit.transform.gameObject.name);

    //    var hitGO = hit.transform.gameObject;

    //    // Ignore self-hit
    //    ulong hitID = (ulong)hit.transform.gameObject.GetInstanceID();
    //    if (hitID == entityID) return false;

    //    string n = hitGO.name;
    //    if (n == playerName) return false;
    //    if (n == "Sphere") return false;
    //    //if (n == "RayDebug") return false;

    //    // Debug.Log("Hit: " + hit.transform.gameObject.name);
    //    return true;
    //}

    private bool HitsObstacleInFront()
    {
        Vector3 origin = transform.position
                       + new Vector3(0f, rayHeight, 0f)
                       + dir * rayForwardOffset;

        if (rayDebug != null)
            rayDebug.transform.position = origin;

        RaycastHit hit;
        bool didHit = Physics.Raycast(origin, dir, out hit, 0.8f);

        if (!didHit)
            return false;

        var hitGO = hit.transform.gameObject;

        // Ignore self-hit
        ulong hitID = (ulong)hitGO.GetInstanceID();
        if (hitID == entityID) return false;

        string n = hitGO.name;
        if (n == playerName) return false;
        if (n == "Sphere") return false;
        if (n == "RayDebug") return false;

        return true;
    }

    private bool IsEdgeAhead()
    {
        // Probe a point forward (roughly where we're about to step)
        Vector3 probePoint = transform.position + dir * edgeCheckForward;

        // Raycast downward to see if there is ground/navmesh collider below
        Vector3 origin = probePoint + Vector3.up * edgeCheckUp;

        RaycastHit hit;
        bool hasGround = Physics.Raycast(origin, Vector3.down, out hit, edgeCheckDown);

        // If your world has "RayDebug"/enemy colliders, you can add ignores here if needed.
        return !hasGround;
    }

    private bool IsStuck()
    {
        float moved = (transform.position - lastPos).Magnitude;

        if (moved <= stuckMoveEps)
            stuckTimer += Time.deltaTime;
        else
            stuckTimer = 0f;

        lastPos = transform.position;

        return stuckTimer >= stuckTimeToTurn;
    }

    private void PushTargetForward(bool force)
    {
        if (!force && repathTimer > 0f) return;

        Vector3 pos = transform.position;
        target = new Vector3(
            pos.x + dir.x * stepDistance,
            pos.y,
            pos.z + dir.z * stepDistance
        );

        NavAgent.SetDestination(entityID, target);
        repathTimer = repathInterval;
    }

    private void TurnAround()
    {
        dir = new Vector3(-dir.x, 0f, -dir.z);
    }

    private void FaceDir()
    {
        float yaw = (dir.z > 0) ? 0f :
                    (dir.z < 0) ? 180f :
                    (dir.x > 0) ? 90f : 270f;

        Vector3 rot = transform.rotation.eulerAngles;
        rot.y = yaw;
        transform.rotation = Quaternion.Euler(rot.x, rot.y, rot.z);
    }

    void OnCollisionEnter(Collision col)
    {
        if (jumping) return;
        if (col.gameObject.name == "JumpArea")
        {
            jumping = true;
            jumpLinkEntityID = (ulong)col.gameObject.GetInstanceID();
        }

        if (!RightClickStunArmed) return;
        if (col.gameObject.name == "Sphere")
        {
            TryStun();
            armTimer = 0.0f;
            RightClickStunArmed = false;
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

        if (!RightClickStunArmed) return;
        if (col.gameObject.name == "Sphere")
        {
            TryStun();
            armTimer = 0.0f;
            RightClickStunArmed = false;
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
