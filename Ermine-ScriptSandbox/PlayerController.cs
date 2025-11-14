using System;
using ErmineEngine;

public class PlayerController : MonoBehaviour
{
    private Transform cam;

    public float moveSpeed = 5f;
    public float sprintSpeed = 8f;
    public float crouchSpeed = 2f;
    public float jumpHeight = 2f;
    public float gravity = -9.81f;

    public float mouseSensitivity = 0.01f;

    public float crouchLerpSpeed = 6f;

    private bool wasGrounded;
    private bool isGrounded;
    private Vector3 groundNormal = Vector3.up;

    // Moving-ground tracking (Catlike-style)
    private Rigidbody connectedBody, previousConnectedBody;
    private Vector3 connectionWorldPosition, connectionLocalPosition;
    private Vector3 connectionVelocity;     // platform velocity at our contact point (this frame)

    private Vector2 moveInput;
    private Vector2 lookInput;
    private float xRotation = 0f;
    private float yRotation = 0f;
    private Vector3 velocity;                    // vertical velocity is used; horizontal is per-frame input
    private bool isSprinting;
    private bool isCrouching;
    private float camDefaultY = 200f;
    private float camCrouchY = 50f;

    void Start()
    {
        cam = GameObject.Find("Main Camera").GetComponent<Transform>();
        //cam = GetComponent<Transform>();
    }

    void Update()
    {
        moveInput = Vector2.zero;
        //lookInput = Vector2.zero;

        if (Input.GetKeyDown(KeyCode.A))
            moveInput.x = Vector2.left.x;
        if (Input.GetKeyDown(KeyCode.D))
            moveInput.x = Vector2.right.x;
        if(Input.GetKeyDown(KeyCode.W))
            moveInput.y = Vector2.up.y;
        if(Input.GetKeyDown(KeyCode.S))
            moveInput.y = Vector2.down.y;

        lookInput = Input.mousePositionDelta;

        HandleMovement();
        HandleLook();
        HandleCameraLerp();

        wasGrounded = isGrounded;
        //if (Input.GetMouseButton(0))
        //{
        //    var projectile = Prefab.Instantiate("../Resources/Prefabs/Sphere.prefab");
        //    if (projectile != null)
        //    {
        //        projectile.transform.position = transform.position + origin;
        //        projectile.transform.rotation = transform.rotation;
        //    }
        //}

        //if (Input.GetMouseButton(1))
        //{
        //    // Swap position with ball and destroy it
        //    GameObject sphere = GameObject.Find("Sphere");
        //    if (sphere == null)
        //        return;
        //    transform.position = sphere.transform.position;
        //    GameObject.Destroy(sphere);
        //}
    }

    private void HandleMovement()
    {
        if (isGrounded && velocity.y < 0f)
            velocity.y = -2f;

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
        //velocity.y += gravity * Time.deltaTime;
        //transform.Translate(velocity * Time.deltaTime);
    }

    private void HandleLook()
    {
        float mouseX = -lookInput.x * mouseSensitivity;
        float mouseY = lookInput.y * mouseSensitivity;

        xRotation = Mathf.Clamp(xRotation - mouseY, -80f, 80f);
        yRotation += mouseX;

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
}