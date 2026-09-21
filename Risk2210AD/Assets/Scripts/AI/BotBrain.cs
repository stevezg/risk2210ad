using System;
using System.Collections.Generic;
using System.Linq;
using Risk2210.Core;

namespace Risk2210.AI
{
    /// <summary>
    /// Heuristic bot. Pure function of a GameState snapshot: safe to run on a worker thread.
    /// Produces exactly one command per call; the runner feeds commands back one at a time.
    /// </summary>
    public sealed class BotBrain
    {
        private readonly Random rng;
        public BotBrain(int seed) { rng = new Random(seed); }

        private T Pick<T>(IList<T> list) => list[rng.Next(list.Count)];

        public GameCommand Decide(GameState s, int p)
        {
            var cmd = DecideInner(s, p);
            if (cmd != null) cmd.Player = p;
            return cmd;
        }

        private GameCommand DecideInner(GameState s, int p)
        {
            if (s.Prompt != null) return AnswerPrompt(s, p, s.Prompt);
            switch (s.Phase)
            {
                case PhaseId.Setup: return Setup(s, p);
                case PhaseId.EnergyBidding: return Bidding(s, p);
                case PhaseId.Deployment: return Deployment(s, p);
                case PhaseId.Invasion: return Invasion(s, p);
                case PhaseId.Fortification: return Fortification(s, p);
                default: return null;
            }
        }

        // ------------------------------------------------------------------

        private List<int> Border(GameState s, int p)
        {
            var b = s.OwnedTerritories(p).Where(t => s.IsBorder(p, t)).ToList();
            return b.Count > 0 ? b : s.OwnedTerritories(p);
        }

        private GameCommand Setup(GameState s, int p)
        {
            switch (s.SetupStage)
            {
                case SetupStage.Claim:
                {
                    var free = new List<int>();
                    foreach (int t in s.Map.TerritoriesOfType(TerritoryType.Land))
                        if (!s.Territories[t].Devastated && s.Territories[t].Owner == -1) free.Add(t);
                    var near = free.Where(t => s.Map[t].Neighbors.Any(n => s.Territories[n].Owner == p)).ToList();
                    int pick = near.Count > 0 && rng.Next(4) != 0 ? Pick(near) : Pick(free);
                    return new ClaimTerritory { Territory = pick };
                }
                case SetupStage.PlaceMods:
                    return new PlaceStartingMod { Territory = Pick(Border(s, p)) };
                case SetupStage.Pieces:
                {
                    var owned = s.OwnedTerritories(p);
                    int best = owned[0];
                    foreach (int t in owned) if (s.Territories[t].Mods > s.Territories[best].Mods) best = t;
                    return new PlaceStartingPieces { SpaceStation = best, LandCommander = best, Diplomat = Pick(Border(s, p)) };
                }
            }
            return null;
        }

        private GameCommand Bidding(GameState s, int p)
        {
            if (s.BidChooser == p) return new ChooseTurnOrder { Marker = s.AvailableMarkers.Min() };
            int e = s.Players[p].Energy;
            int bid = e <= 3 ? 0 : rng.Next(0, Math.Min(3, e / 3) + 1);
            return new SubmitBid { Amount = bid };
        }

        private GameCommand Deployment(GameState s, int p)
        {
            var ps = s.Players[p];
            if (ps.Pool > 0)
            {
                int n = rng.Next(1, ps.Pool + 1);
                return new DeployMods { Territory = Pick(Border(s, p)), Count = n };
            }
            var priority = new[] { CommanderType.Land, CommanderType.Naval, CommanderType.Nuclear, CommanderType.Space, CommanderType.Diplomat };
            foreach (var c in priority)
                if (!s.CommanderInPlay(p, c) && ps.Energy >= Rules.CommanderCost + 1)
                    return new HireCommander { Commander = c, Territory = Pick(Border(s, p)) };
            if (ps.Energy >= Rules.SpaceStationCost + 2 && s.CountSpaceStations(p) < Rules.MaxSpaceStations && rng.Next(3) == 0)
            {
                var spots = s.OwnedTerritories(p, TerritoryType.Land).Where(t => !s.Territories[t].SpaceStation).ToList();
                if (spots.Count > 0) return new BuildSpaceStation { Territory = Pick(spots) };
            }
            if (ps.CardsBoughtThisTurn == 0 && ps.Energy > 1 && ps.Hand.Count < 6)
            {
                var decks = new List<CommanderType>();
                for (int c = 0; c < MapGraph.NumCommanders; c++)
                    if (s.CommanderInPlay(p, (CommanderType)c) && s.CommandDecks[c].Count > 0) decks.Add((CommanderType)c);
                if (decks.Count > 0)
                {
                    int spend = Math.Min(Rules.MaxCardsPerTurn, Math.Min(ps.Energy - 1, 6 - ps.Hand.Count));
                    var want = new List<CommanderType>();
                    var count = new int[MapGraph.NumCommanders];
                    for (int i = 0; i < spend; i++)
                    {
                        var c = Pick(decks);
                        if (++count[(int)c] <= s.CommandDecks[(int)c].Count) want.Add(c);
                    }
                    if (want.Count > 0) return new BuyCards { Decks = want };
                }
            }
            int card = PlayableCard(s, p);
            if (card >= 0) return new PlayCard { HandIndex = card };
            return new EndDeployment();
        }

        private int PlayableCard(GameState s, int p)
        {
            var ps = s.Players[p];
            for (int i = 0; i < ps.Hand.Count; i++)
            {
                var c = CardCatalogue.Get(ps.Hand[i]);
                if (c.Timing != CardTiming.BeforeFirstInvasion) continue;
                if (!s.CommanderInPlay(p, c.Deck)) continue;
                if (c.Kind == CardKind.Armageddon) { bool other = false; foreach (int id in ps.Hand) { var o = CardCatalogue.Get(id); if (o.Deck == CommanderType.Nuclear && o.Cost > 0 && o.Kind != CardKind.Armageddon) other = true; } if (!other) continue; }
                if (c.Kind == CardKind.EnergyExtraction && !s.Map.Regions.Any(r => r.Type == TerritoryType.Moon && s.ControlsRegion(p, r.Id))) continue;
                if (ps.Energy - s.CardCost(p, c) < 1) continue;
                return i;
            }
            return -1;
        }

        private GameCommand Invasion(GameState s, int p)
        {
            var inv = s.Invasion;
            if (inv.Active)
            {
                if (inv.Captured)
                {
                    int maxMove = s.Territories[inv.From].Units - 1;
                    int minMove = Math.Max(1, Math.Min(inv.LastDice, maxMove));
                    int keep = s.IsBorder(p, inv.From) ? Math.Min(maxMove, 2) : 0;
                    return new MoveIn { Count = Math.Max(minMove, maxMove - keep) };
                }
                int units = s.Territories[inv.From].Units;
                if (units < 2 || (units <= s.Territories[inv.To].Units && inv.AttackedOnce)) return new EndInvasion();
                return new Attack { Dice = Math.Min(3, units - 1) };
            }
            if (!s.Players[p].InvasionDeclaredThisTurn)
            {
                int card = PlayableCard(s, p);
                if (card >= 0) return new PlayCard { HandIndex = card };
            }
            if (s.Players[p].InvasionsThisTurn >= 12) return new EndInvasionPhase();
            var options = new List<(int from, int to, float score)>();
            foreach (int from in s.OwnedTerritories(p))
            {
                var f = s.Territories[from];
                if (f.Units < 2) continue;
                var targets = new List<int>(s.Map[from].Neighbors);
                if (f.SpaceStation) foreach (var t in s.Map.Territories) if (t.LunarLandingSite) targets.Add(t.Id);
                if (s.Players[p].InvadeEarthTarget >= 0) targets.Add(s.Players[p].InvadeEarthTarget);
                foreach (int to in targets)
                {
                    if (!s.CanInvade(p, from, to)) continue;
                    int du = s.Territories[to].Units;
                    if (du == 0 || f.Units >= du + 2) options.Add((from, to, f.Units - du + (du == 0 ? 1 : 0)));
                }
            }
            if (options.Count == 0) return new EndInvasionPhase();
            options.Sort((a, b) => b.score.CompareTo(a.score));
            var best = options[Math.Min(rng.Next(3), options.Count - 1)];
            return new DeclareInvasion { From = best.from, To = best.to };
        }

        private GameCommand Fortification(GameState s, int p)
        {
            int bestFrom = -1;
            foreach (int t in s.OwnedTerritories(p))
                if (!s.IsBorder(p, t) && s.Territories[t].Mods >= 2 && (bestFrom < 0 || s.Territories[t].Mods > s.Territories[bestFrom].Mods)) bestFrom = t;
            for (int i = 0; i < s.Players[p].Hand.Count; i++)
            {
                var c = CardCatalogue.Get(s.Players[p].Hand[i]);
                if (c.Timing == CardTiming.EndOfTurn && s.CommanderInPlay(p, c.Deck) && !s.Players[p].FortifiedThisTurn) return new PlayCard { HandIndex = i };
            }
            if (bestFrom < 0) return new EndFortification();
            var dests = Border(s, p).Where(t => t != bestFrom && s.FortifyPathExists(p, bestFrom, t)).ToList();
            if (dests.Count == 0) return new EndFortification();
            return new Fortify { From = bestFrom, To = Pick(dests), Mods = s.Territories[bestFrom].Mods - 1 };
        }

        private GameCommand AnswerPrompt(GameState s, int p, Prompt pr)
        {
            switch (pr.Kind)
            {
                case PromptKind.DefenseDice: return new RespondPrompt { Value = pr.MaxDice };
                case PromptKind.BonusDeck: return new RespondPrompt { Value = 0 };
                case PromptKind.ChoosePlayer:
                {
                    int best = pr.Options[0];
                    foreach (int q in pr.Options) if (s.Players[q].Hand.Count > s.Players[best].Hand.Count) best = q;
                    return new RespondPrompt { Value = best };
                }
                case PromptKind.ChooseTerritory:
                {
                    var border = pr.Options.Where(t => s.Territories[t].Owner == p && s.IsBorder(p, t)).ToList();
                    return new RespondPrompt { Value = border.Count > 0 ? Pick(border) : Pick(pr.Options) };
                }
                case PromptKind.ReactiveCard:
                {
                    var inv = s.Invasion;
                    if (inv.Defender != p) return new RespondPrompt { Value = -1 };
                    var hand = s.Players[p].Hand;
                    var tt = s.Map[inv.To].Type;
                    int attackers = s.Territories[inv.From].Units;
                    int defenders = s.Territories[inv.To].Units;
                    for (int i = 0; i < hand.Count; i++)
                    {
                        var c = CardCatalogue.Get(hand[i]);
                        if (!s.ReactiveCardPlayable(p, c)) continue;
                        switch (c.Kind)
                        {
                            case CardKind.StealthMods: return new RespondPrompt { Value = i };
                            case CardKind.StealthStation: return new RespondPrompt { Value = i };
                            case CardKind.DeathTrap: if (attackers >= 4) return new RespondPrompt { Value = i }; break;
                            case CardKind.CeaseFire: if (attackers >= 6 && s.Players[p].Energy >= c.Cost + 1) return new RespondPrompt { Value = i }; break;
                            case CardKind.Evacuation: if (attackers >= defenders * 3 && defenders >= 3) return new RespondPrompt { Value = i }; break;
                        }
                    }
                    return new RespondPrompt { Value = -1 };
                }
                default:
                    return new RespondPrompt { Value = pr.Options.Count > 0 ? pr.Options[0] : 0 };
            }
        }
    }
}
