using ErmineEngine;
using System;

public class PlayerController2 : MonoBehaviour
{
    private Transform cam;
    private AudioComponent audioComp;

    public float mouseHorSens = 1f;
    public float mouseVertSens = 1f;

    public float moveSpeed = 5f;
    public float jumpspeed = 5f;

    public float crouchLerpSpeed = 6f;

    private bool isGrounded = true;
    private bool isCrouching = false;

    private float xRotation = 0f;
    private float camDefaultY = 200f;
    private float camCrouchY = 50f;

    private bool movementKeyPressed = false;
    private Vector3 move;
    private Vector2 lookInput;

    private float footstepTimer = 0f;
    private float footstepInterval = 0.5f;

    private float minPitch = -1.5f;
    private float maxPitch = 1.5f;

    public float jumpHeight = 2f;

    private float interactRange = 5f;

    void Start()
    {
        cam = GameObject.Find("Main Camera").GetComponent<Transform>();
        HandleCameraLerp();
        audioComp = GetComponent<AudioComponent>();
        if (audioComp == null)
            Console.WriteLine("Warning: No AudioComponent found on player!");
    }

    void Update()
    {
        HandleInput();
        HandleLook();
        HandleCameraLerp();
        HandleFootstepAudio();
        HandleInteract();
    }
    float verticalVelocity;
    private void HandleInput()
    {
        move = Vector3.zero;
        movementKeyPressed = false;

        if (Input.GetKeyDown(KeyCode.W)) { move += transform.forward; movementKeyPressed = true; }
        if (Input.GetKeyDown(KeyCode.S)) { move += -transform.forward; movementKeyPressed = true; }
        if (Input.GetKeyDown(KeyCode.A)) { move += transform.right; movementKeyPressed = true; }
        if (Input.GetKeyDown(KeyCode.D)) { move += -transform.right; movementKeyPressed = true; }

        if (move.SqrMagnitude > 0f)
            move = move.normalized * moveSpeed * Time.deltaTime;

        Vector3 newPos = transform.position + new Vector3(move.x, 0, move.z);

        // Request jump
        if (isGrounded == true && Input.GetKey(KeyCode.Space))
        {
            isGrounded = false; // prevent double jump
            Physics.Jump((ulong)gameObject.GetInstanceID(), jumpspeed);
        }

        transform.position = newPos;

        // Sync physics collider
        Physics.SetPosition((ulong)gameObject.GetInstanceID(), transform.position);
    }

    private void HandleLook()
    {
        lookInput = Input.mousePositionDelta;

        float mouseX = -lookInput.x * mouseHorSens * Time.deltaTime;
        float mouseY = lookInput.y * mouseVertSens * Time.deltaTime;

        // rotate player horizontally
        transform.Rotate(Vector3.up * mouseX);
        Physics.SetRotationQuat((ulong)gameObject.GetInstanceID(), transform.rotation);

        // clamp vertical look
        xRotation -= mouseY;
        xRotation = Mathf.Clamp(xRotation, minPitch, maxPitch);

        cam.rotation = Quaternion.Euler(xRotation, 0f, 0f);
    }

    private void HandleCameraLerp()
    {
        float targetY = isCrouching ? camCrouchY : camDefaultY;
        Vector3 camPos = cam.position;
        camPos.y = Mathf.Lerp(camPos.y, targetY, Time.deltaTime * crouchLerpSpeed);
        cam.position = camPos;
    }

    private void HandleFootstepAudio()
    {
        if (audioComp == null) return;

        footstepTimer += Time.deltaTime;

        if (movementKeyPressed && footstepTimer >= footstepInterval)
        {
            if (!audioComp.isPlaying)
            {
                audioComp.shouldPlay = true;
                footstepTimer = 0f;
            }
        }
    }

    private void HandleInteract()
    {
        if (Input.GetKeyDown(KeyCode.E))
        {
            Physics.RaycastHit hit;

            bool hitSomething = Physics.Raycast(
                gameObject.transform.position + new Vector3(0, cam.position.y * 0.01f, 0),
                cam.forward,
                out hit,
                interactRange
            );

            if (!hitSomething) return;

            ulong id = hit.entityID;

            if (id != 0)
            {
                GameObject obj = GameObject.FromEntityID(id);
                //Debug.Log("You are looking at: " + obj.name);

                if (obj.name == "Switch")
                {
                    // Play switch audio here Kai
                }
                if (obj.name == "Book")
                {
                    // Collect book
                }
            }
            /*else
            {
                Debug.Log("No valid entity hit.");
            }*/
        }
    }

    private ulong GetEntityID(Physics.RaycastHit hit)
    {
        var field = typeof(Physics.RaycastHit).GetField(
            "entityID",
            System.Reflection.BindingFlags.Public |
            System.Reflection.BindingFlags.Instance);

        return (ulong)field.GetValue(hit);
    }

    void OnCollisionEnter(Collision col)
    {
        if (col.gameObject.name.Contains("Platform"))
            isGrounded = true;
    }
    void OnCollisionStay(Collision col)
    {
        if (col.gameObject.name.Contains("Platform"))
            isGrounded = true;
    }
    void OnCollisionExit(Collision col)
    {
        if (col.gameObject.name.Contains("Platform"))
            isGrounded = false;
    }
}
