#pragma once
#include <array>
#include <functional>
#include <string>
#include <vector>

#include "Cards.h"
#include "MapGraph.h"
#include "Types.h"

namespace risk2210 {

struct TerritoryState {
  int owner = -1;
  int mods = 0;
  std::array<bool, kNumCommanders> cmd{};
  bool station = false;
  bool devastated = false;

  int CommanderCount() const {
    int n = 0;
    for (bool b : cmd) n += b ? 1 : 0;
    return n;
  }
  int Units() const { return mods + CommanderCount(); }
  bool HasCommander(Commander c) const { return cmd[static_cast<int>(c)]; }
};

struct PlayerState {
  std::string name;
  bool isBot = false;
  bool isNeutral = false;
  bool eliminated = false;
  int energy = kStartEnergy;
  int pool = 0;
  std::vector<int> hand;
  int finalScore = 0;
  int bid = -1;

  // per-turn bookkeeping (reset at the start of each turn)
  int cardsBoughtThisTurn = 0;
  int contestedCaptures = 0;
  bool bonusClaimed = false;
  bool invadedThisTurn = false;
  int invasionsThisTurn = 0;
  int invadeEarthTarget = -1;
  int extraFortifies = 0;
  bool jammedThisTurn = false;
  bool armageddonThisTurn = false;
  bool lunarExtractionThisTurn = false;
  int scoutTerritory = -1;
  std::vector<int> hiddenEnergyTargets;
  std::vector<int> ceaseFireWith;  // player ids this player may not attack again this turn
};

struct Invasion {
  bool active = false;
  int attacker = -1;
  int defender = -1;  // -1 when the target territory was empty
  int from = -1, to = -1;
  bool contested = false;
  bool attackedOnce = false;
  bool captured = false;
  int lastDice = 0;
  std::array<bool, kNumCommanders> mustMoveIn{};
};

enum class PromptKind { None, ClaimTerritory, PlaceStartingMod, PlaceStartingPieces, DefenseDice, ReactiveCard, BonusDeck, ChoosePlayer, ChooseTerritory };

/// A decision the engine is waiting on from a specific player, outside the normal command flow.
/// `onResponse` is the continuation the engine resumes with; it is never serialized/networked.
struct Prompt {
  PromptKind kind = PromptKind::None;
  int player = -1;
  std::string text;
  std::vector<int> options;
  std::vector<Commander> decks;
  int maxDice = 0;
  std::function<void(int)> onResponse;
  std::function<void(int, int, int)> onPieces;  // PlaceStartingPieces: (station, landCommander, diplomat)
};

/// Complete game state for one room. Generalized to 2-5 real players; a 2-player game adds a
/// synthetic "neutral" seat (official 2-player variant) as one extra PlayerState entry.
struct GameState {
  const MapGraph* map = nullptr;
  std::vector<TerritoryState> territories;
  std::vector<PlayerState> players;
  int numRealPlayers = 0;  // excludes the neutral seat, if any
  int year = 0;
  int yearLimit = kDefaultYears;
  Phase phase = Phase::Setup;
  int current = -1;
  std::vector<int> turnOrder;
  int turnIndex = -1;
  Invasion invasion;
  Prompt prompt;
  bool promptOpen = false;
  int winner = -1;
  bool conquest = false;

  std::array<std::vector<int>, kNumCommanders> decks;
  std::array<std::vector<int>, 3> terrDecks;  // Land, Water, Moon
  std::array<size_t, 3> terrDeckPos{};

  // setup-only bookkeeping
  enum class SetupStage { Claim, PlaceMods, Pieces, Done } setupStage = SetupStage::Claim;
  int setupActor = -1;
  std::vector<int> freeClaims;
  int piecesPlayer = 0;
  std::array<int, 3> pieceSlots{-1, -1, -1};

  // bidding-only bookkeeping
  int bidChooser = -1;
  std::vector<int> bidRanking;
  int bidRankingIndex = 0;
  std::vector<int> availableMarkers;

  bool HasNeutral() const { return static_cast<int>(players.size()) > numRealPlayers; }
  int NeutralPlayer() const { return HasNeutral() ? numRealPlayers : -1; }
  bool IsActive(int p) const { return p >= 0 && p < numRealPlayers && !players[p].eliminated; }
  int NumActiveRealPlayers() const {
    int n = 0;
    for (int p = 0; p < numRealPlayers; p++) n += IsActive(p) ? 1 : 0;
    return n;
  }
};

}  // namespace risk2210
