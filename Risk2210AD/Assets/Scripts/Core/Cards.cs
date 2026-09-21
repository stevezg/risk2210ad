using System.Collections.Generic;

namespace Risk2210.Core
{
    public enum CardTiming { BeforeFirstInvasion, OnInvasionDeclared, Scoring }

    public enum CardKind
    {
        Reinforcements, AssembleMods, StealthMods, ColonyInfluence, EnergyCrisis, CeaseFire, Redeployment,
        EnergyExtraction, ScatterBomb, TheMother, Armageddon, InvadeEarth, ScoutForces
    }

    public sealed class CardDef
    {
        public int Id;
        public string Name;
        public CommanderType Deck;
        public int Cost;
        public CardTiming Timing;
        public CardKind Kind;
        public TerritoryType Target;
        public string Text;
    }

    /// <summary>Static catalogue of command cards and per-deck composition.</summary>
    public static class CardCatalogue
    {
        private static readonly List<CardDef> defs = new List<CardDef>();
        private static readonly List<int> copies = new List<int>();

        public static IReadOnlyList<CardDef> All { get { EnsureBuilt(); return defs; } }
        public static CardDef Get(int id) { EnsureBuilt(); return defs[id]; }

        public static List<int> BuildDeck(CommanderType deck)
        {
            EnsureBuilt();
            var d = new List<int>();
            for (int i = 0; i < defs.Count; i++)
                if (defs[i].Deck == deck) for (int c = 0; c < copies[i]; c++) d.Add(defs[i].Id);
            return d;
        }

        private static void Add(string name, CommanderType deck, int cost, CardTiming timing, CardKind kind, TerritoryType target, string text, int n)
        {
            defs.Add(new CardDef { Id = defs.Count, Name = name, Deck = deck, Cost = cost, Timing = timing, Kind = kind, Target = target, Text = text });
            copies.Add(n);
        }

        private static void EnsureBuilt()
        {
            if (defs.Count > 0) return;
            const CardTiming B = CardTiming.BeforeFirstInvasion, R = CardTiming.OnInvasionDeclared, S = CardTiming.Scoring;
            // Land
            Add("Reinforcements (Land)", CommanderType.Land, 0, B, CardKind.Reinforcements, TerritoryType.Land, "Place 3 MODs, one each on 3 different land territories you occupy.", 4);
            Add("Assemble MODs", CommanderType.Land, 1, B, CardKind.AssembleMods, TerritoryType.Land, "Place 3 MODs on one land territory you occupy.", 4);
            Add("Stealth MODs (Land)", CommanderType.Land, 0, R, CardKind.StealthMods, TerritoryType.Land, "After an invasion into a land territory is declared, place 3 additional defending MODs there.", 4);
            Add("Scout Forces", CommanderType.Land, 0, B, CardKind.ScoutForces, TerritoryType.Land, "Draw a land territory card. When you occupy that territory, immediately place 5 MODs on it.", 3);
            Add("Land Colony Influence", CommanderType.Land, 0, S, CardKind.ColonyInfluence, TerritoryType.Land, "Final scoring: +1 score (Land Commander must be in play).", 3);
            // Diplomat
            Add("Energy Crisis", CommanderType.Diplomat, 0, B, CardKind.EnergyCrisis, TerritoryType.Land, "Collect 1 energy from each opponent.", 4);
            Add("Cease Fire", CommanderType.Diplomat, 2, R, CardKind.CeaseFire, TerritoryType.Land, "After an opponent declares an invasion into a territory you occupy: the invasion is cancelled.", 4);
            Add("Redeployment", CommanderType.Diplomat, 1, B, CardKind.Redeployment, TerritoryType.Land, "Move up to 3 MODs from one territory you occupy to any other territory you occupy.", 4);
            Add("Diplomatic Influence", CommanderType.Diplomat, 0, S, CardKind.ColonyInfluence, TerritoryType.Land, "Final scoring: +1 score (Diplomat must be in play).", 3);
            // Naval
            Add("Reinforcements (Water)", CommanderType.Naval, 0, B, CardKind.Reinforcements, TerritoryType.Water, "Place 3 MODs, one each on 3 different water territories you occupy.", 4);
            Add("Energy Extraction", CommanderType.Naval, 0, B, CardKind.EnergyExtraction, TerritoryType.Water, "Collect 1 energy for each water territory you occupy (max 4).", 4);
            Add("Stealth MODs (Water)", CommanderType.Naval, 0, R, CardKind.StealthMods, TerritoryType.Water, "After an invasion into a water territory is declared, place 3 additional defending MODs there.", 3);
            Add("Assemble MODs (Water)", CommanderType.Naval, 1, B, CardKind.AssembleMods, TerritoryType.Water, "Place 3 MODs on one water territory you occupy.", 2);
            Add("Water Colony Influence", CommanderType.Naval, 0, S, CardKind.ColonyInfluence, TerritoryType.Water, "Final scoring: +1 score (Naval Commander must be in play).", 3);
            // Nuclear
            Add("Scatter Bomb (Land)", CommanderType.Nuclear, 1, B, CardKind.ScatterBomb, TerritoryType.Land, "Turn over 3 land territory cards. Destroy half the opponents' units there (round up).", 3);
            Add("Scatter Bomb (Water)", CommanderType.Nuclear, 1, B, CardKind.ScatterBomb, TerritoryType.Water, "Turn over 3 water territory cards. Destroy half the opponents' units there (round up).", 2);
            Add("Scatter Bomb (Moon)", CommanderType.Nuclear, 1, B, CardKind.ScatterBomb, TerritoryType.Moon, "Turn over 3 lunar territory cards. Destroy half the opponents' units there (round up).", 2);
            Add("The Mother", CommanderType.Nuclear, 3, B, CardKind.TheMother, TerritoryType.Land, "Draw a land territory card. Destroy all units there and in adjacent land territories, then devastate it.", 3);
            Add("Armageddon", CommanderType.Nuclear, 3, B, CardKind.Armageddon, TerritoryType.Land, "Every territory on the board loses half of its units (round down).", 2);
            Add("Nuclear Influence", CommanderType.Nuclear, 0, S, CardKind.ColonyInfluence, TerritoryType.Land, "Final scoring: +1 score (Nuclear Commander must be in play).", 2);
            // Space
            Add("Reinforcements (Moon)", CommanderType.Space, 0, B, CardKind.Reinforcements, TerritoryType.Moon, "Place 3 MODs, one each on 3 different lunar territories you occupy.", 4);
            Add("Invade Earth", CommanderType.Space, 2, B, CardKind.InvadeEarth, TerritoryType.Moon, "Draw a land territory card. This turn you may invade it from any lunar territory.", 3);
            Add("Stealth MODs (Moon)", CommanderType.Space, 0, R, CardKind.StealthMods, TerritoryType.Moon, "After an invasion into a lunar territory is declared, place 3 additional defending MODs there.", 3);
            Add("Assemble MODs (Moon)", CommanderType.Space, 1, B, CardKind.AssembleMods, TerritoryType.Moon, "Place 3 MODs on one lunar territory you occupy.", 2);
            Add("Lunar Colony Influence", CommanderType.Space, 0, S, CardKind.ColonyInfluence, TerritoryType.Moon, "Final scoring: +1 score (Space Commander must be in play).", 3);
        }
    }
}
