using ErmineEngine;
using System;

public class PlayerGunCol : MonoBehaviour
{
    private PlayerController2 playerController;

    void Start()
    {
        GameObject player = GameObject.Find("Player");
        if (player != null)
        {
            playerController = player.GetComponent<PlayerController2>();
        }

        if (playerController == null)
        {
            Console.WriteLine("Warning: PlayerController2 not found on Player!");
        }
    }

    void OnCollisionEnter(Collision col)
    {
        if (playerController == null) return;


        if (col.gameObject.name.Contains("floor") || col.gameObject.name.Contains("Rotating"))
        {
            playerController.isGrounded = true;
            playerController.isKeyJump = false;
        }
    }

    void OnCollisionStay(Collision col)
    {
        if (playerController == null) return;

        if (col.gameObject.name.Contains("floor") || col.gameObject.name.Contains("Rotating"))
        {
            playerController.isGrounded = true;
        }
    }

    void OnCollisionExit(Collision col)
    {
        if (playerController == null) return;

        if (col.gameObject.name.Contains("floor") || col.gameObject.name.Contains("Rotating"))
        {
            playerController.isGrounded = false;
        }
    }
}