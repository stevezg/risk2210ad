using System.Collections.Generic;

namespace Risk2210.Core
{
    public enum GameEventType
    {
        Log, GameStarted, YearStarted, BidsRevealed, TurnOrderSet, TurnStarted, PhaseChanged, PromptOpened, PromptClosed,
        ModsDeployed, CommanderHired, StationBuilt, CardsBought, CardPlayed, InvasionDeclared, InvasionCancelled, Battle,
        TerritoryCaptured, UnitsMoved, UnitsDestroyed, TerritoryDevastated, Fortified, PlayerEliminated, EnergyChanged, GameOver
    }

    /// <summary>
    /// Something that happened in the engine, for the view layer to animate. Events are appended in order
    /// so the animation director can replay them sequentially.
    /// </summary>
    public sealed class GameEvent
    {
        public GameEventType Type;
        public int Player = -1;
        public int OtherPlayer = -1;
        public int From = -1;
        public int To = -1;
        public int Amount;
        public int Amount2;
        public string Text;
        public PhaseId Phase;
        public int[] AttackRolls;
        public int[] DefendRolls;
        public int AttackD8;
        public int DefendD8;
        public bool Captured;

        public static GameEvent Make(GameEventType type, string text = null) => new GameEvent { Type = type, Text = text };
    }
}
