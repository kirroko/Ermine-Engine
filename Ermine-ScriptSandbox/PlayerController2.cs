using ErmineEngine;
using System;

public class PlayerController2 : MonoBehaviour
{
    private Transform cam;
    private AudioComponent audioComp;

    public float moveSpeed = 5f;
    public float jumpspeed = 5f;

    public float mouseSensitivity = 0.01f;
    public float crouchLerpSpeed = 6f;

    private bool isGrounded = true;
    private bool isCrouching;

    private float xRotation = 0f;
    private float camDefaultY = 200f;
    private float camCrouchY = 50f;

    private bool movementKeyPressed = false;
    private Vector3 move;
    private Vector2 lookInput;

    private float footstepTimer = 0f;
    private float footstepInterval = 0.5f;

    private float minPitch = -80f;
    private float maxPitch = 80f;

    private bool jumpRequested = false;
    public float jumpHeight = 2f;
    private float startheight = 0;

    void Start()
    {
        cam = GameObject.Find("Main Camera").GetComponent<Transform>();
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
    }

    private void HandleInput()
    {
        move = Vector3.zero;
        movementKeyPressed = false;

        if (Input.GetKeyDown(KeyCode.W)) { move += transform.forward; movementKeyPressed = true; }
        if (Input.GetKeyDown(KeyCode.S)) { move += -transform.forward; movementKeyPressed = true; }
        if (Input.GetKeyDown(KeyCode.A)) { move += transform.right; movementKeyPressed = true; }
        if (Input.GetKeyDown(KeyCode.D)) { move += -transform.right; movementKeyPressed = true; }

        if (move.SqrMagnitude > 0f)
            move = move.normalized * moveSpeed * Time.fixedDeltaTime;

        Vector3 newPos = transform.position + new Vector3(move.x, 0, move.z);

        // Request jump
        if (Input.GetKey(KeyCode.Space) && isGrounded)
        {
            jumpRequested = true;
            isGrounded = false; // prevent double jump
            startheight = transform.position.y;
        }

        // Apply jump once
        if (jumpRequested)
        {
            if (newPos.y < startheight+jumpHeight)
                newPos.y += jumpspeed * Time.fixedDeltaTime; // teleport player slightly up
            else
                jumpRequested = false;
        }

        transform.position = newPos;

        // Sync physics collider
        Physics.SetPosition((ulong)gameObject.GetInstanceID(), transform.position);


        // Sync physics collider with the transform
        Physics.SetPosition((ulong)gameObject.GetInstanceID(), transform.position);
    }

    private void HandleLook()
    {
        lookInput = Input.mousePositionDelta;

        float mouseX = -lookInput.x * mouseSensitivity;
        float mouseY = lookInput.y * mouseSensitivity;

        xRotation -= mouseY;
        xRotation = Mathf.Clamp(xRotation, minPitch, maxPitch);

        cam.rotation = Quaternion.Euler(xRotation, 0f, 0f);
        transform.Rotate(Vector3.up * mouseX);
    }

    private void HandleCameraLerp()
    {
        float targetY = isCrouching ? camCrouchY : camDefaultY;
        Vector3 camPos = cam.position;
        camPos.y = Mathf.Lerp(camPos.y, targetY, Time.fixedDeltaTime * crouchLerpSpeed);
        cam.position = camPos;
    }

    private void HandleFootstepAudio()
    {
        if (audioComp == null) return;

        footstepTimer += Time.fixedDeltaTime;

        if (movementKeyPressed && footstepTimer >= footstepInterval)
        {
            if (!audioComp.isPlaying)
            {
                audioComp.shouldPlay = true;
                footstepTimer = 0f;
            }
        }
    }

    void OnCollisionEnter(Collision col)
    {
        if (col.gameObject.name == "floor")
            isGrounded = true;
    }
}
