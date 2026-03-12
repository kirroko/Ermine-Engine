using ErmineEngine;
using System;

public class Attack : MonoBehaviour
{
    public string playerName = "Player";

    public float attackRange = 5.0f;
    // If player no longer close enough go back to Chase
    public float disengageDistance = 8.0f;

    // LOS
    public float viewDistance = 18.0f;
    public float rayHeight = 5.5f;
    public float rayForwardOffset = 2.0f; // push ray out of own collider
    public float loseSightGraceTime = 0.25f; // prevents flicker behind corners
    public float closeDetectDistance = 2.0f;

    // Damage settings
    public float damagePerTick = 10f;
    public float tickInterval = 1.0f;

    private GameObject playerGO;
    private ulong entityID;

    //private bool collidingWithPlayer = false;
    private float tickTimer = 0f;

    private float loseSightTimer = 0f;
    public float repathInterval = 0.10f;
    private float repathTimer = 0f;

    // stun guard
    public float stunDuration = 5.0f;
    private bool isStunned = false;
    private float stunTimer = 0.0f;
    public static bool RightClickStunArmed = false;
    private float armTimer = 0.0f;

    private Animator anim;
    public float stunRecoverDelay = 8.0f;
    private float recoverTimer = 0.0f;

    private GameObject enemyLight;

    public float lightHeight = 5.0f;
    public float lightForwardOffset = 0.5f;
    public Vector3 lightRotationOffset = new Vector3(0f, 0f, 0f); // radians

    // replace to this
    // Name of the entity with UIHealthbarComponent (must match your scene)
    //public string playerHealthBarName = "Healthbar";
    //private GameObject playerHealthBar;

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

        NavAgent.SetDestination(entityID, transform.position);

        HideEnemyLight();
        GlobalAudio.PlaySFX("LightDisable");
    }

    private string GetEnemyLightName()
    {
        return "enemyLight_" + entityID;
    }

    private void EnsureEnemyLight()
    {
        enemyLight = GameObject.Find(GetEnemyLightName());

        if (enemyLight == null)
        {
            enemyLight = Prefab.Instantiate("../Resources/Prefabs/LightCone10.prefab");
            if (enemyLight != null)
                enemyLight.name = GetEnemyLightName();
        }
    }

    private void ShowEnemyLight()
    {
        EnsureEnemyLight();

        if (enemyLight == null)
            return;

        Physics.Internal_SetLightValue((ulong)enemyLight.GetInstanceID(), 6.481f);
        Vector3 lightPos = transform.position
                         + new Vector3(0f, lightHeight, 0f)
                         + transform.forward * lightForwardOffset;

        enemyLight.transform.position = lightPos;

        Quaternion rot = transform.rotation;
        rot = rot * Quaternion.Euler(
            lightRotationOffset.x,
            lightRotationOffset.y,
            lightRotationOffset.z
        );

        enemyLight.transform.rotation = rot;
        enemyLight.SetActive(true);
    }

    private void HideEnemyLight()
    {
        EnsureEnemyLight();
        if (enemyLight != null)
        {
            Physics.Internal_SetLightValue((ulong)enemyLight.GetInstanceID(), 0.0f);
            GlobalAudio.StopSFX("LightDamageLoop");
        }
    }

    private void CachePlayerIfNeeded()
    {
        if (playerGO == null)
            playerGO = GameObject.Find(playerName);
    }

    void Start()
    {
        entityID = (ulong)gameObject.GetInstanceID();
        anim = GetComponent<Animator>();
        CachePlayerIfNeeded();
        tickTimer = tickInterval;
        loseSightTimer = loseSightGraceTime;

        // Find healthbar by name (replace to this)
        //playerHealthBar = GameObject.Find(playerHealthBarName);

        ShowEnemyLight();
    }

    private bool HasLineOfSightToPlayer()
    {
        if (playerGO == null) return false;

        Vector3 enemyPos = transform.position;
        Vector3 playerPos = playerGO.transform.position;
        Vector3 flatToPlayer = playerPos - enemyPos;
        flatToPlayer.y = 0f;
        if (flatToPlayer.Magnitude <= closeDetectDistance)
            return true;

        Vector3 origin = enemyPos
                       + new Vector3(0f, rayHeight, 0f)
                       + transform.forward * rayForwardOffset;

        Vector3 toPlayer = playerPos - origin;

        float dist = toPlayer.Magnitude;
        if (dist <= 0.0001f) return true;
        if (dist > viewDistance) return false;

        Vector3 dir = toPlayer / dist;

        RaycastHit hit;
        bool didHit = Physics.Raycast(origin, dir, out hit, dist);
        if (!didHit) return false;

        var hitGO = hit.transform.gameObject;

        // Ignore self-hit
        ulong hitID = (ulong)hitGO.GetInstanceID();
        if (hitID == entityID) return false;

        string n = hitGO.name;
        if (n == GetEnemyLightName()) return false;
        if (n == "Sphere") return false;
        if (n.StartsWith("SpawnPoint_")) return false;

        return hit.transform != null &&
               hit.transform.gameObject != null &&
               hit.transform.gameObject.name == playerName;
    }

    private void FacePlayer()
    {
        if (playerGO == null) return;

        Vector3 toPlayer = playerGO.transform.position - transform.position;
        toPlayer.y = 0f;

        float dist = toPlayer.Magnitude;
        if (dist <= 0.0001f) return;

        // yaw in degrees
        float yaw = (float)(Math.Atan2(toPlayer.x, toPlayer.z) * 180.0 / Math.PI);

        Physics.SetRotationEuler(entityID, new Vector3(0f, yaw, 0f));
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
            HideEnemyLight();
            if (anim != null)
            {
                anim.SetBool("IsMoving", false);
                anim.SetBool("IsHit", true);
            }

            stunTimer -= Time.deltaTime;
            if (stunTimer <= 0.0f)
            {
                isStunned = false;
                recoverTimer = stunRecoverDelay;

                if (anim != null)
                    anim.SetBool("IsHit", false);
            }
            return;
        }

        if (recoverTimer > 0.0f)
        {
            HideEnemyLight();
            recoverTimer -= Time.deltaTime;

            if (anim != null)
                anim.SetBool("IsMoving", false);

            NavAgent.SetDestination(entityID, transform.position);
            return;
        }

        if (isStunned || recoverTimer > 0.0f)
            HideEnemyLight();
        else
            ShowEnemyLight();

        CachePlayerIfNeeded();
        if (playerGO == null) return;

        float distToPlayer = (playerGO.transform.position - transform.position).Magnitude;

        bool hasLOS = HasLineOfSightToPlayer();
        if (hasLOS)
            loseSightTimer = loseSightGraceTime;
        else
            loseSightTimer -= Time.deltaTime;

        if (distToPlayer > disengageDistance || loseSightTimer <= 0f)
        {
            StateMachine.RequestPreviousState(entityID);
            tickTimer = tickInterval;
            return;
        }

        if (distToPlayer > attackRange)
        {
            if (anim != null)
                anim.SetBool("IsMoving", true);

            tickTimer = tickInterval; // don’t damage while out of range

            repathTimer -= Time.deltaTime;
            if (repathTimer <= 0f)
            {
                NavAgent.SetDestination(entityID, playerGO.transform.position);
                repathTimer = repathInterval;
            }
            return;
        }

        // IN attack range, stop moving and deal damage
        FacePlayer();
        ShowEnemyLight();

        if (anim != null)
            anim.SetBool("IsMoving", false);

        NavAgent.SetDestination(entityID, transform.position);

        tickTimer -= Time.deltaTime;
        if (tickTimer <= 0f)
        {
            //DealDamageToPlayer(damagePerTick);
            tickTimer = tickInterval;
        }
    }

    //private void DealDamageToPlayer(float dmg)
    //{
    //    float health = GameplayHUD.GetHealth(GameplayHUD.GetHealthBar());
    //    health = Math.Max(0, health - dmg);

    //    GameObject bar = GameplayHUD.GetHealthBar();
    //    GameplayHUD.SetHealth(bar, health);

    //    // replace to this
    //    //if (playerHealthBar == null) return;

    //    //float health = GameplayHUD.GetHealth(playerHealthBar);
    //    //health = Math.Max(0, health - dmg);

    //    //GameplayHUD.SetHealth(playerHealthBar, health);
    //}

    void OnCollisionEnter(Collision col)
    {
        //if (col.gameObject.name == playerName)
        //    collidingWithPlayer = true;

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
        //if (col.gameObject.name == playerName)
        //    collidingWithPlayer = true;

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
        //if (col.gameObject.name == playerName)
        //{
        //    collidingWithPlayer = false;
        //    tickTimer = tickInterval;
        //}
    }
}