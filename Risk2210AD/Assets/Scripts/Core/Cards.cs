using System.Collections.Generic;

namespace Risk2210.Core
{
    public enum CardTiming { BeforeFirstInvasion, OnInvasionDeclared, EndOfTurn, Scoring }

    /// <summary>Engine-implemented card effects. Several cards share a kind and differ by target type.</summary>
    public enum CardKind
    {
        Reinforcements,     // 3 MODs, one each on 3 different territories of the target type you occupy
        AssembleMods,       // 3 MODs on one territory of the target type you occupy
        StealthMods,        // reactive: +3 defending MODs (target type must match the defending territory)
        StealthStation,     // reactive: a Space Station appears in the defending land territory
        DeathTrap,          // reactive: attacker destroys half the units in the invading territory (round up)
        CeaseFire,          // reactive: invasion prevented; attacker may not attack your territories this turn
        Evacuation,         // reactive: move every unit from the attacked territory to any territory you occupy
        ColonyInfluence,    // scoring: +3 if the deck's commander is alive
        EnergyCrisis,       // collect 1 energy from each opponent
        DecoysRevealed,     // move any of your commanders to territories you occupy
        ModReduction,       // each opponent removes 4 MODs (in turn order), then you remove 2
        Redeployment,       // end of turn: an extra fortify move
        TerritorialStation, // place a Space Station on a land territory you occupy
        FrequencyJam,       // a chosen player cannot play command cards during your turn
        ScoutForces,        // draw a land card; when you occupy it, place 5 MODs there
        HiddenEnergy,       // draw a water card; if you occupy it at end of turn, collect 4 energy
        ZoneStrike,         // roll d6 → a continent/colony of the target type; destroy one unit in every territory there
        AssassinBomb,       // choose an opponent's commander; d8 ≥ 3 destroys it
        Armageddon,         // this turn your nuclear command cards cost no energy
        RocketStrike,       // choose an opponent's territory of the target type; roll d6; destroy that many units
        ScatterBomb,        // turn over N territory cards of the target type; destroy half the opponents' units there (round up)
        InvadeEarth,        // draw land cards until one you don't occupy; you may attack it from any lunar territory this turn
        EnergyExtraction    // if you occupy every territory of a lunar colony at end of turn, collect 7 energy
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
        public int Amount;      // kind-specific (e.g. Scatter Bomb card count)
        public string Text;
    }

    /// <summary>The five base-game command decks (Risk 2210 A.D. Command Card Summary).</summary>
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

        private static void Add(string name, CommanderType deck, int n, int cost, CardTiming timing, CardKind kind, TerritoryType target, string text, int amount = 0)
        {
            defs.Add(new CardDef { Id = defs.Count, Name = name, Deck = deck, Cost = cost, Timing = timing, Kind = kind, Target = target, Text = text, Amount = amount });
            copies.Add(n);
        }

        private static void EnsureBuilt()
        {
            if (defs.Count > 0) return;
            const CardTiming B = CardTiming.BeforeFirstInvasion, R = CardTiming.OnInvasionDeclared, E = CardTiming.EndOfTurn, S = CardTiming.Scoring;
            const TerritoryType L = TerritoryType.Land, W = TerritoryType.Water, M = TerritoryType.Moon;
            var D = CommanderType.Diplomat; var La = CommanderType.Land; var N = CommanderType.Naval; var X = CommanderType.Nuclear; var Sp = CommanderType.Space;

            // ---- Diplomat (18) ------------------------------------------------------
            Add("Cease Fire", D, 2, 2, R, CardKind.CeaseFire, L, "Prevent the invasion. The attacking player cannot attack any of your territories for the rest of their turn.");
            Add("Colony Influence", D, 4, 0, S, CardKind.ColonyInfluence, L, "If your Diplomat Commander is still alive, move your score marker ahead 3 spaces.");
            Add("Decoys Revealed", D, 2, 0, B, CardKind.DecoysRevealed, L, "Move any number of your commanders to any number of territories you control.");
            Add("Energy Crisis", D, 2, 0, B, CardKind.EnergyCrisis, L, "Collect one energy from each opponent.");
            Add("Evacuation", D, 2, 0, R, CardKind.Evacuation, L, "Move all units from the attacked territory to any territory you occupy.");
            Add("MOD Reduction", D, 2, 2, B, CardKind.ModReduction, L, "All of your opponents must remove 4 MODs in turn order. Then you remove 2 MODs.");
            Add("Redeployment", D, 3, 0, E, CardKind.Redeployment, L, "Take an extra free move this turn, after you have finished attacking.");
            Add("Territorial Station", D, 3, 1, B, CardKind.TerritorialStation, L, "Place a Space Station on any land territory you occupy.");

            // ---- Land (20) ------------------------------------------------------------
            Add("Assemble MODs", La, 3, 1, B, CardKind.AssembleMods, L, "Place 3 MODs on any one land territory you occupy.");
            Add("Colony Influence", La, 2, 0, S, CardKind.ColonyInfluence, L, "If your Land Commander is still alive, move your score marker ahead 3 spaces.");
            Add("Frequency Jam", La, 2, 0, B, CardKind.FrequencyJam, L, "Choose a player. The chosen player cannot play command cards during your turn.");
            Add("Land Death Trap", La, 1, 3, R, CardKind.DeathTrap, L, "Your opponent must destroy half the units in the invading territory. Round up.");
            Add("Reinforcements", La, 3, 0, B, CardKind.Reinforcements, L, "Place 3 MODs, one each on 3 different land territories you occupy.");
            Add("Scout Forces", La, 3, 0, B, CardKind.ScoutForces, L, "Draw a land territory card and keep it secret. When you occupy this territory, immediately place 5 MODs there.");
            Add("Stealth MODs", La, 5, 0, R, CardKind.StealthMods, L, "Place 3 additional defending MODs in the defending land territory.");
            Add("Stealth Station", La, 1, 0, R, CardKind.StealthStation, L, "Place a Space Station in the defending land territory.");

            // ---- Naval (20) -----------------------------------------------------------
            Add("Assemble MODs", N, 3, 1, B, CardKind.AssembleMods, W, "Place 3 MODs on any one water territory you occupy.");
            Add("Colony Influence", N, 2, 0, S, CardKind.ColonyInfluence, W, "If your Naval Commander is still alive, move your score marker ahead 3 spaces.");
            Add("Frequency Jam", N, 2, 0, B, CardKind.FrequencyJam, W, "Choose a player. The chosen player cannot play command cards during your turn.");
            Add("Hidden Energy", N, 5, 0, B, CardKind.HiddenEnergy, W, "Draw a water territory card. If you occupy this water territory at the end of your turn, collect 4 energy.");
            Add("Reinforcements", N, 2, 0, B, CardKind.Reinforcements, W, "Place 3 MODs, one each on 3 different water territories you occupy.");
            Add("Stealth MODs", N, 5, 0, R, CardKind.StealthMods, W, "Place 3 additional defending MODs in the defending water territory.");
            Add("Water Death Trap", N, 1, 3, R, CardKind.DeathTrap, W, "Your opponent must destroy half the units in the invading territory. Round up.");

            // ---- Nuclear (18) ---------------------------------------------------------
            Add("Aqua Brother", X, 1, 3, B, CardKind.ZoneStrike, W, "Roll a 6-sided die. Destroy one unit in each territory in the water colony rolled (1-5; 6 = roll again).");
            Add("Assassin Bomb", X, 3, 1, B, CardKind.AssassinBomb, L, "Choose an opponent's commander. Roll an 8-sided die. On a 3 or higher, destroy the chosen commander.");
            Add("Armageddon", X, 1, 4, B, CardKind.Armageddon, L, "Nuclear exchange: this turn, your nuclear command cards may be played without paying their energy cost.");
            Add("The Mother", X, 1, 3, B, CardKind.ZoneStrike, L, "Roll a 6-sided die. Destroy one unit in each territory in the continent rolled: 1 N. America, 2 S. America, 3 Europe, 4 Africa, 5 Asia, 6 Australia.");
            Add("Nicky Boy", X, 1, 3, B, CardKind.ZoneStrike, M, "Roll a 6-sided die. Destroy one unit in each territory in the lunar colony rolled: 1-2 Cresinion, 3-4 Delphot, 5-6 Sajon.");
            Add("Rocket Strike Land", X, 2, 2, B, CardKind.RocketStrike, L, "Choose any opponent's land territory. Roll a 6-sided die. Your opponent must destroy units equal to the number rolled.");
            Add("Rocket Strike Moon", X, 2, 2, B, CardKind.RocketStrike, M, "Choose any opponent's lunar territory. Roll a 6-sided die. Your opponent must destroy units equal to the number rolled.");
            Add("Rocket Strike Water", X, 2, 2, B, CardKind.RocketStrike, W, "Choose any opponent's water territory. Roll a 6-sided die. Your opponent must destroy units equal to the number rolled.");
            Add("Scatter Bomb Land", X, 3, 1, B, CardKind.ScatterBomb, L, "Turn over 3 land territory cards. Destroy half the opponents' units on the territories drawn. Round up.", 3);
            Add("Scatter Bomb Moon", X, 2, 1, B, CardKind.ScatterBomb, M, "Turn over 2 lunar territory cards. Destroy half the opponents' units on the territories drawn. Round up.", 2);
            Add("Scatter Bomb Water", X, 2, 1, B, CardKind.ScatterBomb, W, "Turn over 2 water territory cards. Destroy half the opponents' units on the territories drawn. Round up.", 2);

            // ---- Space (18) -----------------------------------------------------------
            Add("Assemble MODs", Sp, 3, 1, B, CardKind.AssembleMods, M, "Place 3 MODs on any one lunar territory you control.");
            Add("Colony Influence", Sp, 2, 0, S, CardKind.ColonyInfluence, M, "If your Space Commander is still alive, move your score marker ahead 3 spaces.");
            Add("Energy Extraction", Sp, 1, 1, B, CardKind.EnergyExtraction, M, "If you occupy all the lunar territories in a colony at the end of this turn, collect 7 energy.");
            Add("Frequency Jam", Sp, 2, 0, B, CardKind.FrequencyJam, M, "Choose a player. The chosen player cannot play command cards during your turn.");
            Add("Invade Earth", Sp, 3, 0, B, CardKind.InvadeEarth, M, "Turn over land territory cards until you find one you do not occupy. This turn you may attack it from any lunar territory you occupy.");
            Add("Orbital Mines", Sp, 2, 2, R, CardKind.DeathTrap, M, "Your opponent must destroy half the units in the invading territory. Round up.");
            Add("Reinforcements", Sp, 3, 0, B, CardKind.Reinforcements, M, "Place 3 MODs, one each on 3 different lunar territories you occupy.");
            Add("Stealth MODs", Sp, 4, 0, R, CardKind.StealthMods, M, "Place 3 additional defending MODs in the defending lunar territory.");
        }
    }
}
