using ErmineEngine;
using System;

public class Attack : MonoBehaviour
{
    public string playerName = "Player";

    // If player no longer “close enough” -> go back to Chase (previous state)
    public float disengageDistance = 2.0f;

    // Damage settings
    public float damagePerTick = 10f;
    public float tickInterval = 1.0f;

    private GameObject playerGO;
    private ulong entityID;

    private bool collidingWithPlayer = false;
    private float tickTimer = 0f;

    private void CachePlayerIfNeeded()
    {
        if (playerGO == null)
            playerGO = GameObject.Find(playerName);
    }

    void Start()
    {
        entityID = (ulong)gameObject.GetInstanceID();
        CachePlayerIfNeeded();
        tickTimer = tickInterval;
    }

    void Update()
    {
        CachePlayerIfNeeded();
        if (playerGO == null) return;

        float dist = (playerGO.transform.position - transform.position).Magnitude;

        // If not colliding OR too far -> back to Chase (previous state)
        if (!collidingWithPlayer || dist > disengageDistance)
        {
            StateMachine.RequestPreviousState(entityID);
            tickTimer = tickInterval;
            return;
        }

        // Deal damage over time
        tickTimer -= Time.deltaTime;
        if (tickTimer <= 0f)
        {
            DealDamageToPlayer(damagePerTick);
            tickTimer = tickInterval;
        }
    }

    private void DealDamageToPlayer(float dmg)
    {
        float health = GameplayHUD.GetHealth(GameplayHUD.GetHealthBar());
        health = Math.Max(0, health - dmg);

        GameObject bar = GameplayHUD.GetHealthBar();
        GameplayHUD.SetHealth(bar, health);
    }

    void OnCollisionEnter(Collision col)
    {
        if (col.gameObject.name == playerName)
            collidingWithPlayer = true;
    }

    void OnCollisionStay(Collision col)
    {
        if (col.gameObject.name == playerName)
            collidingWithPlayer = true;
    }

    void OnCollisionExit(Collision col)
    {
        if (col.gameObject.name == playerName)
        {
            collidingWithPlayer = false;
            tickTimer = tickInterval;
        }
    }
}

