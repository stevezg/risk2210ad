using System.Collections.Generic;
using Risk2210.AI;
using Risk2210.Core;
using Risk2210.UI;
using Risk2210.View;
using UnityEngine;
using UnityEngine.SceneManagement;

namespace Risk2210
{
    /// <summary>
    /// Assembles the whole game at runtime: engine, board, camera, post-processing, HUD, bots.
    /// The scene only needs this component (the scene builder adds it), so nothing depends on hand-authored assets.
    /// </summary>
    public sealed class GameBootstrap : MonoBehaviour
    {
        public GameDirector Director { get; private set; }

        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        private static void AutoBootstrap()
        {
            if (FindFirstObjectByType<GameBootstrap>() != null || FindFirstObjectByType<MainMenuController>() != null) return;
            string scene = SceneManager.GetActiveScene().name;
            if (scene == "MainMenu") new GameObject("MainMenu").AddComponent<MainMenuController>();
            else Launch();
        }

        private static void ParseCommandLine()
        {
            var args = System.Environment.GetCommandLineArgs();
            for (int i = 0; i < args.Length; i++)
            {
                if (args[i] == "-spectate") GameSettings.Spectate = true;
                else if (args[i] == "-players" && i + 1 < args.Length) GameSettings.Players = Mathf.Clamp(int.Parse(args[i + 1]), 2, 5);
                else if (args[i] == "-seed" && i + 1 < args.Length) GameSettings.Seed = int.Parse(args[i + 1]);
                else if (args[i] == "-screenshot" && i + 1 < args.Length) screenshotPath = args[i + 1];
                else if (args[i] == "-screenshot-delay" && i + 1 < args.Length) screenshotDelay = float.Parse(args[i + 1]);
            }
        }

        private static string screenshotPath;
        private static float screenshotDelay = 8f;
        private float screenshotTimer;

        private void Update()
        {
            if (screenshotPath == null) return;
            screenshotTimer += Time.deltaTime;
            if (screenshotTimer > screenshotDelay)
            {
                ScreenCapture.CaptureScreenshot(screenshotPath);
                screenshotPath = null;
                Invoke(nameof(Quit), 1.5f);
            }
        }

        private void Quit() => Application.Quit();

        public static GameBootstrap Launch()
        {
            var go = new GameObject("Risk2210");
            return go.AddComponent<GameBootstrap>();
        }

        private void Start()
        {
            ParseCommandLine();
            Application.runInBackground = true;
            Application.targetFrameRate = 60;
            QualitySettings.vSyncCount = 1;

            // --- engine ---------------------------------------------------------
            var names = new List<string>(); var bots = new List<bool>();
            for (int p = 0; p < GameSettings.Players; p++) { names.Add(GameSettings.Names[p]); bots.Add(GameSettings.Spectate || p != 0); }
            Director = new GameDirector(names, bots, GameSettings.Seed);

            // --- camera + post ----------------------------------------------------
            var camGo = Camera.main != null ? Camera.main.gameObject : new GameObject("Main Camera", typeof(Camera)) { tag = "MainCamera" };
            var cam = camGo.GetComponent<Camera>();
            cam.orthographic = true;
            cam.clearFlags = CameraClearFlags.SolidColor;
            cam.backgroundColor = new Color(0.02f, 0.03f, 0.06f);
            camGo.transform.position = new Vector3(10, -6, -10);
            camGo.transform.rotation = Quaternion.identity;
            if (camGo.GetComponent<AudioListener>() == null) camGo.AddComponent<AudioListener>();
            var tactical = camGo.GetComponent<TacticalCamera>();
            if (tactical == null) tactical = camGo.AddComponent<TacticalCamera>();
            if (camGo.GetComponent<BloomEffect>() == null) camGo.AddComponent<BloomEffect>();

            // --- board ------------------------------------------------------------
            var boardGo = new GameObject("Board");
            var board = boardGo.AddComponent<BoardView>();
            board.Build(Director.Map);
            board.RefreshAll(Director.State, false);
            GridBackground.Create(boardGo.transform, new Rect(-30, -50, 90, 80));
            var earth = Director.Map.Bounds(TerritoryType.Land);
            earth = Rect.MinMaxRect(earth.xMin - 1.5f, Director.Map.Bounds().yMin - 1f, earth.xMax + 1.5f, earth.yMax + 1f);
            tactical.Bounds = Rect.MinMaxRect(earth.xMin, earth.yMin, earth.xMax, earth.yMax);
            tactical.SnapTo(earth.center, 8f);
            // leave room for the side panels (about 300 px each) and the top/bottom bars
            tactical.SetHome(Rect.MinMaxRect(earth.xMin - 5.5f, Director.Map.Bounds(TerritoryType.Land).yMin - 3.5f, earth.xMax + 5.5f, earth.yMax + 1.5f));

            // --- animation --------------------------------------------------------
            var combat = boardGo.AddComponent<CombatOverlay>();
            var anim = boardGo.AddComponent<AnimationDirector>();
            anim.Board = board; anim.Cam = tactical; anim.Combat = combat; anim.State = Director.State;
            Director.OnEvent += anim.Enqueue;

            // --- input + HUD --------------------------------------------------------
            var input = boardGo.AddComponent<MapInputController>();
            input.Board = board; input.Cam = tactical;
            var hudGo = new GameObject("HUD");
            var hud = hudGo.AddComponent<HudController>();
            hud.Director = Director; hud.Board = board; hud.Input = input; hud.Anim = anim;
            input.IsPointerOverUi = hud.IsPointerOverUi;

            // --- bots ---------------------------------------------------------------
            var runner = gameObject.AddComponent<AsyncBotRunner>();
            runner.Bind(Director, GameSettings.Seed);
            runner.IsViewBusy = () => anim.IsBusy;
            hud.Bots = runner;

            hud.Bind();
            Director.Start();
        }
    }
}
