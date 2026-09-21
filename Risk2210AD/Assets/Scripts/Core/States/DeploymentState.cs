using System.Collections.Generic;

namespace Risk2210.Core.States
{
    /// <summary>Collect MODs/energy, deploy, hire commanders, build stations, buy and play cards.</summary>
    public sealed class DeploymentState : PhaseState
    {
        public override PhaseId Id => PhaseId.Deployment;

        public override void Enter() => D.StartTurn(S.TurnOrder[S.TurnIndex]);

        public override CommandResult Handle(GameCommand cmd)
        {
            int p = cmd.Player;
            var ps = S.Players[p];
            switch (cmd)
            {
                case DeployMods d:
                {
                    if (!ValidTerritory(d.Territory) || S.Territories[d.Territory].Owner != p) return Fail("you do not control that territory");
                    if (d.Count <= 0 || d.Count > ps.Pool) return Fail("invalid MOD count");
                    S.Territories[d.Territory].Mods += d.Count;
                    ps.Pool -= d.Count;
                    D.EmitDeploy(p, d.Territory, d.Count);
                    return Ok;
                }
                case HireCommander h:
                {
                    if (ps.Pool > 0) return Fail("deploy all your MODs first");
                    if (S.CommanderInPlay(p, h.Commander)) return Fail("that commander is already in play");
                    if (ps.Energy < Rules.CommanderCost) return Fail("not enough energy");
                    if (!ValidTerritory(h.Territory) || S.Territories[h.Territory].Owner != p) return Fail("you do not control that territory");
                    D.ChangeEnergy(p, -Rules.CommanderCost);
                    S.Territories[h.Territory].Commanders[(int)h.Commander] = true;
                    var ev = GameEvent.Make(GameEventType.CommanderHired, $"{ps.Name} hires a {h.Commander} Commander in {D.TName(h.Territory)}");
                    ev.Player = p; ev.To = h.Territory; ev.Amount = (int)h.Commander; D.Emit(ev);
                    return Ok;
                }
                case BuildSpaceStation b:
                {
                    if (ps.Pool > 0) return Fail("deploy all your MODs first");
                    if (S.CountSpaceStations(p) >= Rules.MaxSpaceStations) return Fail("you already control 4 Space Stations");
                    if (ps.Energy < Rules.SpaceStationCost) return Fail("not enough energy");
                    if (!ValidTerritory(b.Territory) || S.Territories[b.Territory].Owner != p) return Fail("you do not control that territory");
                    if (Map[b.Territory].Type != TerritoryType.Land) return Fail("Space Stations may only be built on land");
                    if (S.Territories[b.Territory].SpaceStation) return Fail("that territory already has a Space Station");
                    D.ChangeEnergy(p, -Rules.SpaceStationCost);
                    S.Territories[b.Territory].SpaceStation = true;
                    var ev = GameEvent.Make(GameEventType.StationBuilt, $"{ps.Name} builds a Space Station in {D.TName(b.Territory)}");
                    ev.Player = p; ev.To = b.Territory; D.Emit(ev);
                    return Ok;
                }
                case BuyCards bc:
                {
                    if (ps.Pool > 0) return Fail("deploy all your MODs first");
                    if (bc.Decks.Count == 0) return Fail("no decks chosen");
                    if (ps.CardsBoughtThisTurn + bc.Decks.Count > Rules.MaxCardsPerTurn) return Fail("you may buy at most 4 cards per turn");
                    if (ps.Energy < Rules.CardCost * bc.Decks.Count) return Fail("not enough energy");
                    var want = new int[MapGraph.NumCommanders];
                    foreach (var c in bc.Decks)
                    {
                        if (!S.CommanderInPlay(p, c)) return Fail(c + " Commander is not in play");
                        if (++want[(int)c] > D.DeckSize(c)) return Fail(c + " deck is exhausted");
                    }
                    foreach (var c in bc.Decks)
                    {
                        ps.Hand.Add(D.DrawCommandCard(c));
                        ps.CardsBoughtThisTurn++;
                    }
                    D.ChangeEnergy(p, -Rules.CardCost * bc.Decks.Count);
                    var ev = GameEvent.Make(GameEventType.CardsBought, $"{ps.Name} buys {bc.Decks.Count} command card(s)"); ev.Player = p; ev.Amount = bc.Decks.Count; D.Emit(ev);
                    return Ok;
                }
                case PlayCard pc:
                    if (ps.Pool > 0) return Fail("deploy all your MODs first");
                    return PlayCardNow(D, p, pc.HandIndex, CardTiming.BeforeFirstInvasion);
                case EndDeployment _:
                    if (ps.Pool > 0) return Fail("deploy all your MODs first");
                    D.Transition(new InvasionState());
                    return Ok;
                default:
                    return Fail("not allowed during deployment");
            }
        }

        /// <summary>Shared by Deployment, Invasion (before-first-invasion cards) and Fortification (end-of-turn cards).</summary>
        public static CommandResult PlayCardNow(GameDirector D, int p, int handIndex, CardTiming allowed)
        {
            var S = D.State;
            var ps = S.Players[p];
            if (handIndex < 0 || handIndex >= ps.Hand.Count) return CommandResult.Fail("no such card");
            var card = CardCatalogue.Get(ps.Hand[handIndex]);
            if (card.Timing != allowed) return CommandResult.Fail("that card cannot be played now");
            if (allowed == CardTiming.BeforeFirstInvasion && ps.InvasionDeclaredThisTurn) return CommandResult.Fail("you have already declared an invasion this turn");
            if (!S.CommanderInPlay(p, card.Deck)) return CommandResult.Fail(card.Deck + " Commander is not in play");
            int cost = S.CardCost(p, card);
            if (ps.Energy < cost) return CommandResult.Fail("not enough energy");
            if (cost > 0) D.ChangeEnergy(p, -cost);
            ps.Hand.RemoveAt(handIndex);
            var ev = GameEvent.Make(GameEventType.CardPlayed, $"{ps.Name} plays {card.Name}"); ev.Player = p; ev.Amount = card.Id; D.Emit(ev);
            D.ApplyCard(p, card, () => { });
            return CommandResult.Success;
        }
    }
}
