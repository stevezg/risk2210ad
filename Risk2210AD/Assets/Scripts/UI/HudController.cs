using System.Collections.Generic;
using System.Linq;
using System.Text;
using Risk2210.Core;
using Risk2210.View;
using UnityEngine;
using UnityEngine.SceneManagement;
using UnityEngine.UIElements;

namespace Risk2210.UI
{
    /// <summary>
    /// The command overlay: dashboards, year track, prompt/action bar, command hand with tooltips,
    /// and the translation of board clicks into engine commands for the human player.
    /// </summary>
    public sealed class HudController : MonoBehaviour
    {
        public GameDirector Director;
        public BoardView Board;
        public MapInputController Input;
        public AnimationDirector Anim;
        public AI.AsyncBotRunner Bots;

        private UIDocument doc;
        private VisualElement root, yearTrack, players, regions, actions, hand, tooltip, gameOver;
        private Label phaseLabel, promptTitle, promptText, status, log, tooltipTitle, tooltipBody;
        private Button pauseBtn, fastBtn;

        // interaction state
        private int selected = -1;
        private int hireType = -1;              // commander index, -2 = space station, -1 = none
        private readonly List<CommanderType> cart = new List<CommanderType>();
        private int moveIn = 1, fortifyN = 1, bidValue;
        private readonly bool[] fortifyCmds = new bool[MapGraph.NumCommanders];
        private readonly int[] setupPicks = { -1, -1, -1 };
        private int setupStep;
        private PhaseId lastPhase = PhaseId.GameOver;
        private int lastActor = -2;
        private bool paused;
        private int lastLogCount = -1;

        private GameState S => Director.State;

        public bool IsPointerOverUi() => UiPanel.IsPointerOver(doc);

        public void Bind()
        {
            doc = UiPanel.Create(gameObject, "Hud", "Hud");
            root = doc.rootVisualElement;
            yearTrack = root.Q<VisualElement>("year-track");
            players = root.Q<VisualElement>("players");
            regions = root.Q<VisualElement>("regions");
            actions = root.Q<VisualElement>("actions");
            hand = root.Q<VisualElement>("hand");
            tooltip = root.Q<VisualElement>("tooltip");
            gameOver = root.Q<VisualElement>("game-over");
            phaseLabel = root.Q<Label>("phase-label");
            promptTitle = root.Q<Label>("prompt-title");
            promptText = root.Q<Label>("prompt-text");
            status = root.Q<Label>("status");
            log = root.Q<Label>("log");
            tooltipTitle = root.Q<Label>("tooltip-title");
            tooltipBody = root.Q<Label>("tooltip-body");
            pauseBtn = root.Q<Button>("pause");
            fastBtn = root.Q<Button>("fast");

            for (int y = 1; y <= Rules.NumYears; y++)
            {
                var l = new Label((2205 + y).ToString());
                l.AddToClassList("year");
                yearTrack.Add(l);
            }
            pauseBtn.clicked += () => { paused = !paused; pauseBtn.text = paused ? "Resume bots" : "Pause bots"; if (Bots != null) Bots.enabled = !paused; };
            fastBtn.clicked += () => { Anim.FastForward = !Anim.FastForward; fastBtn.text = Anim.FastForward ? "Normal speed" : "Fast forward"; if (Bots != null) Bots.PaceSeconds = Anim.FastForward ? 0.02f : 0.35f; };
            root.Q<Button>("menu").clicked += BackToMenu;
            root.Q<Button>("game-over-menu").clicked += BackToMenu;

            Input.Hovered += OnHover;
            Input.Clicked += OnTerritoryClicked;
            Director.OnEvent += _ => Rebuild();
            Rebuild();
        }

        private void BackToMenu()
        {
            if (Application.CanStreamedLevelBeLoaded("MainMenu")) SceneManager.LoadScene("MainMenu");
            else SceneManager.LoadScene(SceneManager.GetActiveScene().buildIndex);
        }

        private void Update()
        {
            if (Director == null) return;
            if (Director.Log.Count != lastLogCount)
            {
                lastLogCount = Director.Log.Count;
                var lines = Director.Log.Skip(Mathf.Max(0, Director.Log.Count - 7));
                log.text = string.Join("\n", lines);
            }
            if (tooltip.style.display == DisplayStyle.Flex)
            {
                Vector2 p = new Vector2(UnityEngine.Input.mousePosition.x, Screen.height - UnityEngine.Input.mousePosition.y);
                p = RuntimePanelUtils.ScreenToPanel(root.panel, p);
                tooltip.style.left = Mathf.Min(p.x + 18, root.resolvedStyle.width - 320);
                tooltip.style.top = Mathf.Min(p.y + 18, root.resolvedStyle.height - 160);
            }
        }

        // ------------------------------------------------------------------
        // Board interaction
        // ------------------------------------------------------------------

        private bool HumanActing => S.ExpectedActor >= 0 && !S.Players[S.ExpectedActor].IsBot;
        private int Me => S.ExpectedActor;

        private void OnHover(int t)
        {
            foreach (var v in Board.Territories) if (v.Highlight == HighlightMode.Hover) v.SetHighlight(HighlightMode.None);
            if (t < 0) { tooltip.style.display = DisplayStyle.None; return; }
            if (Board[t].Highlight == HighlightMode.None) Board[t].SetHighlight(HighlightMode.Hover);
            var node = Board.Map[t]; var ts = S.Territories[t]; var region = Board.Map.Regions[node.RegionId];
            tooltipTitle.text = node.Name;
            var sb = new StringBuilder();
            sb.Append(node.Type).Append(" · ").Append(region.Name).Append(" (+").Append(region.Bonus).Append(")\n");
            if (ts.Devastated) sb.Append("DEVASTATED - impassable");
            else if (ts.Owner < 0) sb.Append("Unoccupied");
            else
            {
                sb.Append(Director.Name(ts.Owner)).Append(": ").Append(ts.Mods).Append(" MODs");
                for (int c = 0; c < MapGraph.NumCommanders; c++) if (ts.Commanders[c]) sb.Append(" + ").Append((CommanderType)c);
                if (ts.SpaceStation) sb.Append(" · Space Station");
            }
            if (node.LunarLandingSite) sb.Append("\nLunar landing site");
            tooltipBody.text = sb.ToString();
            tooltip.style.display = DisplayStyle.Flex;
        }

        private void OnTerritoryClicked(int t)
        {
            if (!HumanActing) return;
            int me = Me;
            var ts = S.Territories[t];
            CommandResult r = CommandResult.Success;
            bool handled = true;

            if (S.Prompt != null)
            {
                if (S.Prompt.Kind == PromptKind.ChooseTerritory && S.Prompt.Options.Contains(t)) r = Submit(new RespondPrompt { Value = t });
                return;
            }
            switch (S.Phase)
            {
                case PhaseId.Setup:
                    if (S.SetupStage == SetupStage.Claim) r = Submit(new ClaimTerritory { Territory = t });
                    else if (S.SetupStage == SetupStage.PlaceMods) r = Submit(new PlaceStartingMod { Territory = t });
                    else if (S.SetupStage == SetupStage.Pieces)
                    {
                        if (ts.Owner != me) { SetStatus("Choose a territory you control"); return; }
                        setupPicks[setupStep++] = t;
                        if (setupStep == 3)
                        {
                            r = Submit(new PlaceStartingPieces { SpaceStation = setupPicks[0], LandCommander = setupPicks[1], Diplomat = setupPicks[2] });
                            setupStep = 0;
                        }
                    }
                    break;
                case PhaseId.Deployment:
                    if (ts.Owner != me) { SetStatus("Choose a territory you control"); return; }
                    if (S.Players[me].Pool > 0) { selected = t; r = Submit(new DeployMods { Territory = t, Count = 1 }); }
                    else if (hireType == -2) { r = Submit(new BuildSpaceStation { Territory = t }); if (r.Ok) hireType = -1; }
                    else if (hireType >= 0) { r = Submit(new HireCommander { Commander = (CommanderType)hireType, Territory = t }); if (r.Ok) hireType = -1; }
                    else selected = t;
                    break;
                case PhaseId.Invasion:
                    if (S.Invasion.Active) return;
                    if (ts.Owner == me) selected = t;
                    else if (selected >= 0) { r = Submit(new DeclareInvasion { From = selected, To = t }); if (r.Ok) moveIn = 1; }
                    break;
                case PhaseId.Fortification:
                    if (ts.Owner != me) return;
                    if (selected < 0 || selected == t) { selected = t; fortifyN = Mathf.Max(1, ts.Units - 1); System.Array.Clear(fortifyCmds, 0, fortifyCmds.Length); }
                    else
                    {
                        int nCmd = fortifyCmds.Count(b => b);
                        r = Submit(new Fortify { From = selected, To = t, Mods = fortifyN - nCmd, Commanders = (bool[])fortifyCmds.Clone() });
                    }
                    break;
                default:
                    handled = false;
                    break;
            }
            if (handled) SetStatus(r.Ok ? "" : r.Error);
            Rebuild();
        }

        private CommandResult Submit(GameCommand cmd)
        {
            cmd.Player = Me;
            var r = Director.Submit(cmd);
            SetStatus(r.Ok ? "" : r.Error);
            return r;
        }

        private void SetStatus(string s) => status.text = s ?? "";

        // ------------------------------------------------------------------
        // Rebuild all dynamic UI from the engine state
        // ------------------------------------------------------------------

        public void Rebuild()
        {
            if (Director == null || root == null) return;
            if (S.Phase != lastPhase || S.ExpectedActor != lastActor)
            {
                lastPhase = S.Phase; lastActor = S.ExpectedActor;
                selected = -1; hireType = -1; cart.Clear(); setupStep = 0; moveIn = 1;
                if (S.Phase == PhaseId.EnergyBidding) bidValue = 0;
                SetStatus("");
            }
            Anim.State = S;
            BuildYearTrack();
            BuildPlayers();
            BuildRegions();
            BuildPrompt();
            BuildHand();
            BuildHighlights();
            phaseLabel.text = S.Phase.ToString().ToUpperInvariant();
            if (S.Phase == PhaseId.GameOver && gameOver.style.display != DisplayStyle.Flex)
            {
                gameOver.style.display = DisplayStyle.Flex;
                var sb = new StringBuilder();
                foreach (int p in Enumerable.Range(0, S.NumPlayers).OrderByDescending(p => S.Players[p].FinalScore))
                    sb.Append(Director.Name(p)).Append("  ").Append(S.Players[p].FinalScore).Append('\n');
                root.Q<Label>("game-over-body").text = sb.ToString();
                root.Q<Label>("game-over-title").text = Director.Name(S.Winner).ToUpperInvariant() + " IS ELECTED WORLD LEADER";
            }
        }

        private void BuildYearTrack()
        {
            for (int i = 0; i < yearTrack.childCount; i++)
            {
                var l = yearTrack[i];
                l.RemoveFromClassList("active"); l.RemoveFromClassList("done");
                if (i + 1 == S.Year) l.AddToClassList("active");
                else if (i + 1 < S.Year) l.AddToClassList("done");
            }
        }

        private void BuildPlayers()
        {
            players.Clear();
            for (int p = 0; p < S.NumPlayers; p++)
            {
                var ps = S.Players[p];
                var row = new VisualElement(); row.AddToClassList("player-row");
                if (S.CurrentPlayer == p) row.AddToClassList("current");
                var sw = new VisualElement(); sw.AddToClassList("swatch");
                var c = BoardView.PlayerColor(p); sw.style.backgroundColor = new Color(c.r, c.g, c.b, 1);
                var name = new Label(ps.Name + (ps.IsBot ? "" : " (you)") + (ps.Eliminated ? " ✕" : "")); name.AddToClassList("player-name");
                var cmds = new StringBuilder();
                for (int k = 0; k < MapGraph.NumCommanders; k++) if (S.CommanderInPlay(p, (CommanderType)k)) cmds.Append("LDNXS"[k]);
                var stats = new Label($"E {ps.Energy}  T {S.CountTerritories(p)}  S {S.Score(p)}  ★{S.CountSpaceStations(p)}  ▮{ps.Hand.Count}  [{cmds}]"); stats.AddToClassList("player-stats");
                row.Add(sw); row.Add(name); row.Add(stats);
                players.Add(row);
            }
        }

        private void BuildRegions()
        {
            regions.Clear();
            foreach (var r in Board.Map.Regions)
            {
                int owner = -1;
                for (int p = 0; p < S.NumPlayers; p++) if (S.ControlsRegion(p, r.Id)) { owner = p; break; }
                var row = new VisualElement(); row.AddToClassList("region-row");
                var l = new Label($"{r.Name}  +{r.Bonus}");
                var rc = BoardView.RegionColor(r.Id); l.style.color = rc;
                var o = new Label(owner >= 0 ? Director.Name(owner) : "—");
                if (owner >= 0) o.style.color = BoardView.PlayerColor(owner);
                row.Add(l); row.Add(o);
                regions.Add(row);
            }
        }

        private Button Btn(string text, System.Action onClick, bool enabled = true, string cls = null)
        {
            var b = new Button { text = text };
            b.AddToClassList("btn");
            if (cls != null) b.AddToClassList(cls);
            b.SetEnabled(enabled);
            b.clicked += () => { onClick(); Rebuild(); };
            actions.Add(b);
            return b;
        }

        private void BuildPrompt()
        {
            actions.Clear();
            int actor = S.ExpectedActor;
            if (S.Phase == PhaseId.GameOver) { promptTitle.text = "Game over"; promptText.text = ""; return; }
            if (actor < 0) { promptTitle.text = "..."; promptText.text = ""; return; }
            if (S.Players[actor].IsBot)
            {
                promptTitle.text = Director.Name(actor) + " is thinking";
                promptText.text = S.Prompt != null ? S.Prompt.Text : S.Phase.ToString();
                return;
            }
            int me = actor;
            var ps = S.Players[me];
            promptTitle.text = ps.Name + " - your move";

            if (S.Prompt != null)
            {
                var pr = S.Prompt;
                promptText.text = pr.Text;
                switch (pr.Kind)
                {
                    case PromptKind.DefenseDice:
                        for (int d = 1; d <= pr.MaxDice; d++) { int dd = d; Btn("Defend with " + d, () => Submit(new RespondPrompt { Value = dd })); }
                        break;
                    case PromptKind.ReactiveCard:
                        promptText.text += "\nClick a highlighted card in your hand, or pass.";
                        Btn("Pass", () => Submit(new RespondPrompt { Value = -1 }));
                        break;
                    case PromptKind.BonusDeck:
                        for (int i = 0; i < pr.Decks.Count; i++) { int ii = i; Btn(pr.Decks[i].ToString(), () => Submit(new RespondPrompt { Value = ii })); }
                        break;
                    case PromptKind.ChooseTerritory:
                        promptText.text += "\nClick a highlighted territory.";
                        break;
                }
                return;
            }

            switch (S.Phase)
            {
                case PhaseId.Setup:
                    if (S.SetupStage == SetupStage.Claim) promptText.text = "Claim an unoccupied land territory.";
                    else if (S.SetupStage == SetupStage.PlaceMods) promptText.text = $"Place 1 MOD on a territory you control ({ps.Pool} left).";
                    else
                    {
                        string[] steps = { "your SPACE STATION", "your LAND COMMANDER", "your DIPLOMAT" };
                        promptText.text = "Click a territory for " + steps[setupStep] + ".";
                    }
                    break;
                case PhaseId.EnergyBidding:
                    if (S.BidChooser == me)
                    {
                        promptText.text = "Choose your turn order marker.";
                        foreach (int m in S.AvailableMarkers) { int mm = m; Btn("#" + (m + 1), () => Submit(new ChooseTurnOrder { Marker = mm })); }
                    }
                    else
                    {
                        bidValue = Mathf.Clamp(bidValue, 0, ps.Energy);
                        promptText.text = $"Secret bid for turn order: {bidValue} of {ps.Energy} energy.";
                        Btn("-", () => bidValue--, bidValue > 0);
                        Btn("+", () => bidValue++, bidValue < ps.Energy);
                        Btn("Confirm bid", () => Submit(new SubmitBid { Amount = bidValue }), true, "primary");
                    }
                    break;
                case PhaseId.Deployment:
                    if (ps.Pool > 0)
                    {
                        promptText.text = $"{ps.Pool} MODs to deploy. Click your territory (+1) or select one and use the buttons.";
                        Btn("+5", () => Submit(new DeployMods { Territory = selected, Count = 5 }), selected >= 0 && ps.Pool >= 5);
                        Btn("All", () => Submit(new DeployMods { Territory = selected, Count = ps.Pool }), selected >= 0);
                    }
                    else
                    {
                        promptText.text = $"Energy {ps.Energy}. Hire commanders (3E), build a Space Station (5E), buy cards (1E), play cards.";
                        for (int c = 0; c < MapGraph.NumCommanders; c++)
                        {
                            int cc = c;
                            bool have = S.CommanderInPlay(me, (CommanderType)c);
                            Btn("Hire " + (CommanderType)c, () => hireType = hireType == cc ? -1 : cc, !have && ps.Energy >= Rules.CommanderCost, hireType == c ? "toggled" : null);
                        }
                        Btn("Space Station (5E)", () => hireType = hireType == -2 ? -1 : -2, ps.Energy >= Rules.SpaceStationCost && S.CountSpaceStations(me) < Rules.MaxSpaceStations, hireType == -2 ? "toggled" : null);
                        for (int c = 0; c < MapGraph.NumCommanders; c++)
                        {
                            var ct = (CommanderType)c;
                            bool ok = S.CommanderInPlay(me, ct) && Director.DeckSize(ct) > 0 && cart.Count + ps.CardsBoughtThisTurn < Rules.MaxCardsPerTurn && cart.Count < ps.Energy;
                            Btn($"+{ct} card ({Director.DeckSize(ct)})", () => cart.Add(ct), ok);
                        }
                        if (cart.Count > 0)
                        {
                            Btn($"Buy {cart.Count} card(s): {string.Join(" ", cart.Select(c => c.ToString()[0]))}", () => { Submit(new BuyCards { Decks = new List<CommanderType>(cart) }); cart.Clear(); }, true, "primary");
                            Btn("Clear cart", () => cart.Clear());
                        }
                        Btn("End deployment", () => Submit(new EndDeployment()), true, "primary");
                    }
                    break;
                case PhaseId.Invasion:
                {
                    var inv = S.Invasion;
                    if (!inv.Active)
                    {
                        promptText.text = selected >= 0 ? $"From {Board.Map[selected].Name}: click an orange target." : "Click one of your territories to attack from, then a target.";
                        Btn("End invasions", () => Submit(new EndInvasionPhase()), true, "primary");
                    }
                    else if (!inv.Captured)
                    {
                        int maxDice = Mathf.Min(3, S.Territories[inv.From].Units - 1);
                        promptText.text = $"{Board.Map[inv.From].Name} ({S.Territories[inv.From].Units}) vs {Board.Map[inv.To].Name} ({S.Territories[inv.To].Units})";
                        for (int d = 1; d <= 3; d++) { int dd = d; Btn("Roll " + d, () => Submit(new Attack { Dice = dd }), d <= maxDice, "danger"); }
                        Btn("Retreat", () => Submit(new EndInvasion()), inv.AttackedOnce);
                    }
                    else
                    {
                        int maxMove = S.Territories[inv.From].Units - 1;
                        int minMove = Mathf.Max(1, Mathf.Min(inv.LastDice, maxMove));
                        moveIn = Mathf.Clamp(moveIn, minMove, Mathf.Max(minMove, maxMove));
                        promptText.text = $"Captured {Board.Map[inv.To].Name}! Move in {moveIn} unit(s) (min {minMove}, max {maxMove}).";
                        Btn("-", () => moveIn--, moveIn > minMove);
                        Btn("+", () => moveIn++, moveIn < maxMove);
                        Btn("Max", () => moveIn = maxMove);
                        Btn("Move in", () => { int keep = inv.To; if (Submit(new MoveIn { Count = moveIn }).Ok) selected = keep; }, true, "primary");
                    }
                    break;
                }
                case PhaseId.Fortification:
                    if (selected >= 0)
                    {
                        var f = S.Territories[selected];
                        int maxN = Mathf.Max(0, f.Units - 1);
                        fortifyN = Mathf.Clamp(fortifyN, 1, Mathf.Max(1, maxN));
                        promptText.text = $"From {Board.Map[selected].Name}: move {fortifyN} unit(s). Click a blue destination.";
                        Btn("-", () => fortifyN--, fortifyN > 1);
                        Btn("+", () => fortifyN++, fortifyN < maxN);
                        for (int c = 0; c < MapGraph.NumCommanders; c++)
                            if (f.Commanders[c]) { int cc = c; Btn("LDNXS"[c].ToString(), () => fortifyCmds[cc] = !fortifyCmds[cc], true, fortifyCmds[c] ? "toggled" : null); }
                    }
                    else promptText.text = "Fortify once: click the source territory, then the destination. Or skip.";
                    Btn("Skip fortify", () => Submit(new EndFortification()), true, "primary");
                    break;
            }
        }

        private void BuildHand()
        {
            hand.Clear();
            int actor = S.ExpectedActor;
            int me = -1;
            for (int p = 0; p < S.NumPlayers; p++) if (!S.Players[p].IsBot) { me = p; break; }
            if (me < 0) return;
            var ps = S.Players[me];
            bool reactive = S.Prompt != null && S.Prompt.Kind == PromptKind.ReactiveCard && actor == me;
            bool canPlay = actor == me && S.Prompt == null && (S.Phase == PhaseId.Deployment && ps.Pool == 0 || S.Phase == PhaseId.Invasion && !ps.InvasionDeclaredThisTurn);
            for (int i = 0; i < ps.Hand.Count; i++)
            {
                int idx = i;
                var c = CardCatalogue.Get(ps.Hand[i]);
                bool usable = reactive ? c.Timing == CardTiming.OnInvasionDeclared : canPlay && c.Timing == CardTiming.BeforeFirstInvasion;
                usable &= S.CommanderInPlay(me, c.Deck) && ps.Energy >= c.Cost;
                var card = new VisualElement(); card.AddToClassList("card");
                if (usable) card.AddToClassList("playable");
                var head = new VisualElement(); head.AddToClassList("row");
                var icon = new Label(c.Deck.ToString().Substring(0, 1)); icon.AddToClassList("card-icon"); icon.AddToClassList("deck-" + c.Deck);
                var name = new Label(c.Name); name.AddToClassList("card-name");
                head.Add(icon); head.Add(name);
                var meta = new Label($"{c.Deck.ToString().ToUpperInvariant()} DECK · {c.Cost} ENERGY · {TimingText(c.Timing)}"); meta.AddToClassList("card-meta");
                card.Add(head); card.Add(meta);
                card.RegisterCallback<MouseEnterEvent>(_ => ShowCardTooltip(c, usable, me));
                card.RegisterCallback<MouseLeaveEvent>(_ => tooltip.style.display = DisplayStyle.None);
                if (usable) card.RegisterCallback<ClickEvent>(_ =>
                {
                    if (reactive) Submit(new RespondPrompt { Value = idx }); else Submit(new PlayCard { HandIndex = idx });
                    tooltip.style.display = DisplayStyle.None;
                    Rebuild();
                });
                hand.Add(card);
            }
        }

        private static string TimingText(CardTiming t) => t == CardTiming.BeforeFirstInvasion ? "BEFORE FIRST INVASION" : t == CardTiming.OnInvasionDeclared ? "REACTIVE" : "FINAL SCORING";

        private void ShowCardTooltip(CardDef c, bool usable, int me)
        {
            tooltipTitle.text = c.Name;
            string why = usable ? "Playable now." : !S.CommanderInPlay(me, c.Deck) ? $"Needs your {c.Deck} Commander in play." : S.Players[me].Energy < c.Cost ? "Not enough energy." : "Cannot be played at this moment.";
            tooltipBody.text = c.Text + "\n\n" + why;
            tooltip.style.display = DisplayStyle.Flex;
        }

        private void BuildHighlights()
        {
            foreach (var m in new[] { HighlightMode.Selected, HighlightMode.Option, HighlightMode.Target, HighlightMode.FortifyTarget }) Board.ClearHighlights(m);
            if (!HumanActing) return;
            int me = Me;
            if (S.Prompt != null && S.Prompt.Kind == PromptKind.ChooseTerritory) { foreach (int t in S.Prompt.Options) Board[t].SetHighlight(HighlightMode.Option); return; }
            if (S.Phase == PhaseId.Setup)
            {
                if (S.SetupStage == SetupStage.Claim)
                {
                    foreach (var v in Board.Territories)
                        if (v.Node.Type == TerritoryType.Land && S.Territories[v.Id].Owner == -1 && !S.Territories[v.Id].Devastated) v.SetHighlight(HighlightMode.Option);
                }
                else
                {
                    foreach (int t in S.OwnedTerritories(me)) Board[t].SetHighlight(HighlightMode.Option);
                }
                return;
            }
            if (selected >= 0) Board[selected].SetHighlight(HighlightMode.Selected);
            if (S.Phase == PhaseId.Invasion && selected >= 0 && !S.Invasion.Active)
                for (int t = 0; t < Board.Map.Count; t++) if (t != selected && S.CanInvade(me, selected, t)) Board[t].SetHighlight(HighlightMode.Target);
            if (S.Phase == PhaseId.Fortification && selected >= 0)
                for (int t = 0; t < Board.Map.Count; t++) if (t != selected && S.FortifyPathExists(me, selected, t)) Board[t].SetHighlight(HighlightMode.FortifyTarget);
            if (S.Phase == PhaseId.Deployment && hireType != -1)
                foreach (int t in S.OwnedTerritories(me)) if (hireType != -2 || (Board.Map[t].Type == TerritoryType.Land && !S.Territories[t].SpaceStation)) Board[t].SetHighlight(HighlightMode.Option);
        }
    }
}
