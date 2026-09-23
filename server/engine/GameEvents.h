#pragma once
#include <string>
#include <vector>

#include "Types.h"

namespace risk2210 {

enum class GameEventType {
  Log, YearStarted, BidsRevealed, TurnOrderSet, TurnStarted, PhaseChanged, PromptOpened, PromptClosed,
  ModsDeployed, CommanderHired, StationBuilt, CardsBought, CardPlayed, InvasionDeclared, InvasionCancelled,
  Battle, TerritoryCaptured, UnitsMoved, UnitsDestroyed, TerritoryDevastated, Fortified, PlayerEliminated,
  EnergyChanged, GameOver
};

/// Something that happened, in order, for a client's animation/log layer. Every mutation the
/// engine makes is reported through one of these -- a networked server just forwards the stream.
struct GameEvent {
  GameEventType type;
  int player = -1;
  int otherPlayer = -1;
  int from = -1, to = -1;
  int amount = 0, amount2 = 0;
  std::string text;
  std::vector<int> attackRolls, defendRolls;
  int attackD8 = 0, defendD8 = 0;
  bool captured = false;
};

}  // namespace risk2210
