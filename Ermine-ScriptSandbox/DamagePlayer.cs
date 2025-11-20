using ErmineEngine;
using System;

public class DamagePlayer : MonoBehaviour
{
    private float health = 0f;
    public float timer = 1f;
    public float damage = 10f;

    private void Start()
    {
        health = GameplayHUD.GetHealth(GameplayHUD.GetHealthBar());
    }

    private void Update()
    {
    }

    void TakeDamage(float dmg)
    {
        health = Math.Max(0, health - dmg);

        GameObject bar = GameplayHUD.GetHealthBar();
        GameplayHUD.SetHealth(bar, health);

        if (timer <= 0f)
            timer = 1f;
    }

    void OnCollisionEnter(Collision col)
    {
        //if (col.gameObject.name == "Player")
        //{
        //    Debug.Log("Ouch, Im working");
        //    TakeDamage(damage);
        //}
    }

    void OnCollisionStay(Collision col)
    {
        timer -= Time.deltaTime;

        if (timer <= 0f && col.gameObject.name == "Player")
        {
            Debug.Log("Ouch, Im working");
            TakeDamage(damage);
        }
    }
}