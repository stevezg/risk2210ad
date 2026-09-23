#include "GameDirector.h"

#include <algorithm>
#include <numeric>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>

namespace risk2210 {

namespace {
int ci(Commander c) { return static_cast<int>(c); }
int ti(TerritoryType t) { return static_cast<int>(t); }
}  // namespace

// ============================================================================================
// Construction
// ============================================================================================

GameDirector::GameDirector(const MapGraph& map, std::vector<std::string> names, std::vector<bool> bots, unsigned seed)
    : rng_(seed) {
  int n = static_cast<int>(names.size());
  if (n < 2 || n > kMaxPlayers) throw std::invalid_argument("Risk 2210 supports 2-5 players");
  s_.map = &map;
  s_.numRealPlayers = n;
  for (int p = 0; p < n; p++) {
    PlayerState ps;
    ps.name = names[p];
    ps.isBot = bots[p];
    s_.players.push_back(ps);
  }
  if (n == 2) {
    PlayerState neutral;
    neutral.name = "Neutral";
    neutral.isBot = true;
    neutral.isNeutral = true;
    neutral.energy = 0;
    s_.players.push_back(neutral);
  }
  s_.territories.assign(map.Count(), TerritoryState{});
  for (int c = 0; c < kNumCommanders; c++) {
    s_.decks[c] = CardCatalogue::BuildDeck(static_cast<Commander>(c));
    Shuffle(s_.decks[c]);
  }
  for (TerritoryType t : {TerritoryType::Land, TerritoryType::Water, TerritoryType::Moon}) {
    s_.terrDecks[ti(t)] = map.TerritoriesOfType(t);
    Shuffle(s_.terrDecks[ti(t)]);
  }
}

// ============================================================================================
// Events / logging
// ============================================================================================

void GameDirector::Emit(GameEvent ev) {
  if (!ev.text.empty()) log_.push_back(ev.text);
  events_.push_back(ev);
  if (sink_) sink_(events_.back());
}
void GameDirector::LogText(const std::string& text) {
  GameEvent ev{GameEventType::Log};
  ev.text = text;
  Emit(ev);
}
std::string GameDirector::Name(int p) const { return p >= 0 && p < static_cast<int>(s_.players.size()) ? s_.players[p].name : "nobody"; }
std::string GameDirector::TName(int t) const { return Map()[t].name; }

// ============================================================================================
// Queries
// ============================================================================================

bool GameDirector::CommanderInPlay(int p, Commander c) const {
  for (const auto& t : s_.territories)
    if (t.owner == p && t.HasCommander(c)) return true;
  return false;
}
int GameDirector::CountTerritories(int p) const {
  int n = 0;
  for (const auto& t : s_.territories) n += (t.owner == p);
  return n;
}
int GameDirector::CountUnits(int p) const {
  int n = 0;
  for (const auto& t : s_.territories)
    if (t.owner == p) n += t.Units();
  return n;
}
int GameDirector::CountStations(int p) const {
  int n = 0;
  for (const auto& t : s_.territories) n += (t.owner == p && t.station);
  return n;
}
bool GameDirector::ControlsRegion(int p, int region) const {
  bool any = false;
  for (int t : Map().Regions()[region].territories) {
    if (T(t).devastated) continue;
    if (T(t).owner != p) return false;
    any = true;
  }
  return any;
}
int GameDirector::RegionBonus(int p) const {
  int b = 0;
  for (size_t r = 0; r < Map().Regions().size(); r++)
    if (ControlsRegion(p, static_cast<int>(r))) b += Map().Regions()[r].bonus;
  return b;
}
int GameDirector::Income(int p) const { return std::max(3, CountTerritories(p) / 3) + RegionBonus(p); }
int GameDirector::Score(int p) const { return CountTerritories(p) + RegionBonus(p); }

std::vector<int> GameDirector::Owned(int p) const {
  std::vector<int> out;
  for (int t = 0; t < Map().Count(); t++)
    if (T(t).owner == p) out.push_back(t);
  return out;
}
std::vector<int> GameDirector::Owned(int p, TerritoryType type) const {
  std::vector<int> out;
  for (int t = 0; t < Map().Count(); t++)
    if (T(t).owner == p && Map()[t].type == type) out.push_back(t);
  return out;
}
bool GameDirector::IsBorder(int p, int t) const {
  for (int n : Map()[t].adj)
    if (!T(n).devastated && T(n).owner != p) return true;
  return false;
}

bool GameDirector::CanInvade(int p, int from, int to, std::string* why) const {
  auto fail = [&](const char* msg) { if (why) *why = msg; return false; };
  if (from < 0 || from >= Map().Count() || to < 0 || to >= Map().Count()) return fail("no such territory");
  if (from == to) return fail("cannot invade yourself");
  const auto& f = T(from);
  const auto& d = T(to);
  if (f.owner != p) return fail("you do not control the attacking territory");
  if (d.owner == p) return fail("you already control the target");
  if (f.devastated || d.devastated) return fail("devastated territories are impassable");
  if (f.Units() < 2) return fail("need at least 2 units to invade");
  if (d.owner >= 0 && d.owner < s_.numRealPlayers) {
    const auto& cf = P(p).ceaseFireWith;
    if (std::find(cf.begin(), cf.end(), d.owner) != cf.end()) return fail("a Cease Fire protects that player's territories this turn");
  }
  TerritoryType ft = Map()[from].type, tt = Map()[to].type;
  if ((ft == TerritoryType::Water || tt == TerritoryType::Water) && !CommanderInPlay(p, Commander::Naval))
    return fail("a Naval Commander must be in play to invade into or out of water");
  if ((ft == TerritoryType::Moon || tt == TerritoryType::Moon) && !CommanderInPlay(p, Commander::Space))
    return fail("a Space Commander must be in play to invade into or out of the Moon");
  if (ft == TerritoryType::Moon && tt != TerritoryType::Moon) {
    if (P(p).invadeEarthTarget != to) return fail("Earth can only be invaded from the Moon with the Invade Earth card");
    return true;
  }
  if (ft != TerritoryType::Moon && tt == TerritoryType::Moon) {
    if (!f.station) return fail("lunar invasions must launch from a Space Station");
    if (!Map()[to].landingSite) return fail("from Earth you may only invade a lunar landing site");
    return true;
  }
  if (!Map().AreAdjacent(from, to)) return fail("territories are not adjacent");
  return true;
}

std::vector<Commander> GameDirector::AttackD8(int from, int to, int nDice) const {
  const auto& f = T(from);
  TerritoryType ft = Map()[from].type, tt = Map()[to].type;
  auto qualifies = [&](Commander c) {
    switch (c) {
      case Commander::Nuclear: return true;
      case Commander::Land: return ft == TerritoryType::Land || tt == TerritoryType::Land;
      case Commander::Naval: return ft == TerritoryType::Water || tt == TerritoryType::Water;
      case Commander::Space: return ft == TerritoryType::Moon || tt == TerritoryType::Moon;
      case Commander::Diplomat: return false;
    }
    return false;
  };
  std::vector<Commander> which;
  for (int c = 0; c < kNumCommanders && static_cast<int>(which.size()) < nDice; c++)
    if (f.cmd[c] && qualifies(static_cast<Commander>(c))) which.push_back(static_cast<Commander>(c));
  return which;
}
int GameDirector::DefendD8Count(int to, int nDice) const {
  const auto& d = T(to);
  return d.station ? nDice : std::min(nDice, d.CommanderCount());
}

bool GameDirector::FortifyPathExists(int p, int from, int to) const {
  if (T(from).owner != p || T(to).owner != p) return false;
  std::vector<int> stations, sites;
  for (int t = 0; t < Map().Count(); t++) {
    if (T(t).owner != p) continue;
    if (T(t).station) stations.push_back(t);
    if (Map()[t].landingSite) sites.push_back(t);
  }
  std::vector<bool> seen(Map().Count(), false);
  std::queue<int> q;
  q.push(from);
  seen[from] = true;
  auto visit = [&](int n) {
    if (T(n).owner == p && !T(n).devastated && !seen[n]) { seen[n] = true; q.push(n); }
  };
  while (!q.empty()) {
    int cur = q.front();
    q.pop();
    if (cur == to) return true;
    for (int n : Map()[cur].adj) visit(n);
    if (T(cur).station) for (int s : sites) visit(s);
    if (Map()[cur].landingSite) for (int s : stations) visit(s);
  }
  return false;
}

int GameDirector::CardCost(int p, const CardDef& c) const { return (c.deck == Commander::Nuclear && P(p).armageddonThisTurn) ? 0 : c.cost; }

bool GameDirector::ReactivePlayable(int p, const CardDef& c) const {
  if (c.timing != CardTiming::React || !s_.invasion.active) return false;
  if (!CommanderInPlay(p, c.deck) || P(p).energy < CardCost(p, c) || P(p).jammedThisTurn) return false;
  TerritoryType tt = Map()[s_.invasion.to].type;
  switch (c.kind) {
    case CardKind::Stealth: return c.target == tt && s_.invasion.defender >= 0;
    case CardKind::StealthStation: return tt == TerritoryType::Land && s_.invasion.defender == p && !T(s_.invasion.to).station && CountStations(p) < kMaxStations;
    case CardKind::DeathTrap: return c.target == tt && s_.invasion.defender == p;
    case CardKind::CeaseFire: return s_.invasion.defender == p;
    case CardKind::Evacuation: return s_.invasion.defender == p && static_cast<int>(Owned(p).size()) > 1;
    default: return false;
  }
}
bool GameDirector::HasReactiveCard(int p) const {
  if (P(p).jammedThisTurn) return false;
  for (int id : P(p).hand)
    if (ReactivePlayable(p, CardCatalogue::Get(id))) return true;
  return false;
}

int GameDirector::ConquestWinner() const {
  int active = -1, count = 0;
  for (int p = 0; p < s_.numRealPlayers; p++)
    if (Active(p)) { active = p; count++; }
  return count == 1 ? active : -1;
}

// ============================================================================================
// Decks
// ============================================================================================

int GameDirector::DrawTerritoryCard(TerritoryType type, bool skipDevastated) {
  auto& deck = s_.terrDecks[ti(type)];
  auto& pos = s_.terrDeckPos[ti(type)];
  for (int guard = 0; guard < 200; guard++) {
    if (pos >= deck.size()) { Shuffle(deck); pos = 0; }
    int t = deck[pos++];
    if (!skipDevastated || !T(t).devastated) return t;
  }
  return deck[0];
}
int GameDirector::DrawCommandCard(Commander deck) {
  auto& d = s_.decks[ci(deck)];
  if (d.empty()) return -1;
  int id = d.back();
  d.pop_back();
  return id;
}

// ============================================================================================
// Board mutation
// ============================================================================================

int GameDirector::DestroyUnits(int t, int n) {
  auto& ts = T(t);
  int removed = std::min(n, ts.mods);
  ts.mods -= removed;
  for (int c = 0; c < kNumCommanders && removed < n; c++)
    if (ts.cmd[c]) { ts.cmd[c] = false; removed++; }
  if (removed > 0) {
    GameEvent ev{GameEventType::UnitsDestroyed};
    ev.to = t; ev.amount = removed; ev.player = ts.owner;
    Emit(ev);
  }
  if (ts.Units() == 0 && !ts.station) ts.owner = -1;
  return removed;
}
void GameDirector::Devastate(int t) {
  T(t) = TerritoryState{};
  T(t).devastated = true;
  GameEvent ev{GameEventType::TerritoryDevastated, -1, -1, -1, t};
  ev.text = TName(t) + " is devastated";
  Emit(ev);
}
void GameDirector::CheckElimination(int p) {
  if (!Active(p) || CountUnits(p) > 0) return;
  P(p).eliminated = true;
  P(p).hand.clear();
  for (auto& t : s_.territories)
    if (t.owner == p) { t.station = false; t.owner = -1; }
  GameEvent ev{GameEventType::PlayerEliminated};
  ev.player = p; ev.text = Name(p) + " has been eliminated!";
  Emit(ev);
}
void GameDirector::ChangeEnergy(int p, int delta) {
  P(p).energy += delta;
  GameEvent ev{GameEventType::EnergyChanged};
  ev.player = p; ev.amount = delta;
  Emit(ev);
}
void GameDirector::OpenPrompt(Prompt p) {
  s_.prompt = std::move(p);
  s_.promptOpen = true;
  GameEvent ev{GameEventType::PromptOpened};
  ev.player = s_.prompt.player; ev.text = s_.prompt.text;
  Emit(ev);
}

// ============================================================================================
// Public entry points
// ============================================================================================

int GameDirector::ExpectedActor() const {
  if (s_.promptOpen) return s_.prompt.player;
  switch (s_.phase) {
    case Phase::Setup: return s_.setupActor;
    case Phase::Bidding:
      if (s_.bidChooser >= 0) return s_.bidChooser;
      for (int p = 0; p < s_.numRealPlayers; p++)
        if (Active(p) && P(p).bid < 0) return p;
      return -1;
    case Phase::Recruit:
    case Phase::Attack:
    case Phase::Fortify:
      return s_.current;
    default:
      return -1;
  }
}

void GameDirector::Start() { RunSetup(); }

Result GameDirector::Submit(std::unique_ptr<Command> cmd) {
  if (s_.phase == Phase::GameOver) return Result::Fail("the game is over");
  if (s_.promptOpen) {
    auto* r = dynamic_cast<RespondPrompt*>(cmd.get());
    if (!r) return Result::Fail("waiting for " + Name(s_.prompt.player) + " to answer a prompt");
    if (cmd->player != s_.prompt.player) return Result::Fail("not your prompt");
    return ResolvePrompt(*r);
  }
  if (dynamic_cast<RespondPrompt*>(cmd.get())) return Result::Fail("no prompt is open");
  int expected = ExpectedActor();
  bool biddingSubmit = s_.phase == Phase::Bidding && dynamic_cast<SubmitBid*>(cmd.get());
  if (expected >= 0 && cmd->player != expected && !biddingSubmit) return Result::Fail("not your turn");

  switch (s_.phase) {
    case Phase::Setup: return HandleSetupCommand(*cmd);
    case Phase::Bidding: return HandleBidCommand(*cmd);
    case Phase::Recruit: return HandleDeploymentCommand(*cmd);
    case Phase::Attack: return HandleInvasionCommand(*cmd);
    case Phase::Fortify: return HandleFortificationCommand(*cmd);
    default: return Result::Fail("not allowed right now");
  }
}

Result GameDirector::ResolvePrompt(const RespondPrompt& r) {
  Prompt p = s_.prompt;  // copy: OpenPrompt() may be called again inside a continuation
  switch (p.kind) {
    case PromptKind::DefenseDice:
      if (r.value < 1 || r.value > p.maxDice) return Result::Fail("defend with 1-" + std::to_string(p.maxDice) + " dice");
      break;
    case PromptKind::ReactiveCard:
      if (r.value >= 0) {
        if (r.value >= static_cast<int>(P(p.player).hand.size())) return Result::Fail("no such card");
        const auto& c = CardCatalogue::Get(P(p.player).hand[r.value]);
        if (!ReactivePlayable(p.player, c)) return Result::Fail(c.name + " cannot be played against this invasion");
      }
      break;
    case PromptKind::BonusDeck:
      if (r.value < 0 || r.value >= static_cast<int>(p.decks.size())) return Result::Fail("choose a listed deck");
      break;
    case PromptKind::ChoosePlayer:
    case PromptKind::ChooseTerritory:
    case PromptKind::ClaimTerritory:
    case PromptKind::PlaceStartingMod:
      if (std::find(p.options.begin(), p.options.end(), r.value) == p.options.end()) return Result::Fail("choose a highlighted option");
      break;
    case PromptKind::PlaceStartingPieces:
      for (int v : {r.value, r.value2, r.value3})
        if (std::find(p.options.begin(), p.options.end(), v) == p.options.end()) return Result::Fail("place pieces on territories you control");
      break;
    case PromptKind::None:
      return Result::Fail("no prompt is open");
  }
  s_.promptOpen = false;
  Emit(GameEvent{GameEventType::PromptClosed});
  if (p.kind == PromptKind::PlaceStartingPieces) { if (p.onPieces) p.onPieces(r.value, r.value2, r.value3); }
  else if (p.onResponse) p.onResponse(r.value);
  return Result::Success();
}

// ============================================================================================
// Setup
// ============================================================================================

void GameDirector::RunSetup() {
  s_.phase = Phase::Setup;
  LogText("=== Setup ===");
  for (int i = 0; i < kDevastation; i++) Devastate(DrawTerritoryCard(TerritoryType::Land, true));

  int n = s_.numRealPlayers;
  int startMods = n == 2 ? 30 : n == 3 ? 35 : n == 4 ? 30 : 25;
  for (int p = 0; p < n; p++) { P(p).pool = startMods; P(p).energy = kStartEnergy; }

  if (s_.HasNeutral()) {
    int neutral = s_.NeutralPlayer();
    auto place = [&](TerritoryType type, int count) {
      int placed = 0, guard = 0;
      while (placed < count && guard++ < 500) {
        int t = DrawTerritoryCard(type);
        if (T(t).devastated || T(t).owner != -1) continue;
        T(t).owner = neutral; T(t).mods = 3; placed++;
      }
    };
    place(TerritoryType::Land, 16); place(TerritoryType::Water, 6); place(TerritoryType::Moon, 6);
    LogText("Neutral armies hold 16 land, 6 water and 6 lunar territories");
  }

  for (int t : Map().TerritoriesOfType(TerritoryType::Land))
    if (!T(t).devastated && T(t).owner == -1) s_.freeClaims.push_back(t);
  s_.setupStage = GameState::SetupStage::Claim;
  s_.setupActor = 0;
}

int GameDirector::NextWithPool(int start) const {
  for (int i = 0; i < s_.numRealPlayers; i++) {
    int p = (start + i) % s_.numRealPlayers;
    if (P(p).pool > 0) return p;
  }
  return -1;
}

void GameDirector::BeginPieces() {
  s_.setupStage = GameState::SetupStage::Pieces;
  s_.piecesPlayer = 0;
  s_.setupActor = 0;
}

Result GameDirector::HandleSetupCommand(Command& cmd) {
  if (auto* c = dynamic_cast<ClaimTerritory*>(&cmd)) {
    if (s_.setupStage != GameState::SetupStage::Claim) return Result::Fail("claiming is over");
    auto& free = s_.freeClaims;
    auto it = std::find(free.begin(), free.end(), c->territory);
    if (it == free.end()) return Result::Fail("that territory is not available");
    T(c->territory).owner = cmd.player; T(c->territory).mods = 1; P(cmd.player).pool--;
    Emit(GameEvent{GameEventType::ModsDeployed, cmd.player, -1, -1, c->territory, 1});
    free.erase(it);
    if (free.empty()) { s_.setupStage = GameState::SetupStage::PlaceMods; s_.setupActor = NextWithPool(0); }
    else s_.setupActor = (cmd.player + 1) % s_.numRealPlayers;
    if (s_.setupStage == GameState::SetupStage::PlaceMods && s_.setupActor < 0) BeginPieces();
    return Result::Success();
  }
  if (auto* c = dynamic_cast<PlaceStartingMod*>(&cmd)) {
    if (s_.setupStage != GameState::SetupStage::PlaceMods) return Result::Fail("not placing MODs now");
    if (T(c->territory).owner != cmd.player) return Result::Fail("you must choose a territory you control");
    if (P(cmd.player).pool <= 0) return Result::Fail("no MODs left to place");
    T(c->territory).mods++; P(cmd.player).pool--;
    Emit(GameEvent{GameEventType::ModsDeployed, cmd.player, -1, -1, c->territory, 1});
    s_.setupActor = NextWithPool(cmd.player + 1);
    if (s_.setupActor < 0) BeginPieces();
    return Result::Success();
  }
  if (auto* c = dynamic_cast<PlaceStartingPieces*>(&cmd)) {
    if (s_.setupStage != GameState::SetupStage::Pieces) return Result::Fail("not placing pieces now");
    for (int t : {c->station, c->landCommander, c->diplomat})
      if (T(t).owner != cmd.player) return Result::Fail("place pieces on territories you control");
    T(c->station).station = true;
    T(c->landCommander).cmd[ci(Commander::Land)] = true;
    T(c->diplomat).cmd[ci(Commander::Diplomat)] = true;
    GameEvent ev{GameEventType::StationBuilt};
    ev.player = cmd.player; ev.to = c->station;
    ev.text = Name(cmd.player) + " starts with a Space Station in " + TName(c->station) + ", Land Commander in " + TName(c->landCommander) + ", Diplomat in " + TName(c->diplomat);
    Emit(ev);
    s_.piecesPlayer++;
    if (s_.piecesPlayer >= s_.numRealPlayers) { s_.setupStage = GameState::SetupStage::Done; s_.setupActor = -1; BeginYear(); }
    else s_.setupActor = s_.piecesPlayer;
    return Result::Success();
  }
  return Result::Fail("not allowed during setup");
}

// ============================================================================================
// Bidding
// ============================================================================================

void GameDirector::BeginYear() {
  if (s_.year >= s_.yearLimit) { CheckpointOrContinue(); return; }
  s_.year++;
  s_.turnOrder.clear(); s_.turnIndex = -1; s_.bidChooser = -1; s_.bidRanking.clear(); s_.bidRankingIndex = 0; s_.availableMarkers.clear();
  for (int p = 0; p < s_.numRealPlayers; p++) P(p).bid = -1;
  GameEvent ev{GameEventType::YearStarted};
  ev.amount = s_.year; ev.text = "=== Year " + std::to_string(s_.year) + " (" + std::to_string(2205 + s_.year) + " A.D.) ===";
  Emit(ev);
  s_.phase = Phase::Bidding;
}

Result GameDirector::HandleBidCommand(Command& cmd) {
  if (auto* c = dynamic_cast<SubmitBid*>(&cmd)) {
    if (s_.bidChooser >= 0) return Result::Fail("bids are already revealed");
    if (!Active(cmd.player)) return Result::Fail("eliminated players do not bid");
    if (P(cmd.player).bid >= 0) return Result::Fail("you already bid");
    if (c->amount < 0 || c->amount > P(cmd.player).energy) return Result::Fail("bid between 0 and your energy");
    P(cmd.player).bid = c->amount;
    bool allIn = true;
    for (int p = 0; p < s_.numRealPlayers; p++) if (Active(p) && P(p).bid < 0) allIn = false;
    if (allIn) RevealBids();
    return Result::Success();
  }
  if (auto* c = dynamic_cast<ChooseTurnOrder*>(&cmd)) {
    if (s_.bidChooser < 0) return Result::Fail("bids are still secret");
    auto it = std::find(s_.availableMarkers.begin(), s_.availableMarkers.end(), c->marker);
    if (it == s_.availableMarkers.end()) return Result::Fail("that marker is taken");
    s_.turnOrder[c->marker] = cmd.player;
    s_.availableMarkers.erase(it);
    LogText(Name(cmd.player) + " bid " + std::to_string(P(cmd.player).bid) + " energy and takes turn marker #" + std::to_string(c->marker + 1));
    s_.bidRankingIndex++;
    if (s_.bidRankingIndex >= static_cast<int>(s_.bidRanking.size())) FinishTurnOrder();
    else s_.bidChooser = s_.bidRanking[s_.bidRankingIndex];
    return Result::Success();
  }
  return Result::Fail("bidding is in progress");
}

void GameDirector::RevealBids() {
  std::vector<int> ranking;
  for (int p = 0; p < s_.numRealPlayers; p++) if (Active(p)) ranking.push_back(p);
  std::vector<int> tie(s_.numRealPlayers);
  for (int p : ranking) tie[p] = RollDie(6);
  std::sort(ranking.begin(), ranking.end(), [&](int a, int b) {
    if (P(a).bid != P(b).bid) return P(a).bid > P(b).bid;
    return tie[a] > tie[b];
  });
  for (int p : ranking) ChangeEnergy(p, -P(p).bid);
  s_.bidRanking = ranking;
  s_.turnOrder.assign(ranking.size(), -1);
  s_.availableMarkers.resize(ranking.size());
  std::iota(s_.availableMarkers.begin(), s_.availableMarkers.end(), 0);
  s_.bidRankingIndex = 0;
  s_.bidChooser = ranking[0];
  std::ostringstream os;
  os << "Bids:";
  for (size_t i = 0; i < ranking.size(); i++) os << (i ? ", " : " ") << Name(ranking[i]) << " " << P(ranking[i]).bid;
  Emit(GameEvent{GameEventType::BidsRevealed, -1, -1, -1, -1, 0, 0, os.str()});
}

void GameDirector::FinishTurnOrder() {
  s_.bidChooser = -1;
  s_.turnIndex = -1;
  std::ostringstream os;
  os << "Turn order:";
  for (size_t i = 0; i < s_.turnOrder.size(); i++) os << (i ? " > " : " ") << Name(s_.turnOrder[i]);
  Emit(GameEvent{GameEventType::TurnOrderSet, -1, -1, -1, -1, 0, 0, os.str()});
  EndTurn();  // advances turnIndex to the first active player and starts Recruit
}

// ============================================================================================
// Turn structure
// ============================================================================================

void GameDirector::StartTurn(int p) {
  auto& ps = P(p);
  s_.current = p;
  ps.cardsBoughtThisTurn = 0; ps.contestedCaptures = 0; ps.bonusClaimed = false; ps.invadedThisTurn = false;
  ps.invasionsThisTurn = 0; ps.invadeEarthTarget = -1; ps.extraFortifies = 0; ps.armageddonThisTurn = false;
  ps.lunarExtractionThisTurn = false; ps.hiddenEnergyTargets.clear(); ps.ceaseFireWith.clear();
  s_.invasion = Invasion{};

  int inc = Income(p);
  ps.pool += inc; ps.energy += inc;
  int stationMods = 0;
  for (auto& t : s_.territories) if (t.owner == p && t.station) { t.mods++; stationMods++; }
  GameEvent ev{GameEventType::TurnStarted};
  ev.player = p; ev.amount = inc;
  std::ostringstream os;
  os << "--- " << ps.name << ": " << CountTerritories(p) << " territories, +" << inc << " MODs, +" << inc << " energy";
  if (stationMods) os << ", +" << stationMods << " at stations";
  ev.text = os.str();
  Emit(ev);
  s_.phase = Phase::Recruit;
}

void GameDirector::EndTurn() {
  s_.current = -1;
  int cw = ConquestWinner();
  if (cw >= 0 && s_.year > 0) { s_.conquest = true; FinalScoring(); return; }
  for (s_.turnIndex++; s_.turnIndex < static_cast<int>(s_.turnOrder.size()); s_.turnIndex++) {
    int p = s_.turnOrder[s_.turnIndex];
    if (Active(p)) { StartTurn(p); return; }
  }
  BeginYear();
}

// ============================================================================================
// Deployment phase (recruit)
// ============================================================================================

Result GameDirector::HandleDeploymentCommand(Command& cmd) {
  int p = cmd.player;
  auto& ps = P(p);
  if (auto* c = dynamic_cast<DeployMods*>(&cmd)) {
    if (T(c->territory).owner != p) return Result::Fail("you do not control that territory");
    if (c->count <= 0 || c->count > ps.pool) return Result::Fail("invalid MOD count");
    T(c->territory).mods += c->count; ps.pool -= c->count;
    Emit(GameEvent{GameEventType::ModsDeployed, p, -1, -1, c->territory, c->count});
    return Result::Success();
  }
  if (auto* c = dynamic_cast<HireCommander*>(&cmd)) {
    if (ps.pool > 0) return Result::Fail("deploy all your MODs first");
    if (CommanderInPlay(p, c->commander)) return Result::Fail("that commander is already in play");
    if (ps.energy < kCommanderCost) return Result::Fail("not enough energy");
    if (T(c->territory).owner != p) return Result::Fail("you do not control that territory");
    ChangeEnergy(p, -kCommanderCost);
    T(c->territory).cmd[ci(c->commander)] = true;
    GameEvent ev{GameEventType::CommanderHired};
    ev.player = p; ev.to = c->territory; ev.amount = ci(c->commander);
    ev.text = ps.name + " hires a " + ToString(c->commander) + " Commander in " + TName(c->territory);
    Emit(ev);
    return Result::Success();
  }
  if (auto* c = dynamic_cast<BuildStation*>(&cmd)) {
    if (ps.pool > 0) return Result::Fail("deploy all your MODs first");
    if (CountStations(p) >= kMaxStations) return Result::Fail("you already control 4 Space Stations");
    if (ps.energy < kStationCost) return Result::Fail("not enough energy");
    if (T(c->territory).owner != p) return Result::Fail("you do not control that territory");
    if (Map()[c->territory].type != TerritoryType::Land) return Result::Fail("Space Stations may only be built on land");
    if (T(c->territory).station) return Result::Fail("that territory already has a Space Station");
    ChangeEnergy(p, -kStationCost);
    T(c->territory).station = true;
    GameEvent ev{GameEventType::StationBuilt};
    ev.player = p; ev.to = c->territory; ev.text = ps.name + " builds a Space Station in " + TName(c->territory);
    Emit(ev);
    return Result::Success();
  }
  if (auto* c = dynamic_cast<BuyCards*>(&cmd)) {
    if (ps.pool > 0) return Result::Fail("deploy all your MODs first");
    if (c->decks.empty()) return Result::Fail("no decks chosen");
    if (ps.cardsBoughtThisTurn + static_cast<int>(c->decks.size()) > kMaxCardsPerTurn) return Result::Fail("you may buy at most 4 cards per turn");
    if (ps.energy < kCardCost * static_cast<int>(c->decks.size())) return Result::Fail("not enough energy");
    std::array<int, kNumCommanders> want{};
    for (Commander d : c->decks) {
      if (!CommanderInPlay(p, d)) return Result::Fail(std::string(ToString(d)) + " Commander is not in play");
      if (++want[ci(d)] > DeckSize(d)) return Result::Fail(std::string(ToString(d)) + " deck is exhausted");
    }
    for (Commander d : c->decks) { ps.hand.push_back(DrawCommandCard(d)); ps.cardsBoughtThisTurn++; }
    ChangeEnergy(p, -kCardCost * static_cast<int>(c->decks.size()));
    GameEvent ev{GameEventType::CardsBought};
    ev.player = p; ev.amount = static_cast<int>(c->decks.size()); ev.text = ps.name + " buys " + std::to_string(c->decks.size()) + " command card(s)";
    Emit(ev);
    return Result::Success();
  }
  if (auto* c = dynamic_cast<PlayCard*>(&cmd)) {
    if (ps.pool > 0) return Result::Fail("deploy all your MODs first");
    return PlayCardCommand(p, c->handIndex, CardTiming::Before);
  }
  if (dynamic_cast<EndDeployment*>(&cmd)) {
    if (ps.pool > 0) return Result::Fail("deploy all your MODs first");
    s_.phase = Phase::Attack;
    return Result::Success();
  }
  return Result::Fail("not allowed during deployment");
}

Result GameDirector::PlayCardCommand(int p, int handIndex, CardTiming allowed) {
  auto& ps = P(p);
  if (handIndex < 0 || handIndex >= static_cast<int>(ps.hand.size())) return Result::Fail("no such card");
  const auto& card = CardCatalogue::Get(ps.hand[handIndex]);
  if (card.timing != allowed) return Result::Fail("that card cannot be played now");
  if (allowed == CardTiming::Before && ps.invadedThisTurn) return Result::Fail("you have already declared an invasion this turn");
  if (!CommanderInPlay(p, card.deck)) return Result::Fail(std::string(ToString(card.deck)) + " Commander is not in play");
  int cost = CardCost(p, card);
  if (ps.energy < cost) return Result::Fail("not enough energy");
  if (cost > 0) ChangeEnergy(p, -cost);
  ps.hand.erase(ps.hand.begin() + handIndex);
  GameEvent ev{GameEventType::CardPlayed};
  ev.player = p; ev.amount = card.id; ev.text = ps.name + " plays " + card.name;
  Emit(ev);
  ApplyCard(p, card, [] {});
  return Result::Success();
}

// ============================================================================================
// Card effects
// ============================================================================================

int GameDirector::ZoneRoll(TerritoryType type) {
  static const int landRegion[] = {0, 1, 2, 3, 4, 5};    // NA,SA,EU,AF,AS,AU are regions 0-5 by construction order
  static const int moonRegion[] = {11, 11, 12, 12, 13, 13};  // CRE,DEL,SAJ are regions 11-13
  int roll = RollDie(6);
  if (type == TerritoryType::Land) return landRegion[roll - 1];
  if (type == TerritoryType::Moon) return moonRegion[roll - 1];
  while (roll == 6) roll = RollDie(6);
  return 5 + roll;  // water colonies are regions 6-10
}

void GameDirector::RemoveModsAuto(int q, int n) {
  int removed = 0;
  while (removed < n) {
    int best = -1;
    for (int t = 0; t < Map().Count(); t++)
      if (T(t).owner == q && T(t).mods > 0 && T(t).Units() > 1 && (best < 0 || T(t).mods > T(best).mods)) best = t;
    if (best < 0) break;
    T(best).mods--;
    removed++;
  }
  if (removed > 0) {
    GameEvent ev{GameEventType::UnitsDestroyed};
    ev.player = q; ev.amount = removed; ev.text = Name(q) + " removes " + std::to_string(removed) + " MOD(s)";
    Emit(ev);
  }
}

void GameDirector::ApplyCard(int p, const CardDef& card, std::function<void()> done) {
  auto& ps = P(p);
  switch (card.kind) {
    case CardKind::Reinforce: {
      // Heap-allocated: this recursive continuation outlives ApplyCard's own stack frame
      // (it fires later, once the prompt is answered), so nothing here may be a `[&]` capture
      // of a local -- everything needed rides along in the shared_ptrs below.
      auto opts = std::make_shared<std::vector<int>>(Owned(p, card.target));
      auto remaining = std::make_shared<int>(std::min<int>(3, static_cast<int>(opts->size())));
      auto step = std::make_shared<std::function<void()>>();
      *step = [=]() {
        if (*remaining == 0 || opts->empty()) { done(); return; }
        Prompt pr; pr.kind = PromptKind::ChooseTerritory; pr.player = p; pr.text = "Place 1 reinforcement MOD"; pr.options = *opts;
        pr.onResponse = [=](int t) {
          T(t).mods++;
          Emit(GameEvent{GameEventType::ModsDeployed, p, -1, -1, t, 1});
          opts->erase(std::find(opts->begin(), opts->end(), t));
          (*remaining)--;
          (*step)();
        };
        OpenPrompt(pr);
      };
      (*step)();
      break;
    }
    case CardKind::Assemble: {
      auto opts = Owned(p, card.target);
      if (opts.empty()) { done(); break; }
      Prompt pr; pr.kind = PromptKind::ChooseTerritory; pr.player = p; pr.text = "Place 3 MODs"; pr.options = opts;
      pr.onResponse = [=](int t) { T(t).mods += 3; Emit(GameEvent{GameEventType::ModsDeployed, p, -1, -1, t, 3}); done(); };
      OpenPrompt(pr);
      break;
    }
    case CardKind::EnergyCrisis: {
      int got = 0;
      for (int q = 0; q < s_.numRealPlayers; q++)
        if (q != p && Active(q) && P(q).energy > 0) { ChangeEnergy(q, -1); got++; }
      ChangeEnergy(p, got);
      LogText(ps.name + " collects " + std::to_string(got) + " energy");
      done();
      break;
    }
    case CardKind::Decoys: {
      std::vector<std::pair<int, int>> commanders;  // (cmd, at)
      for (int t = 0; t < Map().Count(); t++)
        if (T(t).owner == p) for (int c = 0; c < kNumCommanders; c++) if (T(t).cmd[c]) commanders.push_back({c, t});
      auto idx = std::make_shared<size_t>(0);
      auto next = std::make_shared<std::function<void()>>();
      *next = [=]() mutable {
        if (*idx >= commanders.size()) { done(); return; }
        auto [c, at] = commanders[(*idx)++];
        Prompt pr; pr.kind = PromptKind::ChooseTerritory; pr.player = p;
        pr.text = std::string("Move your ") + ToString(static_cast<Commander>(c)) + " Commander to";
        pr.options = Owned(p);
        pr.onResponse = [=](int t) {
          if (t != at && T(at).cmd[c]) {
            T(at).cmd[c] = false; T(t).cmd[c] = true;
            if (T(at).Units() == 0 && !T(at).station) T(at).owner = -1;
            GameEvent ev{GameEventType::UnitsMoved};
            ev.player = p; ev.from = at; ev.to = t; ev.amount = 1;
            ev.text = ps.name + " moves the " + ToString(static_cast<Commander>(c)) + " Commander to " + TName(t);
            Emit(ev);
          }
          (*next)();
        };
        OpenPrompt(pr);
      };
      (*next)();
      break;
    }
    case CardKind::ModReduction: {
      for (int q = 0; q < s_.numRealPlayers; q++) if (q != p && Active(q)) RemoveModsAuto(q, 4);
      RemoveModsAuto(p, 2);
      done();
      break;
    }
    case CardKind::Redeployment:
      ps.extraFortifies++;
      LogText(ps.name + " may make an extra fortify move");
      done();
      break;
    case CardKind::TerrStation: {
      std::vector<int> opts;
      for (int t : Owned(p, TerritoryType::Land)) if (!T(t).station) opts.push_back(t);
      if (opts.empty() || CountStations(p) >= kMaxStations) { done(); break; }
      Prompt pr; pr.kind = PromptKind::ChooseTerritory; pr.player = p; pr.text = "Place a Space Station in"; pr.options = opts;
      pr.onResponse = [=](int t) {
        T(t).station = true;
        GameEvent ev{GameEventType::StationBuilt};
        ev.player = p; ev.to = t; ev.text = ps.name + " deploys a Space Station to " + TName(t);
        Emit(ev);
        done();
      };
      OpenPrompt(pr);
      break;
    }
    case CardKind::Jam: {
      std::vector<int> opts;
      for (int q = 0; q < s_.numRealPlayers; q++) if (q != p && Active(q)) opts.push_back(q);
      if (opts.empty()) { done(); break; }
      Prompt pr; pr.kind = PromptKind::ChoosePlayer; pr.player = p; pr.text = "Jam which player's command cards this turn?"; pr.options = opts;
      pr.onResponse = [=](int q) {
        P(q).jammedThisTurn = true;
        LogText(Name(q) + " cannot play command cards during " + ps.name + "'s turn");
        done();
      };
      OpenPrompt(pr);
      break;
    }
    case CardKind::Scout: {
      int t = DrawTerritoryCard(TerritoryType::Land, true);
      if (T(t).owner == p) { T(t).mods += 5; Emit(GameEvent{GameEventType::ModsDeployed, p, -1, -1, t, 5}); LogText(ps.name + " places 5 scout MODs on " + TName(t)); }
      else { ps.scoutTerritory = t; LogText(ps.name + " has scouts waiting in " + TName(t)); }
      done();
      break;
    }
    case CardKind::HiddenEnergy: {
      int t = DrawTerritoryCard(TerritoryType::Water);
      ps.hiddenEnergyTargets.push_back(t);
      LogText(ps.name + " will collect 4 energy if they hold " + TName(t) + " at the end of the turn");
      done();
      break;
    }
    case CardKind::Zone: {
      int region = ZoneRoll(card.target);
      const auto& r = Map().Regions()[region];
      LogText(card.name + " strikes " + r.name + "!");
      std::set<int> affected;
      for (int t : r.territories) {
        auto& ts = T(t);
        if (ts.devastated || ts.Units() == 0) continue;
        affected.insert(ts.owner);
        DestroyUnits(t, 1);
      }
      for (int q : affected) CheckElimination(q);
      done();
      break;
    }
    case CardKind::Assassin: {
      std::vector<int> opts;
      for (int t = 0; t < Map().Count(); t++)
        if (T(t).owner >= 0 && T(t).owner != p && T(t).CommanderCount() > 0) opts.push_back(t);
      if (opts.empty()) { LogText("No enemy commander to target"); done(); break; }
      Prompt pr; pr.kind = PromptKind::ChooseTerritory; pr.player = p; pr.text = "Target the commander in"; pr.options = opts;
      pr.onResponse = [=](int t) {
        auto& ts = T(t);
        int c = -1;
        for (int pref : {3, 4, 2, 0, 1}) if (ts.cmd[pref]) { c = pref; break; }
        int roll = RollDie(8);
        if (roll >= 3 && c >= 0) {
          int owner = ts.owner;
          ts.cmd[c] = false;
          GameEvent ev{GameEventType::UnitsDestroyed};
          ev.to = t; ev.amount = 1; ev.player = owner;
          ev.text = "Assassin Bomb rolls " + std::to_string(roll) + ": " + Name(owner) + "'s " + ToString(static_cast<Commander>(c)) + " Commander in " + TName(t) + " is destroyed";
          Emit(ev);
          if (ts.Units() == 0 && !ts.station) ts.owner = -1;
          CheckElimination(owner);
        } else {
          LogText("Assassin Bomb rolls " + std::to_string(roll) + ": the commander survives");
        }
        done();
      };
      OpenPrompt(pr);
      break;
    }
    case CardKind::Armageddon:
      ps.armageddonThisTurn = true;
      LogText(ps.name + " launches Armageddon: nuclear cards are free this turn");
      done();
      break;
    case CardKind::Rocket: {
      std::vector<int> opts;
      for (int t = 0; t < Map().Count(); t++)
        if (Map()[t].type == card.target && T(t).owner >= 0 && T(t).owner != p && T(t).Units() > 0) opts.push_back(t);
      if (opts.empty()) { LogText("No target for the rocket strike"); done(); break; }
      Prompt pr; pr.kind = PromptKind::ChooseTerritory; pr.player = p; pr.text = "Rocket strike target"; pr.options = opts;
      pr.onResponse = [=](int t) {
        int roll = RollDie(6);
        int owner = T(t).owner;
        LogText("Rocket strike on " + TName(t) + " rolls " + std::to_string(roll));
        DestroyUnits(t, roll);
        CheckElimination(owner);
        done();
      };
      OpenPrompt(pr);
      break;
    }
    case CardKind::Scatter: {
      for (int i = 0; i < std::max(1, card.amount); i++) {
        int t = DrawTerritoryCard(card.target);
        auto& ts = T(t);
        if (ts.owner < 0 || ts.owner == p || ts.devastated) continue;
        int owner = ts.owner;
        int n = (ts.Units() + 1) / 2;
        LogText("Scatter bomb hits " + TName(t) + ": " + std::to_string(n) + " unit(s) destroyed");
        DestroyUnits(t, n);
        CheckElimination(owner);
      }
      done();
      break;
    }
    case CardKind::InvadeEarth: {
      int t = -1;
      for (int guard = 0; guard < 60; guard++) {
        int c = DrawTerritoryCard(TerritoryType::Land, true);
        if (T(c).owner != p) { t = c; break; }
      }
      if (t >= 0) { ps.invadeEarthTarget = t; LogText(ps.name + " may invade " + TName(t) + " from the Moon this turn"); }
      done();
      break;
    }
    case CardKind::Extraction:
      ps.lunarExtractionThisTurn = true;
      done();
      break;
    default:
      done();
      break;
  }
}

// ============================================================================================
// Invasion phase
// ============================================================================================

Result GameDirector::HandleInvasionCommand(Command& cmd) {
  int p = cmd.player;
  auto& inv = s_.invasion;
  if (auto* c = dynamic_cast<PlayCard*>(&cmd)) return PlayCardCommand(p, c->handIndex, CardTiming::Before);
  if (auto* c = dynamic_cast<DeclareInvasion*>(&cmd)) {
    if (inv.active) return Result::Fail("an invasion is already in progress");
    std::string why;
    if (!CanInvade(p, c->from, c->to, &why)) return Result::Fail(why);
    DeclareInvasionImpl(p, c->from, c->to);
    return Result::Success();
  }
  if (auto* c = dynamic_cast<Attack*>(&cmd)) {
    if (!inv.active || inv.attacker != p) return Result::Fail("no invasion in progress");
    if (inv.captured) return Result::Fail("territory already captured; move units in");
    int maxA = std::min(3, T(inv.from).Units() - 1);
    if (maxA < 1) return Result::Fail("not enough units to attack");
    if (c->dice < 1 || c->dice > maxA) return Result::Fail("you may roll between 1 and " + std::to_string(maxA) + " dice");
    int maxD = std::min(2, T(inv.to).Units());
    if (maxD == 2 && Active(inv.defender) && !P(inv.defender).isBot) {
      Prompt pr; pr.kind = PromptKind::DefenseDice; pr.player = inv.defender; pr.maxDice = maxD;
      pr.text = Name(p) + " attacks " + TName(inv.to) + " with " + std::to_string(c->dice) + " dice - defend with how many?";
      int nDice = c->dice;
      pr.onResponse = [=](int dd) { ResolveAttack(nDice, dd); };
      OpenPrompt(pr);
      return Result::Success();
    }
    ResolveAttack(c->dice, maxD);
    return Result::Success();
  }
  if (auto* c = dynamic_cast<MoveIn*>(&cmd)) {
    if (!inv.active || inv.attacker != p) return Result::Fail("no invasion in progress");
    if (!inv.captured) return Result::Fail("territory not captured yet");
    int maxMove = T(inv.from).Units() - 1;
    int minMove = std::max(1, std::min(inv.lastDice, maxMove));
    if (c->count < minMove) return Result::Fail("you must move in at least " + std::to_string(minMove) + " unit(s)");
    if (c->count > maxMove) return Result::Fail("you must leave at least one unit behind");
    MoveInImpl(p, c->count);
    return Result::Success();
  }
  if (dynamic_cast<EndInvasion*>(&cmd)) {
    if (!inv.active || inv.attacker != p) return Result::Fail("no invasion in progress");
    if (inv.captured) return Result::Fail("you must move units into the captured territory first");
    if (!inv.attackedOnce) return Result::Fail("you must attack at least once before calling off an invasion");
    inv = Invasion{};
    Emit(GameEvent{GameEventType::InvasionCancelled});
    return Result::Success();
  }
  if (dynamic_cast<EndInvasionPhase*>(&cmd)) {
    if (inv.active && inv.captured) return Result::Fail("move units into the captured territory first");
    if (inv.active && !inv.attackedOnce) return Result::Fail("you must attack at least once before calling off an invasion");
    inv = Invasion{};
    s_.phase = Phase::Fortify;
    return Result::Success();
  }
  return Result::Fail("not allowed during the invasion phase");
}

void GameDirector::DeclareInvasionImpl(int p, int from, int to) {
  Invasion inv;
  inv.active = true; inv.attacker = p; inv.from = from; inv.to = to;
  inv.defender = T(to).owner; inv.contested = T(to).Units() > 0;
  s_.invasion = inv;
  P(p).invadedThisTurn = true; P(p).invasionsThisTurn++;
  GameEvent ev{GameEventType::InvasionDeclared};
  ev.player = p; ev.from = from; ev.to = to; ev.text = Name(p) + " invades " + TName(to) + " from " + TName(from);
  Emit(ev);

  if (!s_.invasion.contested) { OccupyEmpty(); return; }

  // reactive cards: the actual defender first, then a bystander may help -- but only when the
  // defender is a real player (nobody gets asked to "help" the neutral army).
  std::vector<int> reactors;
  if (Active(s_.invasion.defender) && HasReactiveCard(s_.invasion.defender)) reactors.push_back(s_.invasion.defender);
  if (Active(s_.invasion.defender)) {
    for (int q = 0; q < s_.numRealPlayers; q++)
      if (q != s_.invasion.defender && q != p && Active(q) && HasReactiveCard(q)) reactors.push_back(q);
  }
  PromptNextReactor(reactors, 0);
}

void GameDirector::PromptNextReactor(std::vector<int> reactors, size_t idx) {
  if (!s_.invasion.active || idx >= reactors.size()) {
    if (s_.invasion.active && T(s_.invasion.to).Units() == 0) OccupyEmpty();
    return;
  }
  int q = reactors[idx];
  if (!HasReactiveCard(q)) { PromptNextReactor(reactors, idx + 1); return; }
  Prompt pr; pr.kind = PromptKind::ReactiveCard; pr.player = q;
  pr.text = Name(s_.invasion.attacker) + " invades " + TName(s_.invasion.to) + " from " + TName(s_.invasion.from) + " - play a reactive card?";
  pr.onResponse = [=](int idx2) {
    if (idx2 < 0) { PromptNextReactor(reactors, idx + 1); return; }
    ApplyReactiveCard(q, idx2);
    if (!s_.invasion.active) return;  // Cease Fire / Death Trap may have ended it
    PromptNextReactor(reactors, idx);  // same player may play another
  };
  OpenPrompt(pr);
}

void GameDirector::ApplyReactiveCard(int q, int handIndex) {
  auto& ps = P(q);
  const auto& card = CardCatalogue::Get(ps.hand[handIndex]);
  int cost = CardCost(q, card);
  if (cost > 0) ChangeEnergy(q, -cost);
  ps.hand.erase(ps.hand.begin() + handIndex);
  GameEvent ev{GameEventType::CardPlayed};
  ev.player = q; ev.amount = card.id; ev.text = ps.name + " plays " + card.name;
  Emit(ev);
  auto& inv = s_.invasion;
  switch (card.kind) {
    case CardKind::Stealth:
      T(inv.to).mods += 3;
      Emit(GameEvent{GameEventType::ModsDeployed, T(inv.to).owner, -1, -1, inv.to, 3});
      break;
    case CardKind::StealthStation: {
      T(inv.to).station = true;
      GameEvent st{GameEventType::StationBuilt};
      st.player = q; st.to = inv.to; st.text = "A Space Station appears in " + TName(inv.to);
      Emit(st);
      break;
    }
    case CardKind::DeathTrap: {
      int n = (T(inv.from).Units() + 1) / 2;
      LogText(Name(inv.attacker) + " loses " + std::to_string(n) + " unit(s) in " + TName(inv.from) + " to the trap");
      DestroyUnits(inv.from, n);
      CheckElimination(inv.attacker);
      if (T(inv.from).Units() < 2 || !Active(inv.attacker)) {
        inv = Invasion{};
        Emit(GameEvent{GameEventType::InvasionCancelled, -1, -1, -1, -1, 0, 0, "The invasion collapses"});
      }
      break;
    }
    case CardKind::CeaseFire: {
      P(inv.attacker).ceaseFireWith.push_back(q);
      int defenderName_p = q;
      std::string txt = "Cease Fire: " + Name(inv.attacker) + " may not attack " + Name(defenderName_p) + " again this turn";
      inv = Invasion{};
      Emit(GameEvent{GameEventType::InvasionCancelled, -1, -1, -1, -1, 0, 0, txt});
      break;
    }
    case CardKind::Evacuation: {
      std::vector<int> dests = Owned(q);
      dests.erase(std::find(dests.begin(), dests.end(), inv.to));
      if (dests.empty()) break;
      Prompt pr; pr.kind = PromptKind::ChooseTerritory; pr.player = q; pr.text = "Evacuate all units to"; pr.options = dests;
      int to = inv.to;
      pr.onResponse = [=](int destT) {
        auto& f = T(to); auto& d = T(destT);
        int moved = f.Units();
        d.mods += f.mods; f.mods = 0;
        for (int c = 0; c < kNumCommanders; c++) if (f.cmd[c]) { f.cmd[c] = false; d.cmd[c] = true; }
        if (!f.station) f.owner = -1;
        GameEvent mv{GameEventType::UnitsMoved};
        mv.player = q; mv.from = to; mv.to = destT; mv.amount = moved;
        mv.text = Name(q) + " evacuates " + std::to_string(moved) + " unit(s) from " + TName(to) + " to " + TName(destT);
        Emit(mv);
        if (s_.invasion.active && s_.invasion.to == to && T(to).Units() == 0) OccupyEmpty();
      };
      OpenPrompt(pr);
      break;
    }
    default:
      break;
  }
}

void GameDirector::OccupyEmpty() {
  auto& inv = s_.invasion;
  auto& d = T(inv.to);
  int old = d.owner;
  d.owner = inv.attacker;
  if (old >= 0 && old != inv.attacker && d.station && CountStations(inv.attacker) > kMaxStations) d.station = false;
  inv.captured = true; inv.lastDice = 1; inv.attackedOnce = true;
  GameEvent ev{GameEventType::TerritoryCaptured};
  ev.player = inv.attacker; ev.otherPlayer = old; ev.to = inv.to; ev.from = inv.from;
  ev.text = Name(inv.attacker) + " occupies " + TName(inv.to);
  Emit(ev);
}

void GameDirector::ResolveAttack(int nDice, int dDice) {
  auto& inv = s_.invasion;
  int p = inv.attacker;
  auto& f = T(inv.from);
  auto& d = T(inv.to);
  auto which = AttackD8(inv.from, inv.to, nDice);
  int a8 = static_cast<int>(which.size());
  int d8 = DefendD8Count(inv.to, dDice);

  std::vector<int> ar(nDice), dr(dDice);
  for (int i = 0; i < nDice; i++) ar[i] = RollDie(i < a8 ? 8 : 6);
  for (int i = 0; i < dDice; i++) dr[i] = RollDie(i < d8 ? 8 : 6);
  std::sort(ar.rbegin(), ar.rend());
  std::sort(dr.rbegin(), dr.rend());
  int aLost = 0, dLost = 0;
  for (int i = 0; i < std::min(nDice, dDice); i++) { if (ar[i] > dr[i]) dLost++; else aLost++; }

  int defender = inv.defender;
  inv.attackedOnce = true; inv.lastDice = nDice;

  std::ostringstream os;
  os << "  " << Name(p) << " rolls";
  for (int r : ar) os << " " << r;
  if (a8) os << " (" << a8 << "xd8)";
  os << " vs";
  for (int r : dr) os << " " << r;
  if (d8) os << " (" << d8 << "xd8)";
  os << " -> attacker -" << aLost << ", defender -" << dLost;
  GameEvent bev{GameEventType::Battle};
  bev.player = p; bev.otherPlayer = defender; bev.from = inv.from; bev.to = inv.to;
  bev.attackRolls = ar; bev.defendRolls = dr; bev.attackD8 = a8; bev.defendD8 = d8;
  bev.amount = aLost; bev.amount2 = dLost; bev.text = os.str();
  bool captured = d.Units() - dLost <= 0;
  bev.captured = captured;
  Emit(bev);

  auto quiet = [&](int t, int k) {
    auto& ts = T(t);
    int m = std::min(k, ts.mods);
    ts.mods -= m;
    int removed = m;
    for (int c = 0; c < kNumCommanders && removed < k; c++) if (ts.cmd[c]) { ts.cmd[c] = false; removed++; }
  };
  quiet(inv.from, aLost);
  quiet(inv.to, dLost);

  if (captured) {
    inv.captured = true;
    for (int c = 0; c < kNumCommanders; c++) inv.mustMoveIn[c] = false;
    for (Commander c : which) if (f.cmd[ci(c)]) inv.mustMoveIn[ci(c)] = true;
    if (d.station && CountStations(p) >= kMaxStations) { d.station = false; LogText("  the Space Station in " + TName(inv.to) + " is destroyed"); }
    d.owner = p;
    GameEvent cap{GameEventType::TerritoryCaptured};
    cap.player = p; cap.otherPlayer = defender; cap.to = inv.to; cap.from = inv.from; cap.text = "  " + Name(p) + " captures " + TName(inv.to);
    Emit(cap);

    auto& ps = P(p);
    if (inv.contested) ps.contestedCaptures++;
    if (ps.contestedCaptures >= 3 && !ps.bonusClaimed) {
      ps.bonusClaimed = true;
      ChangeEnergy(p, 1);
      std::vector<Commander> opts;
      for (int c = 0; c < kNumCommanders; c++)
        if (CommanderInPlay(p, static_cast<Commander>(c)) && DeckSize(static_cast<Commander>(c)) > 0) opts.push_back(static_cast<Commander>(c));
      LogText("  3-territory bonus: " + ps.name + " gains 1 energy and a command card");
      if (!opts.empty()) {
        Prompt pr; pr.kind = PromptKind::BonusDeck; pr.player = p; pr.decks = opts; pr.text = "3-territory bonus: draw a card from which deck?";
        pr.onResponse = [=](int i) {
          Commander c = (i >= 0 && i < static_cast<int>(opts.size())) ? opts[i] : opts[0];
          P(p).hand.push_back(DrawCommandCard(c));
        };
        OpenPrompt(pr);
      }
    }
    CheckElimination(defender);
  }
}

void GameDirector::MoveInImpl(int p, int k) {
  auto& inv = s_.invasion;
  auto& f = T(inv.from);
  auto& d = T(inv.to);
  int maxMove = f.Units() - 1;
  k = std::max(1, std::min(k, maxMove));
  int moved = 0;
  for (int c = 0; c < kNumCommanders; c++) if (inv.mustMoveIn[c] && f.cmd[c]) { f.cmd[c] = false; d.cmd[c] = true; moved++; }
  // commanders follow the invading force by preference; MODs are what stays behind
  for (int c = 0; c < kNumCommanders && moved < k; c++) if (f.cmd[c]) { f.cmd[c] = false; d.cmd[c] = true; moved++; }
  int m = std::min(k - moved, f.mods);
  f.mods -= m; d.mods += m; moved += m;
  d.owner = p;
  GameEvent ev{GameEventType::UnitsMoved};
  ev.player = p; ev.from = inv.from; ev.to = inv.to; ev.amount = moved; ev.text = "  " + Name(p) + " moves " + std::to_string(moved) + " unit(s) into " + TName(inv.to);
  Emit(ev);
  auto& ps = P(p);
  if (ps.scoutTerritory == inv.to) { d.mods += 5; ps.scoutTerritory = -1; Emit(GameEvent{GameEventType::ModsDeployed, p, -1, -1, inv.to, 5}); LogText("  scout forces arrive: +5 MODs"); }
  inv = Invasion{};
}

// ============================================================================================
// Fortification phase
// ============================================================================================

Result GameDirector::HandleFortificationCommand(Command& cmd) {
  int p = cmd.player;
  if (auto* c = dynamic_cast<PlayCard*>(&cmd)) return PlayCardCommand(p, c->handIndex, CardTiming::End);
  if (auto* c = dynamic_cast<Fortify*>(&cmd)) {
    auto& f = T(c->from);
    auto& d = T(c->to);
    if (f.owner != p || d.owner != p) return Result::Fail("you must control both territories");
    int cmdMoves = 0;
    for (int i = 0; i < kNumCommanders; i++) if (c->commanders[i]) { if (!f.cmd[i]) return Result::Fail("that commander is not in the source territory"); cmdMoves++; }
    if (c->mods < 0 || c->mods > f.mods) return Result::Fail("invalid MOD count");
    if (c->mods + cmdMoves <= 0) return Result::Fail("nothing to move");
    if (c->mods + cmdMoves > f.Units() - 1) return Result::Fail("you must leave at least one unit behind");
    if (!FortifyPathExists(p, c->from, c->to)) return Result::Fail("no path of friendly territories connects them");
    f.mods -= c->mods; d.mods += c->mods;
    for (int i = 0; i < kNumCommanders; i++) if (c->commanders[i]) { f.cmd[i] = false; d.cmd[i] = true; }
    GameEvent ev{GameEventType::Fortified};
    ev.player = p; ev.from = c->from; ev.to = c->to; ev.amount = c->mods + cmdMoves;
    ev.text = Name(p) + " fortifies " + std::to_string(c->mods + cmdMoves) + " unit(s) from " + TName(c->from) + " to " + TName(c->to);
    Emit(ev);
    if (P(p).extraFortifies > 0) { P(p).extraFortifies--; LogText(Name(p) + " may fortify again"); return Result::Success(); }
    EndTurn();
    return Result::Success();
  }
  if (dynamic_cast<EndFortification*>(&cmd)) { EndTurn(); return Result::Success(); }
  return Result::Fail("not allowed during fortification");
}

// ============================================================================================
// Scoring / conquest / continue
// ============================================================================================

void GameDirector::CheckpointOrContinue() {
  // A real client asks "stop, or play N more years?" here; a headless/bot-only run just stops.
  FinalScoring();
}

void GameDirector::FinalScoring() {
  s_.phase = Phase::GameOver;
  LogText(s_.conquest ? "=== Total conquest ===" : "=== Final scoring ===");
  std::vector<int> res;
  for (int p = 0; p < s_.numRealPlayers; p++) {
    auto& ps = P(p);
    int inf = 0;
    if (!ps.eliminated) {
      std::vector<int> keep;
      for (int id : ps.hand) {
        const auto& c = CardCatalogue::Get(id);
        if (c.timing == CardTiming::Score && CommanderInPlay(p, c.deck)) inf += 3;
        else keep.push_back(id);
      }
      ps.hand = keep;
    }
    ps.finalScore = Score(p) + inf;
    LogText(ps.name + ": " + std::to_string(CountTerritories(p)) + " territories + " + std::to_string(RegionBonus(p)) + " bonus + " + std::to_string(inf) + " influence = " + std::to_string(ps.finalScore));
    res.push_back(p);
  }
  std::sort(res.begin(), res.end(), [&](int a, int b) {
    if (P(a).finalScore != P(b).finalScore) return P(a).finalScore > P(b).finalScore;
    if (P(a).energy != P(b).energy) return P(a).energy > P(b).energy;
    return CountUnits(a) > CountUnits(b);
  });
  s_.winner = res[0];
  GameEvent ev{GameEventType::GameOver};
  ev.player = s_.winner;
  ev.text = s_.conquest ? (Name(s_.winner) + " has conquered the world!") : (Name(s_.winner) + " is elected the new world leader!");
  Emit(ev);
}

}  // namespace risk2210
