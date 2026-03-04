using ErmineEngine;

public class CutsceneVideo: MonoBehaviour
{
    public string nextScenePath = "../Resources/Scenes/m4-test_copy_copy.scene";
    public string test = "yes";

    private const string VideoName = "intro_cinematic";
    private const string VideoPath = "../Resources/Videos/IntroCinematic_SFX.mpeg";

    private bool finished = false;
    private int frameCounter = 0;

    void Start()
    {
        if (VideoManager.Load(VideoName, VideoPath, false))
        {
            VideoManager.SetCurrent(VideoName);
            VideoManager.SetFitMode(VideoFitMode.StretchToFill);
            VideoManager.SetRenderEnabled(true);
            VideoManager.Play();
        }
        else
        {
            Debug.LogError("CutsceneVideo: Failed to load video '" + VideoPath + "'.");
        }
    }

    void Update()
    {
        frameCounter++;

        // print every ~60 frames
        if (frameCounter % 60 == 0)
        {
            Debug.Log("[CutsceneVideo] Update running. finished=" + finished);
        }


        if (finished) return;

        bool done = VideoManager.IsDonePlaying(VideoName);

        if (frameCounter % 30 == 0)
        {
            Debug.Log("[CutsceneVideo] IsDonePlaying = " + done);
        }

        if (done)
        {
            Debug.Log("[CutsceneVideo] Video finished!");

            finished = true;

            Debug.Log("[CutsceneVideo] Stopping video");
            VideoManager.Stop();

            Debug.Log("[CutsceneVideo] Freeing video");
            VideoManager.Free(VideoName);

            Debug.Log("[CutsceneVideo] Loading next scene: " + nextScenePath);
            SceneManager.LoadScene(nextScenePath);
        }
    }

    void OnDestroy()
    {
        VideoManager.Stop();
        VideoManager.Free(VideoName);
    }
}
