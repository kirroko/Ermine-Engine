using ErmineEngine;
using System;

public class DamagePlayer : MonoBehaviour
{
    private float health = 0f;
    public float timer = 1f;
    public float damage = 10f;
    private bool playerInside = false;

    private void Start()
    {
        health = GameplayHUD.GetHealth(GameplayHUD.GetHealthBar());
    }

    private void Update()
    {
        if (playerInside)
        {
            timer -= Time.deltaTime;

            if (timer <= 0f)
            {
                TakeDamage(damage);
                timer = 1f;
            }
        }
    }

    void TakeDamage(float dmg)
    {
        health = GameplayHUD.GetHealth(GameplayHUD.GetHealthBar());
        health = Math.Max(0, health - dmg);

        GameObject bar = GameplayHUD.GetHealthBar();
        GameplayHUD.SetHealth(bar, health);
    }

    void OnCollisionEnter(Collision col)
    {
        if (col.gameObject.name == "Player")
            playerInside = true;
    }
    void OnCollisionStay(Collision col)
    {
        if (col.gameObject.name == "Player")
            playerInside = true;
    }

    void OnCollisionExit(Collision col)
    {
        if (col.gameObject.name == "Player")
        {
            playerInside = false;
            timer = 1f; // Reset timer when leaving
        }
    }
}