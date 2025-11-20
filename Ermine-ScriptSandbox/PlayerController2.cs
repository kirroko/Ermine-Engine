using ErmineEngine;
using System;
using System.Collections;

public class PlayerController2 : MonoBehaviour
{
    private Transform cam;
    private AudioComponent audioComp;

    public float moveSpeed = 5f;
    public float jumpspeed = 2f;
    private Vector3 jumpvec = new Vector3(0, 1, 0);

    public float mouseSensitivity = 0.01f;

    public float crouchLerpSpeed = 6f;

    private bool isGrounded = true;
    private Vector3 groundNormal = Vector3.up;

    private Vector3 connectionVelocity;

    private float xRotation = 0f;
    private bool isCrouching;
    private float camDefaultY = 200f;
    private float camCrouchY = 50f;

    private bool movementKeyPressed = false;

    private Vector2 moveInput;
    private Vector2 lookInput;
    private Vector3 move;

    // Footstep timing
    private float footstepTimer = 0f;
    private float footstepInterval = 0.5f; // Time between footsteps (adjust this!)

    void Start()
    {
        cam = GameObject.Find("Main Camera").GetComponent<Transform>();

        audioComp = GetComponent<AudioComponent>();
        if (audioComp == null)
        {
            Console.WriteLine("Warning: No AudioComponent found on player!");
        }
        else
        {
            Console.WriteLine("AudioComponent found successfully!");
        }
    }

    void Update()
    {
        moveInput = Vector2.zero;
        movementKeyPressed = false;
        move = Vector3.zero;

        if (Input.GetKeyDown(KeyCode.A))
        {
            //moveInput.x = Vector2.left.x;
            move += transform.right;
            //transform.Translate(new Vector3(1, 0, 0) * Time.deltaTime);
            movementKeyPressed = true;
        }
        if (Input.GetKeyDown(KeyCode.D))
        {
            //moveInput.x = Vector2.right.x;
            move += -transform.right;
            //transform.Translate(new Vector3(-1, 0, 0) * Time.deltaTime);

            movementKeyPressed = true;
        }
        if (Input.GetKeyDown(KeyCode.W))
        {
            //transform.Translate(new Vector3(0, 0, 1) * Time.deltaTime);
            //moveInput.y = Vector2.up.y;
            move += transform.forward;
            movementKeyPressed = true;
        }
        if (Input.GetKeyDown(KeyCode.S))
        {
            //moveInput.y = Vector2.down.y;
            //transform.Translate(new Vector3(0, 0, -1) * Time.deltaTime);
            move += -transform.forward;
            movementKeyPressed = true;
        }

        if (move.SqrMagnitude > 0)
            move = move.normalized * moveSpeed * Time.fixedDeltaTime;

        move = new Vector3(move.x, 0, move.z);

        Vector3 newPos = transform.position + move;

        if (Input.GetKeyDown(KeyCode.Space) && isGrounded)
        {
            isGrounded = false;
            newPos += new Vector3(0, jumpspeed, 0);
            //jumpvec = new Vector3(0, jumpspeed, 0);
        }
        //if(!isGrounded)
        //{
        //    jumpvec += new Vector3(0, -0.981f, 0);
        //    transform.Translate(jumpvec * Time.deltaTime);
        //}

        Physics.SetPosition((ulong)gameObject.GetInstanceID(), transform.position);
        transform.position = newPos;
        //HandleMovement();
        HandleLook();
        HandleCameraLerp();
        HandleFootstepAudio();

        lookInput = Input.mousePositionDelta;
    }

    private void HandleMovement()
    {
        float speed = moveSpeed;
        //if (isSprinting && !isCrouching) speed = sprintSpeed;
        //if (isCrouching) speed = crouchSpeed;

        Vector3 inputWorld = transform.right * -moveInput.x + transform.forward * moveInput.y;

        Vector3 xAxis = Vector3.ProjectOnPlane(transform.right, groundNormal).normalized;
        Vector3 zAxis = Vector3.ProjectOnPlane(transform.forward, groundNormal).normalized;
        Vector3 desiredRelative =
            (xAxis * Vector3.Dot(inputWorld, xAxis) + zAxis * Vector3.Dot(inputWorld, zAxis)).normalized * speed;

        Vector3 horizontalVelocity = connectionVelocity;
        if (desiredRelative.SqrMagnitude > 0f)
            horizontalVelocity += desiredRelative;

        // Move the character in world space using the computed horizontal velocity
        transform.Translate(horizontalVelocity * Time.deltaTime);
        transform.Translate(jumpvec * Time.deltaTime);
        Physics.SetPosition((ulong)gameObject.GetInstanceID(), transform.position);
    }

    private void HandleFootstepAudio()
    {
        if (audioComp == null) return;

        footstepTimer += Time.deltaTime;

        // Play footstep when a movement key is pressed and player is grounded
        if (movementKeyPressed && footstepTimer >= footstepInterval && !audioComp.isPlaying)
        {
            Console.WriteLine("Playing footstep sound - Key pressed and grounded");
            audioComp.shouldPlay = true;
            footstepTimer = 0f;
        }
        else if (movementKeyPressed && !isGrounded)
        {
            Console.WriteLine("Key pressed but NOT grounded - no footstep");
        }
        else if (!movementKeyPressed)
        {
            // This will spam the console, but helps debug
            // Console.WriteLine("No movement key pressed this frame");
        }
    }

    private void HandleLook()
    {
        float mouseX = -lookInput.x * mouseSensitivity;
        float mouseY = lookInput.y * mouseSensitivity;

        xRotation = Mathf.Clamp(xRotation - mouseY, -80f, 80f);

        cam.rotation = Quaternion.Euler(xRotation, 0f, 0f); // camera child handles pitch
        transform.Rotate(Vector3.up * mouseX); // player object handles yaw
    }

    private void HandleCameraLerp()
    {
        float targetY = isCrouching ? camCrouchY : camDefaultY;
        Vector3 camPos = cam.position;
        camPos.y = Mathf.Lerp(camPos.y, targetY, Time.deltaTime * crouchLerpSpeed);
        cam.position = camPos;
    }

    void OnCollisionEnter(Collision col)
    {
        if (col.gameObject.name == "floor")
            isGrounded = true;
    }

    void OnCollisionStay(Collision col)
    {
    }

    void OnCollisionExit(Collision col)
    {
    }
}