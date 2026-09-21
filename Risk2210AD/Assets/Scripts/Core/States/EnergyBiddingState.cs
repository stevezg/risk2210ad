using System.Collections.Generic;
using System.Linq;

namespace Risk2210.Core.States
{
    /// <summary>Secret sealed-bid auction for turn order; highest bidder picks a marker first.</summary>
    public sealed class EnergyBiddingState : PhaseState
    {
        public override PhaseId Id => PhaseId.EnergyBidding;
        private readonly Dictionary<int, int> tiebreak = new Dictionary<int, int>();
        private int chooserIndex;

        public override void Enter()
        {
            for (int p = 0; p < S.NumPlayers; p++) S.Players[p].Bid = -1;
            S.BidChooser = -1;
        }

        public override CommandResult Handle(GameCommand cmd)
        {
            switch (cmd)
            {
                case SubmitBid b:
                {
                    if (S.BidChooser >= 0) return Fail("bids are already revealed");
                    if (!S.IsActive(cmd.Player)) return Fail("eliminated players do not bid");
                    var ps = S.Players[cmd.Player];
                    if (ps.Bid >= 0) return Fail("you already bid");
                    if (b.Amount < 0 || b.Amount > ps.Energy) return Fail("bid between 0 and your energy");
                    ps.Bid = b.Amount;
                    tiebreak[cmd.Player] = D.RollDie(6);
                    if (AllBid()) Reveal();
                    return Ok;
                }
                case ChooseTurnOrder c:
                {
                    if (S.BidChooser < 0) return Fail("bids are still secret");
                    if (!S.AvailableMarkers.Contains(c.Marker)) return Fail("that marker is taken");
                    S.TurnOrder[c.Marker] = cmd.Player;
                    S.AvailableMarkers.Remove(c.Marker);
                    D.LogText($"{D.Name(cmd.Player)} bid {S.Players[cmd.Player].Bid} energy and takes turn marker #{c.Marker + 1}");
                    chooserIndex++;
                    if (chooserIndex >= S.BidRanking.Count) FinishOrder();
                    else S.BidChooser = S.BidRanking[chooserIndex];
                    return Ok;
                }
                default:
                    return Fail("bidding is in progress");
            }
        }

        private bool AllBid()
        {
            for (int p = 0; p < S.NumPlayers; p++) if (S.IsActive(p) && S.Players[p].Bid < 0) return false;
            return true;
        }

        private void Reveal()
        {
            var ranking = Enumerable.Range(0, S.NumPlayers).Where(S.IsActive).ToList();
            ranking.Sort((a, b) =>
            {
                int c = S.Players[b].Bid.CompareTo(S.Players[a].Bid);
                return c != 0 ? c : tiebreak[b].CompareTo(tiebreak[a]);
            });
            foreach (int p in ranking) D.ChangeEnergy(p, -S.Players[p].Bid);
            S.BidRanking = ranking;
            S.TurnOrder = Enumerable.Repeat(-1, ranking.Count).ToList();
            S.AvailableMarkers = Enumerable.Range(0, ranking.Count).ToList();
            chooserIndex = 0;
            S.BidChooser = ranking[0];
            var ev = GameEvent.Make(GameEventType.BidsRevealed, "Bids: " + string.Join(", ", ranking.Select(p => $"{D.Name(p)} {S.Players[p].Bid}")));
            D.Emit(ev);
        }

        private void FinishOrder()
        {
            S.BidChooser = -1;
            S.TurnIndex = -1;
            D.Emit(GameEvent.Make(GameEventType.TurnOrderSet, "Turn order: " + string.Join(" > ", S.TurnOrder.Select(D.Name))));
            D.EndTurn();   // advances TurnIndex to the first active player and enters Deployment
        }
    }
}
