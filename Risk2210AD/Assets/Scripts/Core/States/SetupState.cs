using System.Collections.Generic;

namespace Risk2210.Core.States
{
    /// <summary>Devastation markers, neutral army, territory claiming, starting MODs, starting pieces.</summary>
    public sealed class SetupState : PhaseState
    {
        public override PhaseId Id => PhaseId.Setup;
        private readonly List<int> free = new List<int>();
        private int piecesPlayer;

        public override void Enter()
        {
            D.LogText("=== Setup ===");
            for (int i = 0; i < Rules.DevastationMarkers; i++)
            {
                int t = D.DrawTerritoryCard(TerritoryType.Land, skipDevastated: true);
                D.Devastate(t);
            }
            int startMods = S.NumPlayers == 3 ? 35 : S.NumPlayers == 4 ? 30 : S.NumPlayers == 5 ? 25 : 30;
            for (int p = 0; p < S.NumPlayers; p++) { S.Players[p].Pool = startMods; S.Players[p].Energy = Rules.StartingEnergy; }

            if (S.HasNeutral)
            {
                int n = S.NeutralPlayer;
                void Place(TerritoryType type, int count)
                {
                    int placed = 0, guard = 0;
                    while (placed < count && guard++ < 500)
                    {
                        int t = D.DrawTerritoryCard(type);
                        if (S.Territories[t].Devastated || S.Territories[t].Owner != -1) continue;
                        S.Territories[t].Owner = n; S.Territories[t].Mods = 3; placed++;
                    }
                }
                Place(TerritoryType.Land, 16); Place(TerritoryType.Water, 6); Place(TerritoryType.Moon, 6);
                D.LogText("Neutral armies placed on 16 land, 6 water and 6 lunar territories");
            }

            foreach (int t in Map.TerritoriesOfType(TerritoryType.Land))
                if (!S.Territories[t].Devastated && S.Territories[t].Owner == -1) free.Add(t);
            S.SetupStage = SetupStage.Claim;
            S.SetupActor = 0;
            D.LogText(D.Name(0) + " claims first");
        }

        public override CommandResult Handle(GameCommand cmd)
        {
            switch (cmd)
            {
                case ClaimTerritory c:
                {
                    if (S.SetupStage != SetupStage.Claim) return Fail("claiming is over");
                    if (!free.Contains(c.Territory)) return Fail("that territory is not available");
                    S.Territories[c.Territory].Owner = cmd.Player;
                    S.Territories[c.Territory].Mods = 1;
                    S.Players[cmd.Player].Pool--;
                    free.Remove(c.Territory);
                    D.EmitDeploy(cmd.Player, c.Territory, 1);
                    if (free.Count == 0) { S.SetupStage = SetupStage.PlaceMods; S.SetupActor = NextWithPool(0); }
                    else S.SetupActor = (cmd.Player + 1) % S.NumPlayers;
                    if (S.SetupStage == SetupStage.PlaceMods && S.SetupActor < 0) BeginPieces();
                    return Ok;
                }
                case PlaceStartingMod m:
                {
                    if (S.SetupStage != SetupStage.PlaceMods) return Fail("not placing MODs now");
                    if (!ValidTerritory(m.Territory) || S.Territories[m.Territory].Owner != cmd.Player) return Fail("you must choose a territory you control");
                    if (S.Players[cmd.Player].Pool <= 0) return Fail("no MODs left to place");
                    S.Territories[m.Territory].Mods++;
                    S.Players[cmd.Player].Pool--;
                    D.EmitDeploy(cmd.Player, m.Territory, 1);
                    S.SetupActor = NextWithPool(cmd.Player + 1);
                    if (S.SetupActor < 0) BeginPieces();
                    return Ok;
                }
                case PlaceStartingPieces pp:
                {
                    if (S.SetupStage != SetupStage.Pieces) return Fail("not placing pieces now");
                    foreach (int t in new[] { pp.SpaceStation, pp.LandCommander, pp.Diplomat })
                        if (!ValidTerritory(t) || S.Territories[t].Owner != cmd.Player) return Fail("place pieces on territories you control");
                    S.Territories[pp.SpaceStation].SpaceStation = true;
                    S.Territories[pp.LandCommander].Commanders[(int)CommanderType.Land] = true;
                    S.Territories[pp.Diplomat].Commanders[(int)CommanderType.Diplomat] = true;
                    var ev = GameEvent.Make(GameEventType.StationBuilt, $"{D.Name(cmd.Player)} starts with a Space Station in {D.TName(pp.SpaceStation)}, Land Commander in {D.TName(pp.LandCommander)}, Diplomat in {D.TName(pp.Diplomat)}");
                    ev.Player = cmd.Player; ev.To = pp.SpaceStation; D.Emit(ev);
                    piecesPlayer++;
                    if (piecesPlayer >= S.NumPlayers) { S.SetupStage = SetupStage.Done; S.SetupActor = -1; D.Transition(new YearInitState()); }
                    else S.SetupActor = piecesPlayer;
                    return Ok;
                }
                default:
                    return Fail("not allowed during setup");
            }
        }

        private int NextWithPool(int start)
        {
            for (int i = 0; i < S.NumPlayers; i++)
            {
                int p = (start + i) % S.NumPlayers;
                if (S.Players[p].Pool > 0) return p;
            }
            return -1;
        }

        private void BeginPieces()
        {
            S.SetupStage = SetupStage.Pieces;
            piecesPlayer = 0;
            S.SetupActor = 0;
            D.LogText("Place your Space Station, Land Commander and Diplomat");
        }
    }
}
