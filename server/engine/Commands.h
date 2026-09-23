#pragma once
#include <array>
#include <memory>
#include <vector>

#include "Types.h"

namespace risk2210 {

/// Everything a client (human or bot) can ask the engine to do. One struct per action;
/// GameDirector::Submit validates against the current phase/prompt before applying anything.
struct Command {
  virtual ~Command() = default;
  int player = -1;
};
using CommandPtr = std::unique_ptr<Command>;

struct ClaimTerritory : Command { int territory; };
struct PlaceStartingMod : Command { int territory; };
struct PlaceStartingPieces : Command { int station, landCommander, diplomat; };

struct SubmitBid : Command { int amount; };
struct ChooseTurnOrder : Command { int marker; };

struct DeployMods : Command { int territory, count; };
struct HireCommander : Command { Commander commander; int territory; };
struct BuildStation : Command { int territory; };
struct BuyCards : Command { std::vector<Commander> decks; };
struct PlayCard : Command { int handIndex; };
struct EndDeployment : Command {};

struct DeclareInvasion : Command { int from, to; };
struct Attack : Command { int dice; };
struct MoveIn : Command { int count; };
struct EndInvasion : Command {};
struct EndInvasionPhase : Command {};

struct Fortify : Command { int from, to, mods; std::array<bool, kNumCommanders> commanders{}; };
struct EndFortification : Command {};

/// Answers whatever Prompt is currently open.
struct RespondPrompt : Command { int value = 0, value2 = 0, value3 = 0; };

}  // namespace risk2210
