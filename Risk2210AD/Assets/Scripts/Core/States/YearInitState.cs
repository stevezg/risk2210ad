namespace Risk2210.Core.States
{
    /// <summary>Advances the year marker; ends the game after year 5.</summary>
    public sealed class YearInitState : PhaseState
    {
        public override PhaseId Id => PhaseId.YearInit;

        public override void Enter()
        {
            if (S.Year >= Rules.NumYears) { D.Transition(new GameOverState()); return; }
            S.Year++;
            S.TurnOrder.Clear(); S.TurnIndex = 0; S.BidChooser = -1; S.AvailableMarkers.Clear(); S.BidRanking.Clear();
            for (int p = 0; p < S.NumPlayers; p++) S.Players[p].Bid = -1;
            var ev = GameEvent.Make(GameEventType.YearStarted, $"=== Year {S.Year} ({2205 + S.Year} A.D.) ==="); ev.Amount = S.Year; D.Emit(ev);
            D.Transition(new EnergyBiddingState());
        }

        public override CommandResult Handle(GameCommand cmd) => Fail("year is initialising");
    }
}
