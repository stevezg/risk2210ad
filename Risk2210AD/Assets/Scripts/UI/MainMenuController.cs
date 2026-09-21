using UnityEngine;
using UnityEngine.SceneManagement;
using UnityEngine.UIElements;

namespace Risk2210.UI
{
    public sealed class MainMenuController : MonoBehaviour
    {
        private void Start()
        {
            foreach (var a in System.Environment.GetCommandLineArgs())
                if (a == "-screenshot" || a == "-autostart") { Destroy(this); GameBootstrap.Launch(); return; }
            var doc = UiPanel.Create(gameObject, "MainMenu", "Hud", "MainMenu");
            var root = doc.rootVisualElement;
            var players = root.Q<VisualElement>("players");
            var modes = root.Q<VisualElement>("modes");
            Button[] pb = new Button[4];
            for (int n = 2; n <= 5; n++)
            {
                int count = n;
                var b = new Button { text = n + " players" };
                b.AddToClassList("btn");
                pb[n - 2] = b;
                b.clicked += () => { GameSettings.Players = count; foreach (var x in pb) x.RemoveFromClassList("toggled"); b.AddToClassList("toggled"); };
                players.Add(b);
            }
            pb[GameSettings.Players - 2].AddToClassList("toggled");

            var play = new Button { text = "Play as Red" }; play.AddToClassList("btn");
            var watch = new Button { text = "Spectate bots" }; watch.AddToClassList("btn");
            play.clicked += () => { GameSettings.Spectate = false; play.AddToClassList("toggled"); watch.RemoveFromClassList("toggled"); };
            watch.clicked += () => { GameSettings.Spectate = true; watch.AddToClassList("toggled"); play.RemoveFromClassList("toggled"); };
            (GameSettings.Spectate ? watch : play).AddToClassList("toggled");
            modes.Add(play); modes.Add(watch);

            root.Q<Button>("start").clicked += () =>
            {
                GameSettings.Seed = System.Environment.TickCount;
                if (Application.CanStreamedLevelBeLoaded("MainGame")) SceneManager.LoadScene("MainGame");
                else { Destroy(this); GameBootstrap.Launch(); }
            };
        }
    }
}
