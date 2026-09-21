using System;
using System.Collections.Generic;

namespace Risk2210.Core
{
    public enum PhaseId { Setup, YearInit, EnergyBidding, Deployment, Invasion, Fortification, GameOver }

    public static class Rules
    {
        public const int CommanderCost = 3;
        public const int SpaceStationCost = 5;
        public const int MaxSpaceStations = 4;
        public const int CardCost = 1;
        public const int MaxCardsPerTurn = 4;
        public const int NumYears = 5;
        public const int StartingEnergy = 3;
        public const int DevastationMarkers = 4;
    }

    public sealed class TerritoryState
    {
        public int Owner = -1;
        public int Mods;
        public bool[] Commanders = new bool[MapGraph.NumCommanders];
        public bool SpaceStation;
        public bool Devastated;

        public int CommanderCount { get { int n = 0; foreach (bool b in Commanders) if (b) n++; return n; } }
        public int Units => Mods + CommanderCount;
        public bool HasCommander(CommanderType c) => Commanders[(int)c];

        public TerritoryState Clone()
        {
            var t = (TerritoryState)MemberwiseClone();
            t.Commanders = (bool[])Commanders.Clone();
            return t;
        }
    }

    public sealed class PlayerState
    {
        public string Name;
        public int Energy = Rules.StartingEnergy;
        public int Pool;                       // MODs waiting to be deployed
        public List<int> Hand = new List<int>();
        public bool Eliminated;
        public bool IsBot;
        public bool IsNeutral;
        public int ScoutTerritory = -1;
        public int FinalScore;
        public int Bid = -1;                   // this year's bid, -1 until submitted

        // per-turn bookkeeping
        public int CardsBoughtThisTurn;
        public int ContestedCaptures;
        public bool BonusClaimed;
        public bool InvasionDeclaredThisTurn;
        public bool FortifiedThisTurn;
        public int InvadeEarthTarget = -1;
        public int InvasionsThisTurn;

        public PlayerState Clone()
        {
            var p = (PlayerState)MemberwiseClone();
            p.Hand = new List<int>(Hand);
            return p;
        }
    }

    public sealed class Invasion
    {
        public bool Active;
        public int Attacker = -1;
        public int Defender = -1;   // -1 when the target was empty
        public int From = -1;
        public int To = -1;
        public bool Contested;
        public bool AttackedOnce;
        public bool Captured;
        public int LastDice;
        public bool[] MustMoveIn = new bool[MapGraph.NumCommanders];
        public int PendingAttackDice;  // attack waiting on the defender's dice choice

        public Invasion Clone()
        {
            var i = (Invasion)MemberwiseClone();
            i.MustMoveIn = (bool[])MustMoveIn.Clone();
            return i;
        }
    }

    public enum SetupStage { Claim, PlaceMods, Pieces, Done }

    public enum PromptKind { None, ClaimTerritory, PlaceStartingMod, PlaceStartingPieces, DefenseDice, ReactiveCard, BonusDeck, ChooseTerritory }

    /// <summary>A decision the engine is waiting on from a specific player, outside the normal command flow.</summary>
    public sealed class Prompt
    {
        public PromptKind Kind;
        public int Player;
        public string Text;
        public List<int> Options = new List<int>();
        public List<CommanderType> Decks = new List<CommanderType>();
        public int MaxDice;
        [NonSerialized] public Action<int> OnResponse;       // continuation (not cloned)
        [NonSerialized] public Action<int, int, int> OnPieces; // PlaceStartingPieces continuation

        public Prompt Clone()
        {
            var p = (Prompt)MemberwiseClone();
            p.Options = new List<int>(Options);
            p.Decks = new List<CommanderType>(Decks);
            p.OnResponse = null; p.OnPieces = null;
            return p;
        }
    }

    /// <summary>Complete, cloneable game state. The AI works on clones so the main thread never blocks.</summary>
    public sealed class GameState
    {
        public MapGraph Map;
        public TerritoryState[] Territories;
        public List<PlayerState> Players = new List<PlayerState>();
        public int NumPlayers;                 // real players (neutral excluded)
        public int Year;
        public PhaseId Phase = PhaseId.Setup;
        public int CurrentPlayer = -1;
        public List<int> TurnOrder = new List<int>();
        public int TurnIndex;
        public Invasion Invasion = new Invasion();
        public Prompt Prompt;                  // null when no prompt is pending
        public List<int>[] CommandDecks = new List<int>[MapGraph.NumCommanders];
        public List<int>[] TerritoryDecks = new List<int>[3];
        public int[] TerritoryDeckPos = new int[3];
        public int Winner = -1;

        // sub-stage bookkeeping exposed so bots can read it from a snapshot
        public SetupStage SetupStage = SetupStage.Claim;
        public int SetupActor = -1;            // player expected to act during setup
        public int BidChooser = -1;            // player currently picking a turn marker (-1 while bids are secret)
        public List<int> AvailableMarkers = new List<int>();
        public List<int> BidRanking = new List<int>();

        public bool HasNeutral => Players.Count > NumPlayers;
        public int NeutralPlayer => HasNeutral ? NumPlayers : -1;
        public bool IsActive(int p) => p >= 0 && p < NumPlayers && !Players[p].Eliminated;

        /// <summary>The player the engine is waiting on right now (-1 if none, e.g. game over).</summary>
        public int ExpectedActor
        {
            get
            {
                if (Prompt != null) return Prompt.Player;
                switch (Phase)
                {
                    case PhaseId.Setup: return SetupActor;
                    case PhaseId.EnergyBidding:
                        if (BidChooser >= 0) return BidChooser;
                        for (int p = 0; p < NumPlayers; p++) if (IsActive(p) && Players[p].Bid < 0) return p;
                        return -1;
                    case PhaseId.Deployment:
                    case PhaseId.Invasion:
                    case PhaseId.Fortification:
                        return CurrentPlayer;
                    default: return -1;
                }
            }
        }

        public GameState Clone()
        {
            var s = (GameState)MemberwiseClone();   // Map is shared (immutable after construction)
            s.Territories = new TerritoryState[Territories.Length];
            for (int i = 0; i < Territories.Length; i++) s.Territories[i] = Territories[i].Clone();
            s.Players = new List<PlayerState>();
            foreach (var p in Players) s.Players.Add(p.Clone());
            s.TurnOrder = new List<int>(TurnOrder);
            s.Invasion = Invasion.Clone();
            s.Prompt = Prompt?.Clone();
            s.CommandDecks = new List<int>[MapGraph.NumCommanders];
            for (int i = 0; i < MapGraph.NumCommanders; i++) s.CommandDecks[i] = new List<int>(CommandDecks[i]);
            s.TerritoryDecks = new List<int>[3];
            for (int i = 0; i < 3; i++) s.TerritoryDecks[i] = new List<int>(TerritoryDecks[i]);
            s.TerritoryDeckPos = (int[])TerritoryDeckPos.Clone();
            s.AvailableMarkers = new List<int>(AvailableMarkers);
            s.BidRanking = new List<int>(BidRanking);
            return s;
        }

        // ------------------------------------------------------------------
        // Rule queries (pure; safe to call from the AI thread on a clone)
        // ------------------------------------------------------------------

        public bool CommanderInPlay(int p, CommanderType c)
        {
            foreach (var t in Territories) if (t.Owner == p && t.HasCommander(c)) return true;
            return false;
        }

        public int CountTerritories(int p) { int n = 0; foreach (var t in Territories) if (t.Owner == p) n++; return n; }
        public int CountUnits(int p) { int n = 0; foreach (var t in Territories) if (t.Owner == p) n += t.Units; return n; }
        public int CountSpaceStations(int p) { int n = 0; foreach (var t in Territories) if (t.Owner == p && t.SpaceStation) n++; return n; }

        public bool ControlsRegion(int p, int region)
        {
            bool any = false;
            foreach (int t in Map.Regions[region].Territories)
            {
                if (Territories[t].Devastated) continue;
                if (Territories[t].Owner != p) return false;
                any = true;
            }
            return any;
        }

        public int RegionBonus(int p)
        {
            int b = 0;
            for (int r = 0; r < Map.Regions.Count; r++) if (ControlsRegion(p, r)) b += Map.Regions[r].Bonus;
            return b;
        }

        public int Income(int p) => Math.Max(3, CountTerritories(p) / 3) + RegionBonus(p);
        public int Score(int p) => CountTerritories(p) + RegionBonus(p);

        public List<int> OwnedTerritories(int p, TerritoryType? type = null)
        {
            var list = new List<int>();
            for (int t = 0; t < Territories.Length; t++)
                if (Territories[t].Owner == p && (!type.HasValue || Map[t].Type == type.Value)) list.Add(t);
            return list;
        }

        public bool IsBorder(int p, int t)
        {
            foreach (int n in Map[t].Neighbors)
                if (!Territories[n].Devastated && Territories[n].Owner != p) return true;
            return false;
        }

        public bool CanInvade(int p, int from, int to, out string why)
        {
            why = null;
            if (from < 0 || from >= Map.Count || to < 0 || to >= Map.Count) { why = "no such territory"; return false; }
            if (from == to) { why = "cannot invade yourself"; return false; }
            var f = Territories[from]; var d = Territories[to];
            if (f.Owner != p) { why = "you do not control the attacking territory"; return false; }
            if (d.Owner == p) { why = "you already control the target"; return false; }
            if (f.Devastated || d.Devastated) { why = "devastated territories are impassable"; return false; }
            if (f.Units < 2) { why = "need at least 2 units to invade"; return false; }
            var ft = Map[from].Type; var tt = Map[to].Type;
            if ((ft == TerritoryType.Water || tt == TerritoryType.Water) && !CommanderInPlay(p, CommanderType.Naval))
            { why = "a Naval Commander must be in play to invade into or out of water"; return false; }
            if ((ft == TerritoryType.Moon || tt == TerritoryType.Moon) && !CommanderInPlay(p, CommanderType.Space))
            { why = "a Space Commander must be in play to invade into or out of the Moon"; return false; }
            if (ft == TerritoryType.Moon && tt != TerritoryType.Moon)
            {
                if (Players[p].InvadeEarthTarget != to) { why = "Earth can only be invaded from the Moon with the Invade Earth card"; return false; }
                return true;
            }
            if (ft != TerritoryType.Moon && tt == TerritoryType.Moon)
            {
                if (!f.SpaceStation) { why = "lunar invasions must launch from a Space Station"; return false; }
                if (!Map[to].LunarLandingSite) { why = "from Earth you may only invade a lunar landing site"; return false; }
                return true;
            }
            if (!Map.AreAdjacent(from, to)) { why = "territories are not adjacent"; return false; }
            return true;
        }

        public bool CanInvade(int p, int from, int to) => CanInvade(p, from, to, out _);

        /// <summary>Attacker d8 count for nDice, per the rulebook commander table. `which` marks the contributing commanders.</summary>
        public int AttackD8Count(int from, int to, int nDice, bool[] which = null)
        {
            var f = Territories[from];
            var ft = Map[from].Type; var tt = Map[to].Type;
            int n = 0;
            if (which != null) Array.Clear(which, 0, which.Length);
            for (int c = 0; c < MapGraph.NumCommanders && n < nDice; c++)
            {
                if (!f.Commanders[c]) continue;
                bool ok;
                switch ((CommanderType)c)
                {
                    case CommanderType.Nuclear: ok = true; break;
                    case CommanderType.Land: ok = ft == TerritoryType.Land || tt == TerritoryType.Land; break;
                    case CommanderType.Naval: ok = ft == TerritoryType.Water || tt == TerritoryType.Water; break;
                    case CommanderType.Space: ok = ft == TerritoryType.Moon || tt == TerritoryType.Moon; break;
                    default: ok = false; break;
                }
                if (ok) { n++; if (which != null) which[c] = true; }
            }
            return n;
        }

        public int DefendD8Count(int to, int nDice)
        {
            var d = Territories[to];
            return d.SpaceStation ? nDice : Math.Min(nDice, d.CommanderCount);
        }

        /// <summary>Whether a chain of friendly territories (including station↔landing-site launches) links two territories.</summary>
        public bool FortifyPathExists(int p, int from, int to)
        {
            if (Territories[from].Owner != p || Territories[to].Owner != p) return false;
            var stations = new List<int>(); var sites = new List<int>();
            for (int t = 0; t < Map.Count; t++)
            {
                if (Territories[t].Owner != p) continue;
                if (Territories[t].SpaceStation) stations.Add(t);
                if (Map[t].LunarLandingSite) sites.Add(t);
            }
            var seen = new bool[Map.Count];
            var q = new Queue<int>();
            q.Enqueue(from); seen[from] = true;
            void Visit(int n) { if (Territories[n].Owner == p && !Territories[n].Devastated && !seen[n]) { seen[n] = true; q.Enqueue(n); } }
            while (q.Count > 0)
            {
                int cur = q.Dequeue();
                if (cur == to) return true;
                foreach (int n in Map[cur].Neighbors) Visit(n);
                if (Territories[cur].SpaceStation) foreach (int s in sites) Visit(s);
                if (Map[cur].LunarLandingSite) foreach (int s in stations) Visit(s);
            }
            return false;
        }

        public bool HasReactiveCard(int p)
        {
            foreach (int id in Players[p].Hand)
            {
                var c = CardCatalogue.Get(id);
                if (c.Timing == CardTiming.OnInvasionDeclared && CommanderInPlay(p, c.Deck) && Players[p].Energy >= c.Cost) return true;
            }
            return false;
        }
    }
}
