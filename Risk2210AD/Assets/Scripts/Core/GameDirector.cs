using System;
using System.Collections.Generic;
using System.Linq;
using Risk2210.Core.States;

namespace Risk2210.Core
{
    /// <summary>
    /// Owns the game state and the phase state machine. The view and the bots talk to it only through
    /// <see cref="Submit"/>; it answers with a <see cref="CommandResult"/> and appends <see cref="GameEvent"/>s
    /// for the animation layer. No Unity dependencies beyond UnityEngine.Vector2 in the map.
    /// </summary>
    public sealed class GameDirector
    {
        public GameState State { get; private set; }
        public PhaseState Current { get; private set; }
        public readonly List<GameEvent> Events = new List<GameEvent>();
        public readonly List<string> Log = new List<string>();
        public readonly Random Rng;
        public event Action<GameEvent> OnEvent;

        public MapGraph Map => State.Map;

        public GameDirector(IList<string> playerNames, IList<bool> isBot, int seed)
        {
            if (playerNames.Count < 2 || playerNames.Count > 5) throw new ArgumentException("Risk 2210 supports 2-5 players");
            Rng = new Random(seed);
            State = new GameState { Map = MapGraph.CreateStandard(), NumPlayers = playerNames.Count };
            State.Territories = new TerritoryState[State.Map.Count];
            for (int i = 0; i < State.Territories.Length; i++) State.Territories[i] = new TerritoryState();
            for (int p = 0; p < playerNames.Count; p++) State.Players.Add(new PlayerState { Name = playerNames[p], IsBot = isBot[p] });
            if (playerNames.Count == 2) State.Players.Add(new PlayerState { Name = "Neutral", IsNeutral = true, IsBot = true, Energy = 0 });

            for (int c = 0; c < MapGraph.NumCommanders; c++) { State.CommandDecks[c] = CardCatalogue.BuildDeck((CommanderType)c); Shuffle(State.CommandDecks[c]); }
            for (int t = 0; t < 3; t++) { State.TerritoryDecks[t] = State.Map.TerritoriesOfType((TerritoryType)t); Shuffle(State.TerritoryDecks[t]); }
        }

        public void Start()
        {
            Emit(GameEvent.Make(GameEventType.GameStarted));
            Transition(new SetupState());
        }

        // ------------------------------------------------------------------
        // Command entry point
        // ------------------------------------------------------------------

        public CommandResult Submit(GameCommand cmd)
        {
            if (State.Phase == PhaseId.GameOver) return CommandResult.Fail("the game is over");
            if (State.Prompt != null)
            {
                if (!(cmd is RespondPrompt r)) return CommandResult.Fail("waiting for " + State.Players[State.Prompt.Player].Name + " to answer a prompt");
                if (cmd.Player != State.Prompt.Player) return CommandResult.Fail("not your prompt");
                return ResolvePrompt(r);
            }
            if (cmd is RespondPrompt) return CommandResult.Fail("no prompt is open");
            int expected = State.ExpectedActor;
            if (expected >= 0 && cmd.Player != expected && !(State.Phase == PhaseId.EnergyBidding && cmd is SubmitBid))
                return CommandResult.Fail("not your turn");
            return Current.Handle(cmd);
        }

        public void Transition(PhaseState next)
        {
            Current?.Exit();
            Current = next;
            next.Bind(this);
            State.Phase = next.Id;
            var ev = GameEvent.Make(GameEventType.PhaseChanged); ev.Phase = next.Id; ev.Player = State.CurrentPlayer;
            Emit(ev);
            next.Enter();
        }

        // ------------------------------------------------------------------
        // Prompts
        // ------------------------------------------------------------------

        public void OpenPrompt(Prompt p)
        {
            State.Prompt = p;
            var ev = GameEvent.Make(GameEventType.PromptOpened, p.Text); ev.Player = p.Player;
            Emit(ev);
        }

        private CommandResult ResolvePrompt(RespondPrompt r)
        {
            var p = State.Prompt;
            switch (p.Kind)
            {
                case PromptKind.DefenseDice:
                    if (r.Value < 1 || r.Value > p.MaxDice) return CommandResult.Fail("defend with 1-" + p.MaxDice + " dice");
                    break;
                case PromptKind.ReactiveCard:
                    if (r.Value >= 0)
                    {
                        if (r.Value >= State.Players[p.Player].Hand.Count) return CommandResult.Fail("no such card");
                        var c = CardCatalogue.Get(State.Players[p.Player].Hand[r.Value]);
                        if (c.Timing != CardTiming.OnInvasionDeclared) return CommandResult.Fail("not a reactive card");
                        if (!State.CommanderInPlay(p.Player, c.Deck)) return CommandResult.Fail(c.Deck + " Commander is not in play");
                        if (State.Players[p.Player].Energy < c.Cost) return CommandResult.Fail("not enough energy");
                        if (c.Kind == CardKind.StealthMods && Map[State.Invasion.To].Type != c.Target) return CommandResult.Fail("wrong territory type for Stealth MODs");
                        if (c.Kind == CardKind.CeaseFire && State.Invasion.Defender != p.Player) return CommandResult.Fail("only the defender may play Cease Fire");
                    }
                    break;
                case PromptKind.BonusDeck:
                    if (r.Value < 0 || r.Value >= p.Decks.Count) return CommandResult.Fail("choose a listed deck");
                    break;
                case PromptKind.ChooseTerritory:
                case PromptKind.ClaimTerritory:
                case PromptKind.PlaceStartingMod:
                    if (!p.Options.Contains(r.Value)) return CommandResult.Fail("choose a highlighted territory");
                    break;
                case PromptKind.PlaceStartingPieces:
                    if (!p.Options.Contains(r.Value) || !p.Options.Contains(r.Value2) || !p.Options.Contains(r.Value3)) return CommandResult.Fail("place pieces on territories you control");
                    break;
            }
            State.Prompt = null;
            Emit(GameEvent.Make(GameEventType.PromptClosed));
            if (p.Kind == PromptKind.PlaceStartingPieces) p.OnPieces?.Invoke(r.Value, r.Value2, r.Value3);
            else p.OnResponse?.Invoke(r.Value);
            return CommandResult.Success;
        }

        // ------------------------------------------------------------------
        // Events / logging
        // ------------------------------------------------------------------

        public void Emit(GameEvent ev)
        {
            Events.Add(ev);
            if (ev.Text != null) Log.Add(ev.Text);
            OnEvent?.Invoke(ev);
        }

        public void LogText(string text) => Emit(GameEvent.Make(GameEventType.Log, text));

        public string Name(int p) => p >= 0 && p < State.Players.Count ? State.Players[p].Name : "nobody";
        public string TName(int t) => Map[t].Name;

        // ------------------------------------------------------------------
        // Decks and dice
        // ------------------------------------------------------------------

        public int RollDie(int sides) => Rng.Next(1, sides + 1);

        public void Shuffle<T>(IList<T> list)
        {
            for (int i = list.Count - 1; i > 0; i--) { int j = Rng.Next(i + 1); (list[i], list[j]) = (list[j], list[i]); }
        }

        public int DrawTerritoryCard(TerritoryType type, bool skipDevastated = false)
        {
            int k = (int)type;
            for (int guard = 0; guard < 200; guard++)
            {
                if (State.TerritoryDeckPos[k] >= State.TerritoryDecks[k].Count) { Shuffle(State.TerritoryDecks[k]); State.TerritoryDeckPos[k] = 0; }
                int t = State.TerritoryDecks[k][State.TerritoryDeckPos[k]++];
                if (!skipDevastated || !State.Territories[t].Devastated) return t;
            }
            return State.TerritoryDecks[k][0];
        }

        public int DrawCommandCard(CommanderType deck)
        {
            var d = State.CommandDecks[(int)deck];
            if (d.Count == 0) return -1;
            int id = d[d.Count - 1];
            d.RemoveAt(d.Count - 1);
            return id;
        }

        public int DeckSize(CommanderType deck) => State.CommandDecks[(int)deck].Count;

        // ------------------------------------------------------------------
        // Board mutation helpers (all emit events)
        // ------------------------------------------------------------------

        /// <summary>Removes n units (MODs first, then commanders). Returns the number removed.</summary>
        public int DestroyUnits(int t, int n)
        {
            var ts = State.Territories[t];
            int removed = Math.Min(n, ts.Mods);
            ts.Mods -= removed;
            for (int c = 0; c < MapGraph.NumCommanders && removed < n; c++)
                if (ts.Commanders[c]) { ts.Commanders[c] = false; removed++; }
            if (removed > 0) { var ev = GameEvent.Make(GameEventType.UnitsDestroyed); ev.To = t; ev.Amount = removed; ev.Player = ts.Owner; Emit(ev); }
            if (ts.Units == 0 && !ts.SpaceStation) ts.Owner = -1;
            return removed;
        }

        public void Devastate(int t)
        {
            var ts = State.Territories[t];
            ts.Owner = -1; ts.Mods = 0; Array.Clear(ts.Commanders, 0, ts.Commanders.Length); ts.SpaceStation = false; ts.Devastated = true;
            var ev = GameEvent.Make(GameEventType.TerritoryDevastated, TName(t) + " is devastated"); ev.To = t; Emit(ev);
        }

        public void CheckElimination(int p)
        {
            if (!State.IsActive(p) || State.CountUnits(p) > 0) return;
            State.Players[p].Eliminated = true;
            State.Players[p].Hand.Clear();
            foreach (var t in State.Territories) if (t.Owner == p) { t.SpaceStation = false; t.Owner = -1; }
            var ev = GameEvent.Make(GameEventType.PlayerEliminated, Name(p) + " has been eliminated!"); ev.Player = p; Emit(ev);
        }

        public void ChangeEnergy(int p, int delta)
        {
            State.Players[p].Energy += delta;
            var ev = GameEvent.Make(GameEventType.EnergyChanged); ev.Player = p; ev.Amount = delta; Emit(ev);
        }

        // ------------------------------------------------------------------
        // Turn structure
        // ------------------------------------------------------------------

        public void StartTurn(int p)
        {
            var ps = State.Players[p];
            State.CurrentPlayer = p;
            ps.CardsBoughtThisTurn = 0; ps.ContestedCaptures = 0; ps.BonusClaimed = false; ps.InvasionDeclaredThisTurn = false;
            ps.FortifiedThisTurn = false; ps.InvadeEarthTarget = -1; ps.InvasionsThisTurn = 0;
            State.Invasion = new Invasion();

            int inc = State.Income(p);
            ps.Pool += inc;
            ps.Energy += inc;
            int stationMods = 0;
            foreach (var t in State.Territories) if (t.Owner == p && t.SpaceStation) { t.Mods++; stationMods++; }
            var ev = GameEvent.Make(GameEventType.TurnStarted,
                $"--- {ps.Name}'s turn: {State.CountTerritories(p)} territories, +{inc} MODs, +{inc} energy" + (stationMods > 0 ? $", +{stationMods} at Space Stations" : ""));
            ev.Player = p; ev.Amount = inc; Emit(ev);
        }

        /// <summary>Advances to the next player in turn order, or to the next year.</summary>
        public void EndTurn()
        {
            State.CurrentPlayer = -1;
            for (State.TurnIndex++; State.TurnIndex < State.TurnOrder.Count; State.TurnIndex++)
            {
                int p = State.TurnOrder[State.TurnIndex];
                if (State.IsActive(p)) { Transition(new DeploymentState()); return; }
            }
            Transition(new YearInitState());
        }

        // ------------------------------------------------------------------
        // Command cards
        // ------------------------------------------------------------------

        /// <summary>Applies a before-first-invasion card. May open ChooseTerritory prompts; `done` runs when complete.</summary>
        public void ApplyCard(int p, CardDef card, Action done)
        {
            var ps = State.Players[p];
            switch (card.Kind)
            {
                case CardKind.Reinforcements:
                {
                    var opts = State.OwnedTerritories(p, card.Target);
                    int placements = Math.Min(3, opts.Count);
                    void Step(int remaining)
                    {
                        if (remaining == 0 || opts.Count == 0) { done(); return; }
                        OpenPrompt(new Prompt { Kind = PromptKind.ChooseTerritory, Player = p, Text = "Place 1 reinforcement MOD", Options = new List<int>(opts),
                            OnResponse = t => { State.Territories[t].Mods++; EmitDeploy(p, t, 1); opts.Remove(t); Step(remaining - 1); } });
                    }
                    Step(placements);
                    break;
                }
                case CardKind.AssembleMods:
                {
                    var opts = State.OwnedTerritories(p, card.Target);
                    if (opts.Count == 0) { done(); break; }
                    OpenPrompt(new Prompt { Kind = PromptKind.ChooseTerritory, Player = p, Text = "Place 3 MODs", Options = opts,
                        OnResponse = t => { State.Territories[t].Mods += 3; EmitDeploy(p, t, 3); done(); } });
                    break;
                }
                case CardKind.EnergyCrisis:
                {
                    int got = 0;
                    for (int q = 0; q < State.NumPlayers; q++)
                        if (q != p && State.IsActive(q) && State.Players[q].Energy > 0) { ChangeEnergy(q, -1); got++; }
                    ChangeEnergy(p, got);
                    LogText($"{ps.Name} collects {got} energy");
                    done();
                    break;
                }
                case CardKind.Redeployment:
                {
                    var sources = State.OwnedTerritories(p).Where(t => State.Territories[t].Mods >= 1 && State.Territories[t].Units >= 2).ToList();
                    if (sources.Count == 0) { done(); break; }
                    OpenPrompt(new Prompt { Kind = PromptKind.ChooseTerritory, Player = p, Text = "Redeploy MODs from", Options = sources, OnResponse = from =>
                    {
                        var dests = State.OwnedTerritories(p); dests.Remove(from);
                        if (dests.Count == 0) { done(); return; }
                        OpenPrompt(new Prompt { Kind = PromptKind.ChooseTerritory, Player = p, Text = "Redeploy MODs to", Options = dests, OnResponse = to =>
                        {
                            int n = Math.Min(3, Math.Min(State.Territories[from].Mods, State.Territories[from].Units - 1));
                            State.Territories[from].Mods -= n; State.Territories[to].Mods += n;
                            var ev = GameEvent.Make(GameEventType.UnitsMoved, $"{ps.Name} redeploys {n} MOD(s) from {TName(from)} to {TName(to)}");
                            ev.Player = p; ev.From = from; ev.To = to; ev.Amount = n; Emit(ev);
                            done();
                        } });
                    } });
                    break;
                }
                case CardKind.EnergyExtraction:
                {
                    int n = Math.Min(4, State.OwnedTerritories(p, TerritoryType.Water).Count);
                    ChangeEnergy(p, n);
                    LogText($"{ps.Name} extracts {n} energy");
                    done();
                    break;
                }
                case CardKind.ScatterBomb:
                {
                    for (int i = 0; i < 3; i++)
                    {
                        int t = DrawTerritoryCard(card.Target);
                        var ts = State.Territories[t];
                        if (ts.Owner < 0 || ts.Owner == p || ts.Devastated) continue;
                        int owner = ts.Owner;
                        int n = (ts.Units + 1) / 2;
                        LogText($"Scatter bomb hits {TName(t)}: {n} unit(s) destroyed");
                        DestroyUnits(t, n);
                        CheckElimination(owner);
                    }
                    done();
                    break;
                }
                case CardKind.TheMother:
                {
                    int t = DrawTerritoryCard(TerritoryType.Land, skipDevastated: true);
                    var affected = new HashSet<int>();
                    foreach (int n in Map[t].Neighbors)
                        if (Map[n].Type == TerritoryType.Land && !State.Territories[n].Devastated)
                        {
                            if (State.Territories[n].Owner >= 0) affected.Add(State.Territories[n].Owner);
                            DestroyUnits(n, State.Territories[n].Units);
                        }
                    if (State.Territories[t].Owner >= 0) affected.Add(State.Territories[t].Owner);
                    LogText($"The Mother detonates on {TName(t)}");
                    Devastate(t);
                    foreach (int q in affected) CheckElimination(q);
                    done();
                    break;
                }
                case CardKind.Armageddon:
                {
                    for (int t = 0; t < Map.Count; t++) DestroyUnits(t, State.Territories[t].Units / 2);
                    LogText("Armageddon: every territory loses half its units");
                    for (int q = 0; q < State.NumPlayers; q++) CheckElimination(q);
                    done();
                    break;
                }
                case CardKind.InvadeEarth:
                {
                    int t = DrawTerritoryCard(TerritoryType.Land, skipDevastated: true);
                    ps.InvadeEarthTarget = t;
                    LogText($"{ps.Name} may invade {TName(t)} from the Moon this turn");
                    done();
                    break;
                }
                case CardKind.ScoutForces:
                {
                    int t = DrawTerritoryCard(TerritoryType.Land, skipDevastated: true);
                    if (State.Territories[t].Owner == p) { State.Territories[t].Mods += 5; EmitDeploy(p, t, 5); LogText($"{ps.Name} places 5 scout MODs on {TName(t)}"); }
                    else { ps.ScoutTerritory = t; LogText($"{ps.Name} has scouts waiting in {TName(t)}"); }
                    done();
                    break;
                }
                default:
                    done();
                    break;
            }
        }

        public void EmitDeploy(int p, int t, int n)
        {
            var ev = GameEvent.Make(GameEventType.ModsDeployed); ev.Player = p; ev.To = t; ev.Amount = n; Emit(ev);
        }

        // ------------------------------------------------------------------
        // Final scoring
        // ------------------------------------------------------------------

        public void FinalScoring()
        {
            LogText("=== Final scoring ===");
            for (int p = 0; p < State.NumPlayers; p++)
            {
                var ps = State.Players[p];
                int influence = 0;
                if (!ps.Eliminated)
                {
                    var keep = new List<int>();
                    foreach (int id in ps.Hand)
                    {
                        var c = CardCatalogue.Get(id);
                        if (c.Timing == CardTiming.Scoring && State.CommanderInPlay(p, c.Deck)) influence++; else keep.Add(id);
                    }
                    ps.Hand = keep;
                }
                ps.FinalScore = State.Score(p) + influence;
                LogText($"{ps.Name}: {State.CountTerritories(p)} territories + {State.RegionBonus(p)} bonus + {influence} influence = {ps.FinalScore}");
            }
            var ranking = Enumerable.Range(0, State.NumPlayers).ToList();
            ranking.Sort((a, b) =>
            {
                int c = State.Players[b].FinalScore.CompareTo(State.Players[a].FinalScore);
                if (c != 0) return c;
                c = State.Players[b].Energy.CompareTo(State.Players[a].Energy);
                if (c != 0) return c;
                return State.CountUnits(b).CompareTo(State.CountUnits(a));
            });
            State.Winner = ranking[0];
            var ev = GameEvent.Make(GameEventType.GameOver, $"Winner: {Name(State.Winner)} is elected the new world leader!"); ev.Player = State.Winner; Emit(ev);
        }
    }
}
