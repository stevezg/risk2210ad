using System;
using System.Collections.Generic;
using System.Linq;

namespace Risk2210.Core.States
{
    /// <summary>Declare invasions, resolve dice, capture territories, move in.</summary>
    public sealed class InvasionState : PhaseState
    {
        public override PhaseId Id => PhaseId.Invasion;

        public override CommandResult Handle(GameCommand cmd)
        {
            int p = cmd.Player;
            var inv = S.Invasion;
            switch (cmd)
            {
                case PlayCard pc:
                    return DeploymentState.PlayBeforeInvasionCard(D, p, pc.HandIndex);
                case DeclareInvasion di:
                    return Declare(p, di.From, di.To);
                case Attack a:
                    return BeginAttack(p, a.Dice);
                case MoveIn m:
                    return DoMoveIn(p, m.Count);
                case EndInvasion _:
                    if (!inv.Active || inv.Attacker != p) return Fail("no invasion in progress");
                    if (inv.Captured) return Fail("you must move units into the captured territory first");
                    if (!inv.AttackedOnce) return Fail("you must attack at least once before calling off an invasion");
                    S.Invasion = new Invasion();
                    D.Emit(GameEvent.Make(GameEventType.InvasionCancelled));
                    return Ok;
                case EndInvasionPhase _:
                    if (inv.Active && inv.Captured) return Fail("move units into the captured territory first");
                    if (inv.Active && !inv.AttackedOnce) return Fail("you must attack at least once before calling off an invasion");
                    S.Invasion = new Invasion();
                    D.Transition(new FortificationState());
                    return Ok;
                default:
                    return Fail("not allowed during the invasion phase");
            }
        }

        // ------------------------------------------------------------------

        private CommandResult Declare(int p, int from, int to)
        {
            if (S.Invasion.Active) return Fail("an invasion is already in progress");
            if (!S.CanInvade(p, from, to, out string why)) return Fail(why);

            var inv = new Invasion { Active = true, Attacker = p, From = from, To = to, Defender = S.Territories[to].Owner, Contested = S.Territories[to].Units > 0 };
            S.Invasion = inv;
            S.Players[p].InvasionDeclaredThisTurn = true;
            S.Players[p].InvasionsThisTurn++;
            var ev = GameEvent.Make(GameEventType.InvasionDeclared, $"{D.Name(p)} invades {D.TName(to)} from {D.TName(from)}");
            ev.Player = p; ev.From = from; ev.To = to; D.Emit(ev);

            if (!inv.Contested) { OccupyEmpty(); return Ok; }

            // Reactive cards: defender first, then everyone else who can play one.
            var reactors = new Queue<int>();
            if (S.IsActive(inv.Defender) && S.HasReactiveCard(inv.Defender)) reactors.Enqueue(inv.Defender);
            for (int q = 0; q < S.NumPlayers; q++)
                if (q != inv.Defender && q != p && S.IsActive(q) && S.HasReactiveCard(q)) reactors.Enqueue(q);
            PromptNextReactor(reactors);
            return Ok;
        }

        private void PromptNextReactor(Queue<int> reactors)
        {
            if (!S.Invasion.Active || reactors.Count == 0) return;
            int q = reactors.Peek();
            if (!S.HasReactiveCard(q)) { reactors.Dequeue(); PromptNextReactor(reactors); return; }
            D.OpenPrompt(new Prompt
            {
                Kind = PromptKind.ReactiveCard, Player = q,
                Text = $"{D.Name(S.Invasion.Attacker)} invades {D.TName(S.Invasion.To)} from {D.TName(S.Invasion.From)} - play a reactive card?",
                OnResponse = idx =>
                {
                    if (idx < 0) { reactors.Dequeue(); PromptNextReactor(reactors); return; }
                    ApplyReactiveCard(q, idx);
                    PromptNextReactor(reactors);   // same player may play another
                }
            });
        }

        private void ApplyReactiveCard(int q, int handIndex)
        {
            var ps = S.Players[q];
            var card = CardCatalogue.Get(ps.Hand[handIndex]);
            if (card.Cost > 0) D.ChangeEnergy(q, -card.Cost);
            ps.Hand.RemoveAt(handIndex);
            var ev = GameEvent.Make(GameEventType.CardPlayed, $"{ps.Name} plays {card.Name}"); ev.Player = q; ev.Amount = card.Id; D.Emit(ev);
            if (card.Kind == CardKind.StealthMods)
            {
                S.Territories[S.Invasion.To].Mods += 3;
                D.EmitDeploy(S.Territories[S.Invasion.To].Owner, S.Invasion.To, 3);
            }
            else if (card.Kind == CardKind.CeaseFire)
            {
                S.Invasion = new Invasion();
                D.Emit(GameEvent.Make(GameEventType.InvasionCancelled, "The invasion is cancelled by Cease Fire"));
            }
        }

        private void OccupyEmpty()
        {
            var inv = S.Invasion;
            var d = S.Territories[inv.To];
            int old = d.Owner;
            d.Owner = inv.Attacker;
            if (old >= 0 && old != inv.Attacker && d.SpaceStation && S.CountSpaceStations(inv.Attacker) > Rules.MaxSpaceStations) d.SpaceStation = false;
            inv.Captured = true; inv.LastDice = 1; inv.AttackedOnce = true;
            var ev = GameEvent.Make(GameEventType.TerritoryCaptured, $"{D.Name(inv.Attacker)} occupies {D.TName(inv.To)}");
            ev.Player = inv.Attacker; ev.OtherPlayer = old; ev.To = inv.To; ev.From = inv.From; D.Emit(ev);
        }

        // ------------------------------------------------------------------

        private CommandResult BeginAttack(int p, int nDice)
        {
            var inv = S.Invasion;
            if (!inv.Active || inv.Attacker != p) return Fail("no invasion in progress");
            if (inv.Captured) return Fail("territory already captured; move units in");
            var f = S.Territories[inv.From]; var d = S.Territories[inv.To];
            int maxA = Math.Min(3, f.Units - 1);
            if (maxA < 1) return Fail("not enough units to attack");
            if (nDice < 1 || nDice > maxA) return Fail("you may roll between 1 and " + maxA + " dice");

            int maxD = Math.Min(2, d.Units);
            bool humanDefender = S.IsActive(inv.Defender) && !S.Players[inv.Defender].IsBot;
            if (maxD == 2 && humanDefender)
            {
                inv.PendingAttackDice = nDice;
                D.OpenPrompt(new Prompt
                {
                    Kind = PromptKind.DefenseDice, Player = inv.Defender, MaxDice = maxD,
                    Text = $"{D.Name(p)} attacks {D.TName(inv.To)} with {nDice} dice - defend with how many?",
                    OnResponse = dd => ResolveAttack(nDice, dd)
                });
                return Ok;
            }
            ResolveAttack(nDice, maxD);
            return Ok;
        }

        private void ResolveAttack(int nDice, int dDice)
        {
            var inv = S.Invasion;
            int p = inv.Attacker;
            var f = S.Territories[inv.From]; var d = S.Territories[inv.To];
            var which = new bool[MapGraph.NumCommanders];
            int a8 = S.AttackD8Count(inv.From, inv.To, nDice, which);
            int d8 = S.DefendD8Count(inv.To, dDice);

            var ar = new int[nDice]; var dr = new int[dDice];
            for (int i = 0; i < nDice; i++) ar[i] = D.RollDie(i < a8 ? 8 : 6);
            for (int i = 0; i < dDice; i++) dr[i] = D.RollDie(i < d8 ? 8 : 6);
            Array.Sort(ar); Array.Reverse(ar);
            Array.Sort(dr); Array.Reverse(dr);
            int aLost = 0, dLost = 0;
            for (int i = 0; i < Math.Min(nDice, dDice); i++) { if (ar[i] > dr[i]) dLost++; else aLost++; }

            int defender = inv.Defender;
            inv.AttackedOnce = true;
            inv.LastDice = nDice;

            var ev = GameEvent.Make(GameEventType.Battle,
                $"  {D.Name(p)} rolls {string.Join(" ", ar)}{(a8 > 0 ? $" ({a8}xd8)" : "")} vs {string.Join(" ", dr)}{(d8 > 0 ? $" ({d8}xd8)" : "")} -> attacker -{aLost}, defender -{dLost}");
            ev.Player = p; ev.OtherPlayer = defender; ev.From = inv.From; ev.To = inv.To;
            ev.AttackRolls = ar; ev.DefendRolls = dr; ev.AttackD8 = a8; ev.DefendD8 = d8; ev.Amount = aLost; ev.Amount2 = dLost;
            bool captured = d.Units - dLost <= 0;
            ev.Captured = captured;
            D.Emit(ev);

            // Apply losses silently (the Battle event carries the totals for the view)
            RemoveUnitsQuiet(inv.From, aLost);
            RemoveUnitsQuiet(inv.To, dLost);

            if (captured)
            {
                inv.Captured = true;
                for (int c = 0; c < MapGraph.NumCommanders; c++) inv.MustMoveIn[c] = which[c] && f.Commanders[c];
                if (d.SpaceStation && S.CountSpaceStations(p) >= Rules.MaxSpaceStations) { d.SpaceStation = false; D.LogText($"  the Space Station in {D.TName(inv.To)} is destroyed"); }
                d.Owner = p;
                var cap = GameEvent.Make(GameEventType.TerritoryCaptured, $"  {D.Name(p)} captures {D.TName(inv.To)}");
                cap.Player = p; cap.OtherPlayer = defender; cap.To = inv.To; cap.From = inv.From; D.Emit(cap);

                var ps = S.Players[p];
                if (inv.Contested) ps.ContestedCaptures++;
                if (ps.ContestedCaptures >= 3 && !ps.BonusClaimed)
                {
                    ps.BonusClaimed = true;
                    D.ChangeEnergy(p, 1);
                    var opts = new List<CommanderType>();
                    for (int c = 0; c < MapGraph.NumCommanders; c++)
                        if (S.CommanderInPlay(p, (CommanderType)c) && D.DeckSize((CommanderType)c) > 0) opts.Add((CommanderType)c);
                    D.LogText($"  3-territory bonus: {ps.Name} gains 1 energy and 1 command card");
                    if (opts.Count > 0)
                        D.OpenPrompt(new Prompt { Kind = PromptKind.BonusDeck, Player = p, Decks = opts, Text = "3-territory bonus: draw a card from which deck?",
                            OnResponse = i => ps.Hand.Add(D.DrawCommandCard(opts[i])) });
                }
                D.CheckElimination(defender);
            }
        }

        private void RemoveUnitsQuiet(int t, int n)
        {
            var ts = S.Territories[t];
            int m = Math.Min(n, ts.Mods);
            ts.Mods -= m;
            int removed = m;
            for (int c = 0; c < MapGraph.NumCommanders && removed < n; c++)
                if (ts.Commanders[c]) { ts.Commanders[c] = false; removed++; }
            if (ts.Units == 0 && !ts.SpaceStation && ts.Owner != S.Invasion.Attacker) ts.Owner = -1;
        }

        // ------------------------------------------------------------------

        private CommandResult DoMoveIn(int p, int n)
        {
            var inv = S.Invasion;
            if (!inv.Active || inv.Attacker != p) return Fail("no invasion in progress");
            if (!inv.Captured) return Fail("territory not captured yet");
            var f = S.Territories[inv.From]; var d = S.Territories[inv.To];
            int maxMove = f.Units - 1;
            int minMove = Math.Max(1, Math.Min(inv.LastDice, maxMove));
            if (n < minMove) return Fail("you must move in at least " + minMove + " unit(s)");
            if (n > maxMove) return Fail("you must leave at least one unit behind");

            int moved = 0;
            for (int c = 0; c < MapGraph.NumCommanders; c++)
                if (inv.MustMoveIn[c] && f.Commanders[c]) { f.Commanders[c] = false; d.Commanders[c] = true; moved++; }
            n = Math.Max(n, moved);
            int m = Math.Min(n - moved, f.Mods);
            f.Mods -= m; d.Mods += m; moved += m;
            for (int c = 0; c < MapGraph.NumCommanders && moved < n; c++)
                if (f.Commanders[c]) { f.Commanders[c] = false; d.Commanders[c] = true; moved++; }
            d.Owner = p;
            var ev = GameEvent.Make(GameEventType.UnitsMoved, $"  {D.Name(p)} moves {moved} unit(s) into {D.TName(inv.To)}");
            ev.Player = p; ev.From = inv.From; ev.To = inv.To; ev.Amount = moved; D.Emit(ev);

            var ps = S.Players[p];
            if (ps.ScoutTerritory == inv.To) { d.Mods += 5; ps.ScoutTerritory = -1; D.EmitDeploy(p, inv.To, 5); D.LogText("  scout forces arrive: +5 MODs"); }
            S.Invasion = new Invasion();
            return Ok;
        }
    }
}
