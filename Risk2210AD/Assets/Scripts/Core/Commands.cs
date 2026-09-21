using System.Collections.Generic;

namespace Risk2210.Core
{
    /// <summary>Everything the view or a bot can ask the engine to do. Validated by the current phase state.</summary>
    public abstract class GameCommand
    {
        public int Player;
    }

    // --- setup ---
    public sealed class ClaimTerritory : GameCommand { public int Territory; }
    public sealed class PlaceStartingMod : GameCommand { public int Territory; }
    public sealed class PlaceStartingPieces : GameCommand { public int SpaceStation; public int LandCommander; public int Diplomat; }

    // --- year init / bidding ---
    public sealed class SubmitBid : GameCommand { public int Amount; }
    public sealed class ChooseTurnOrder : GameCommand { public int Marker; }

    // --- deployment (recruit, hire, buy, play) ---
    public sealed class DeployMods : GameCommand { public int Territory; public int Count; }
    public sealed class HireCommander : GameCommand { public CommanderType Commander; public int Territory; }
    public sealed class BuildSpaceStation : GameCommand { public int Territory; }
    public sealed class BuyCards : GameCommand { public List<CommanderType> Decks = new List<CommanderType>(); }
    public sealed class PlayCard : GameCommand { public int HandIndex; }
    public sealed class EndDeployment : GameCommand { }

    // --- invasion ---
    public sealed class DeclareInvasion : GameCommand { public int From; public int To; }
    public sealed class Attack : GameCommand { public int Dice; }
    public sealed class MoveIn : GameCommand { public int Count; }
    public sealed class EndInvasion : GameCommand { }
    public sealed class EndInvasionPhase : GameCommand { }

    // --- fortification ---
    public sealed class Fortify : GameCommand { public int From; public int To; public int Mods; public bool[] Commanders = new bool[MapGraph.NumCommanders]; }
    public sealed class EndFortification : GameCommand { }

    // --- prompt answers ---
    public sealed class RespondPrompt : GameCommand { public int Value; public int Value2; public int Value3; }

    public struct CommandResult
    {
        public bool Ok;
        public string Error;
        public static CommandResult Success => new CommandResult { Ok = true };
        public static CommandResult Fail(string e) => new CommandResult { Ok = false, Error = e };
    }
}
