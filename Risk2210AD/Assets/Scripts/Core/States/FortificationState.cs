namespace Risk2210.Core.States
{
    /// <summary>One free move along a chain of friendly territories, then the turn ends.</summary>
    public sealed class FortificationState : PhaseState
    {
        public override PhaseId Id => PhaseId.Fortification;

        public override CommandResult Handle(GameCommand cmd)
        {
            int p = cmd.Player;
            switch (cmd)
            {
                case Fortify ft:
                {
                    if (!ValidTerritory(ft.From) || !ValidTerritory(ft.To) || ft.From == ft.To) return Fail("invalid territories");
                    var f = S.Territories[ft.From]; var d = S.Territories[ft.To];
                    if (f.Owner != p || d.Owner != p) return Fail("you must control both territories");
                    int cmdMoves = 0;
                    for (int c = 0; c < MapGraph.NumCommanders; c++)
                        if (ft.Commanders != null && ft.Commanders[c]) { if (!f.Commanders[c]) return Fail("that commander is not in the source territory"); cmdMoves++; }
                    if (ft.Mods < 0 || ft.Mods > f.Mods) return Fail("invalid MOD count");
                    if (ft.Mods + cmdMoves <= 0) return Fail("nothing to move");
                    if (ft.Mods + cmdMoves > f.Units - 1) return Fail("you must leave at least one unit behind");
                    if (!S.FortifyPathExists(p, ft.From, ft.To)) return Fail("no path of friendly territories connects them");
                    f.Mods -= ft.Mods; d.Mods += ft.Mods;
                    for (int c = 0; c < MapGraph.NumCommanders; c++)
                        if (ft.Commanders != null && ft.Commanders[c]) { f.Commanders[c] = false; d.Commanders[c] = true; }
                    S.Players[p].FortifiedThisTurn = true;
                    var ev = GameEvent.Make(GameEventType.Fortified, $"{D.Name(p)} fortifies {ft.Mods + cmdMoves} unit(s) from {D.TName(ft.From)} to {D.TName(ft.To)}");
                    ev.Player = p; ev.From = ft.From; ev.To = ft.To; ev.Amount = ft.Mods + cmdMoves; D.Emit(ev);
                    if (S.Players[p].ExtraFortifies > 0) { S.Players[p].ExtraFortifies--; D.LogText($"{D.Name(p)} may fortify again"); return Ok; }
                    D.EndTurn();
                    return Ok;
                }
                case PlayCard pc:
                    return DeploymentState.PlayCardNow(D, p, pc.HandIndex, CardTiming.EndOfTurn);
                case EndFortification _:
                    D.EndTurn();
                    return Ok;
                default:
                    return Fail("not allowed during fortification");
            }
        }
    }
}
