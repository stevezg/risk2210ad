#pragma once
#include <functional>
#include <random>
#include <string>
#include <vector>

#include "Commands.h"
#include "GameEvents.h"
#include "GameState.h"
#include "MapGraph.h"
#include "Types.h"

namespace risk2210 {

/// Complete Risk 2210 A.D. rules engine for 2-5 players. No I/O, no networking: a room wraps
/// one GameDirector per game, feeds it Commands (from humans or bots), and forwards the
/// GameEvent stream + Prompt to whichever client needs to see it.
class GameDirector {
 public:
  /// `names.size()` (2-5) is the real player count; `bots[i]` marks player i as AI-controlled.
  GameDirector(const MapGraph& map, std::vector<std::string> names, std::vector<bool> bots, unsigned seed);

  void Start();  // runs setup, opens the first prompt/turn
  Result Submit(std::unique_ptr<Command> cmd);

  const GameState& State() const { return s_; }
  const std::vector<GameEvent>& Events() const { return events_; }
  const std::vector<std::string>& Log() const { return log_; }

  /// The player id the engine is currently waiting on input from (-1 if none, e.g. game over).
  int ExpectedActor() const;

  void SetEventSink(std::function<void(const GameEvent&)> fn) { sink_ = std::move(fn); }

  // ---- queries (read-only; safe for bots/clients to call against State()) -------------------
  const MapGraph& Map() const { return *s_.map; }
  bool Active(int p) const { return s_.IsActive(p); }
  bool CommanderInPlay(int p, Commander c) const;
  int CountTerritories(int p) const;
  int CountUnits(int p) const;
  int CountStations(int p) const;
  bool ControlsRegion(int p, int region) const;
  int RegionBonus(int p) const;
  int Income(int p) const;
  int Score(int p) const;
  std::vector<int> Owned(int p) const;
  std::vector<int> Owned(int p, TerritoryType type) const;
  bool IsBorder(int p, int t) const;
  bool CanInvade(int p, int from, int to, std::string* why = nullptr) const;
  std::vector<Commander> AttackD8(int from, int to, int nDice) const;
  int DefendD8Count(int to, int nDice) const;
  bool FortifyPathExists(int p, int from, int to) const;
  int CardCost(int p, const CardDef& c) const;
  bool ReactivePlayable(int p, const CardDef& c) const;
  bool HasReactiveCard(int p) const;
  int ConquestWinner() const;  // >=0 once only one real player remains active
  int DeckSize(Commander deck) const { return static_cast<int>(s_.decks[static_cast<int>(deck)].size()); }

 private:
  const TerritoryState& T(int t) const { return s_.territories[t]; }
  TerritoryState& T(int t) { return s_.territories[t]; }
  const PlayerState& P(int p) const { return s_.players[p]; }
  PlayerState& P(int p) { return s_.players[p]; }

  // ---- dice / decks -------------------------------------------------------------------------
  int RollDie(int sides) { return std::uniform_int_distribution<int>(1, sides)(rng_); }
  template <class T> void Shuffle(std::vector<T>& v) { std::shuffle(v.begin(), v.end(), rng_); }
  int DrawTerritoryCard(TerritoryType type, bool skipDevastated = false);
  int DrawCommandCard(Commander deck);

  // ---- mutation helpers (all emit events) ---------------------------------------------------
  void Emit(GameEvent ev);
  void LogText(const std::string& text);
  std::string Name(int p) const;
  std::string TName(int t) const;
  int DestroyUnits(int t, int n);
  void Devastate(int t);
  void CheckElimination(int p);
  void ChangeEnergy(int p, int delta);
  void OpenPrompt(Prompt p);
  Result ResolvePrompt(const RespondPrompt& r);

  // ---- turn structure ------------------------------------------------------------------------
  void RunSetup();
  Result HandleSetupCommand(Command& cmd);
  void BeginPieces();
  int NextWithPool(int start) const;

  void BeginYear();
  Result HandleBidCommand(Command& cmd);
  void RevealBids();
  void FinishTurnOrder();

  void StartTurn(int p);
  void EndTurn();
  Result HandleDeploymentCommand(Command& cmd);
  Result HandleInvasionCommand(Command& cmd);
  Result HandleFortificationCommand(Command& cmd);

  void DeclareInvasionImpl(int p, int from, int to);
  void PromptNextReactor(std::vector<int> reactors, size_t idx);
  void ApplyReactiveCard(int q, int handIndex);
  void OccupyEmpty();
  void ResolveAttack(int nDice, int dDice);
  void MoveInImpl(int p, int k);

  Result PlayCardCommand(int p, int handIndex, CardTiming allowed);
  void ApplyCard(int p, const CardDef& card, std::function<void()> done);
  int ZoneRoll(TerritoryType type);
  void RemoveModsAuto(int p, int n);

  void FinalScoring();
  void CheckpointOrContinue();  // opens a prompt at year >= yearLimit; caller re-enters BeginYear() on continue

  GameState s_;
  std::mt19937 rng_;
  std::vector<GameEvent> events_;
  std::vector<std::string> log_;
  std::function<void(const GameEvent&)> sink_;
};

}  // namespace risk2210
