#pragma once
#include <string>

namespace risk2210 {

enum class TerritoryType { Land, Water, Moon };

// Order matches the client's CMD_INITIAL "LDNXS" and index.html's CMDS array.
enum class Commander { Land = 0, Diplomat = 1, Naval = 2, Nuclear = 3, Space = 4 };
constexpr int kNumCommanders = 5;

enum class Phase { Setup, Bidding, Placement, Recruit, Attack, Fortify, GameOver };

enum class CardTiming { Before, React, End, Score };
enum class CardKind {
  CeaseFire, Influence, Decoys, EnergyCrisis, Evacuation, ModReduction, Redeployment,
  TerrStation, Assemble, Jam, DeathTrap, Reinforce, Scout, Stealth, StealthStation,
  HiddenEnergy, Zone, Assassin, Armageddon, Rocket, Scatter, Extraction, InvadeEarth
};

constexpr int kCommanderCost = 3;
constexpr int kStationCost = 5;
constexpr int kMaxStations = 4;
constexpr int kCardCost = 1;
constexpr int kMaxCardsPerTurn = 4;
constexpr int kStartEnergy = 3;
constexpr int kDevastation = 4;
constexpr int kDefaultYears = 5;
constexpr int kMaxPlayers = 5;

inline const char* ToString(Commander c) {
  switch (c) {
    case Commander::Land: return "Land";
    case Commander::Diplomat: return "Diplomat";
    case Commander::Naval: return "Naval";
    case Commander::Nuclear: return "Nuclear";
    case Commander::Space: return "Space";
  }
  return "?";
}

inline const char* ToString(TerritoryType t) {
  switch (t) {
    case TerritoryType::Land: return "land";
    case TerritoryType::Water: return "water";
    case TerritoryType::Moon: return "moon";
  }
  return "?";
}

inline const char* ToString(Phase p) {
  switch (p) {
    case Phase::Setup: return "Setup";
    case Phase::Bidding: return "Bidding";
    case Phase::Placement: return "Placement";
    case Phase::Recruit: return "Recruit";
    case Phase::Attack: return "Attack";
    case Phase::Fortify: return "Fortify";
    case Phase::GameOver: return "GameOver";
  }
  return "?";
}

/// Outcome of a command: `ok == false` means state is unchanged and `error` explains why.
struct Result {
  bool ok = true;
  std::string error;
  static Result Success() { return {}; }
  static Result Fail(std::string msg) { return {false, std::move(msg)}; }
  explicit operator bool() const { return ok; }
};

}  // namespace risk2210
