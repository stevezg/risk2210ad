// Headless self-play fuzz test: bot-vs-bot-vs-bot games at every player count (2-5), asserting
// invariants after every command. No networking, no AI personalities (Phase 2) -- just enough
// heuristic to drive complete, legal games so the ported ruleset can be validated in isolation.
#include <algorithm>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <sstream>

#include "../engine/Cards.h"
#include "../engine/GameDirector.h"
#include "../engine/MapGraph.h"

using namespace risk2210;

namespace {

int failures = 0;
#define CHECK(cond, msg_expr)                                                                   \
  do {                                                                                           \
    if (!(cond)) {                                                                               \
      std::ostringstream _oss;                                                                   \
      _oss << msg_expr;                                                                          \
      std::cerr << "FAIL: " << _oss.str() << " (" << __FILE__ << ":" << __LINE__ << ")\n";     \
      failures++;                                                                                \
    }                                                                                             \
  } while (0)

/// Extremely simple, legal-but-not-clever decision maker, enough to exercise every phase and
/// every card kind's prompt continuation. Not a real AI (that's Phase 2's Agent interface).
class Bot {
 public:
  explicit Bot(unsigned seed) : rng_(seed) {}

  CommandPtr Decide(GameDirector& g, const GameState& s, int p) {
    if (s.promptOpen) return AnswerPrompt(g, s, p);
    switch (s.phase) {
      case Phase::Setup: return DecideSetup(g, s, p);
      case Phase::Bidding: return DecideBidding(g, s, p);
      case Phase::Recruit: return DecideRecruit(g, s, p);
      case Phase::Attack: return DecideAttack(g, s, p);
      case Phase::Fortify: return DecideFortify(g, s, p);
      default: return nullptr;
    }
  }

 private:
  std::mt19937 rng_;
  template <class T> const T& Pick(const std::vector<T>& v) { return v[std::uniform_int_distribution<size_t>(0, v.size() - 1)(rng_)]; }
  int RandInt(int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(rng_); }

  std::vector<int> BorderOrAll(GameDirector& g, int p) {
    auto owned = g.Owned(p);
    std::vector<int> border;
    for (int t : owned) if (g.IsBorder(p, t)) border.push_back(t);
    return border.empty() ? owned : border;
  }

  template <class Cmd, class... Args> CommandPtr Make(int p, Args&&... args) {
    auto c = std::make_unique<Cmd>();
    c->player = p;
    Fill(*c, std::forward<Args>(args)...);
    return c;
  }
  static void Fill(ClaimTerritory& c, int t) { c.territory = t; }
  static void Fill(PlaceStartingMod& c, int t) { c.territory = t; }
  static void Fill(PlaceStartingPieces& c, int st, int lc, int dp) { c.station = st; c.landCommander = lc; c.diplomat = dp; }
  static void Fill(SubmitBid& c, int a) { c.amount = a; }
  static void Fill(ChooseTurnOrder& c, int m) { c.marker = m; }
  static void Fill(DeployMods& c, int t, int n) { c.territory = t; c.count = n; }
  static void Fill(HireCommander& c, Commander cm, int t) { c.commander = cm; c.territory = t; }
  static void Fill(BuildStation& c, int t) { c.territory = t; }
  static void Fill(BuyCards& c, std::vector<Commander> d) { c.decks = std::move(d); }
  static void Fill(PlayCard& c, int i) { c.handIndex = i; }
  static void Fill(EndDeployment&) {}
  static void Fill(DeclareInvasion& c, int f, int t) { c.from = f; c.to = t; }
  static void Fill(Attack& c, int d) { c.dice = d; }
  static void Fill(MoveIn& c, int n) { c.count = n; }
  static void Fill(EndInvasion&) {}
  static void Fill(EndInvasionPhase&) {}
  static void Fill(Fortify& c, int f, int t, int m, std::array<bool, kNumCommanders> cmds) { c.from = f; c.to = t; c.mods = m; c.commanders = cmds; }
  static void Fill(EndFortification&) {}
  static void Fill(RespondPrompt& c, int v, int v2 = 0, int v3 = 0) { c.value = v; c.value2 = v2; c.value3 = v3; }

  CommandPtr DecideSetup(GameDirector& g, const GameState& s, int p) {
    switch (s.setupStage) {
      case GameState::SetupStage::Claim: {
        std::vector<int> near;
        for (int t : s.freeClaims)
          for (int n : g.Map()[t].adj)
            if (g.Map()[n].type == TerritoryType::Land && s.territories[n].owner == p) { near.push_back(t); break; }
        int t = (!near.empty() && RandInt(0, 3) != 0) ? Pick(near) : Pick(s.freeClaims);
        return Make<ClaimTerritory>(p, t);
      }
      case GameState::SetupStage::PlaceMods:
        return Make<PlaceStartingMod>(p, Pick(BorderOrAll(g, p)));
      case GameState::SetupStage::Pieces: {
        auto owned = g.Owned(p);
        int best = owned[0];
        for (int t : owned) if (s.territories[t].mods > s.territories[best].mods) best = t;
        return Make<PlaceStartingPieces>(p, best, best, Pick(BorderOrAll(g, p)));
      }
      default: return nullptr;
    }
  }

  CommandPtr DecideBidding(GameDirector& g, const GameState& s, int p) {
    if (s.bidChooser == p) return Make<ChooseTurnOrder>(p, s.availableMarkers.front());
    int e = s.players[p].energy;
    int bid = e <= 3 ? 0 : RandInt(0, std::min(3, e / 3));
    return Make<SubmitBid>(p, bid);
  }

  CommandPtr DecideRecruit(GameDirector& g, const GameState& s, int p) {
    const auto& ps = s.players[p];
    if (ps.pool > 0) return Make<DeployMods>(p, Pick(BorderOrAll(g, p)), RandInt(1, ps.pool));
    static const Commander order[] = {Commander::Land, Commander::Naval, Commander::Nuclear, Commander::Space, Commander::Diplomat};
    for (Commander c : order)
      if (!g.CommanderInPlay(p, c) && ps.energy >= kCommanderCost + 1) return Make<HireCommander>(p, c, Pick(BorderOrAll(g, p)));
    if (ps.energy >= kStationCost + 2 && g.CountStations(p) < kMaxStations && RandInt(0, 2) == 0) {
      std::vector<int> spots;
      for (int t : g.Owned(p, TerritoryType::Land)) if (!s.territories[t].station) spots.push_back(t);
      if (!spots.empty()) return Make<BuildStation>(p, Pick(spots));
    }
    if (ps.cardsBoughtThisTurn == 0 && ps.energy > 1 && ps.hand.size() < 6) {
      std::vector<Commander> inPlay;
      for (int c = 0; c < kNumCommanders; c++)
        if (g.CommanderInPlay(p, static_cast<Commander>(c)) && g.DeckSize(static_cast<Commander>(c)) > 0) inPlay.push_back(static_cast<Commander>(c));
      if (!inPlay.empty()) {
        int spend = std::min({kMaxCardsPerTurn, ps.energy - 1, static_cast<int>(6 - ps.hand.size())});
        std::vector<Commander> want;
        std::array<int, kNumCommanders> count{};
        for (int i = 0; i < spend; i++) {
          Commander c = Pick(inPlay);
          if (++count[static_cast<int>(c)] <= g.DeckSize(c)) want.push_back(c);
        }
        if (!want.empty()) return Make<BuyCards>(p, want);
      }
    }
    for (size_t i = 0; i < ps.hand.size(); i++) {
      const auto& c = CardCatalogue::Get(ps.hand[i]);
      if (c.timing != CardTiming::Before || !g.CommanderInPlay(p, c.deck) || ps.energy - g.CardCost(p, c) < 1) continue;
      if ((c.kind == CardKind::Armageddon || c.kind == CardKind::Zone) && RandInt(0, 3) != 0) continue;  // still exercised, just not on every possible turn
      return Make<PlayCard>(p, static_cast<int>(i));
    }
    return Make<EndDeployment>(p);
  }

  CommandPtr DecideAttack(GameDirector& g, const GameState& s, int p) {
    const auto& inv = s.invasion;
    if (inv.active) {
      if (inv.captured) {
        int maxMove = s.territories[inv.from].Units() - 1;
        int minMove = std::max(1, std::min(inv.lastDice, maxMove));
        int keep = g.IsBorder(p, inv.from) ? std::min(maxMove, 2) : 0;
        return Make<MoveIn>(p, std::max(minMove, maxMove - keep));
      }
      int units = s.territories[inv.from].Units();
      if (units < 2 || (units <= s.territories[inv.to].Units() && inv.attackedOnce)) return Make<EndInvasion>(p);
      return Make<Attack>(p, std::min(3, units - 1));
    }
    if (s.players[p].invasionsThisTurn >= 12) return Make<EndInvasionPhase>(p);
    struct Opt { int from, to; int score; };
    std::vector<Opt> opts;
    for (int from : g.Owned(p)) {
      if (s.territories[from].Units() < 2) continue;
      std::vector<int> targets = g.Map()[from].adj;
      if (s.territories[from].station) for (int t = 0; t < g.Map().Count(); t++) if (g.Map()[t].landingSite) targets.push_back(t);
      if (s.players[p].invadeEarthTarget >= 0) targets.push_back(s.players[p].invadeEarthTarget);
      for (int to : targets) {
        if (!g.CanInvade(p, from, to)) continue;
        int du = s.territories[to].Units();
        if (du == 0 || s.territories[from].Units() >= du + 2) opts.push_back({from, to, s.territories[from].Units() - du});
      }
    }
    if (opts.empty()) return Make<EndInvasionPhase>(p);
    std::sort(opts.begin(), opts.end(), [](const Opt& a, const Opt& b) { return a.score > b.score; });
    const auto& o = opts[std::min<size_t>(RandInt(0, 2), opts.size() - 1)];
    return Make<DeclareInvasion>(p, o.from, o.to);
  }

  CommandPtr DecideFortify(GameDirector& g, const GameState& s, int p) {
    int bestFrom = -1;
    for (int t : g.Owned(p))
      if (!g.IsBorder(p, t) && s.territories[t].mods >= 2 && (bestFrom < 0 || s.territories[t].mods > s.territories[bestFrom].mods)) bestFrom = t;
    if (bestFrom < 0) return Make<EndFortification>(p);
    std::vector<int> dests;
    for (int t : BorderOrAll(g, p)) if (t != bestFrom && g.FortifyPathExists(p, bestFrom, t)) dests.push_back(t);
    if (dests.empty()) return Make<EndFortification>(p);
    std::array<bool, kNumCommanders> none{};
    return Make<Fortify>(p, bestFrom, Pick(dests), s.territories[bestFrom].mods - 1, none);
  }

  CommandPtr AnswerPrompt(GameDirector& g, const GameState& s, int p) {
    const auto& pr = s.prompt;
    switch (pr.kind) {
      case PromptKind::DefenseDice: return Make<RespondPrompt>(p, pr.maxDice);
      case PromptKind::BonusDeck: return Make<RespondPrompt>(p, 0);
      case PromptKind::ChoosePlayer: {
        int best = pr.options[0];
        for (int q : pr.options) if (s.players[q].hand.size() > s.players[best].hand.size()) best = q;
        return Make<RespondPrompt>(p, best);
      }
      case PromptKind::ChooseTerritory:
      case PromptKind::ClaimTerritory:
      case PromptKind::PlaceStartingMod: {
        std::vector<int> border;
        for (int t : pr.options) if (s.territories[t].owner == p && g.IsBorder(p, t)) border.push_back(t);
        return Make<RespondPrompt>(p, border.empty() ? pr.options.front() : Pick(border));
      }
      case PromptKind::ReactiveCard: {
        const auto& inv = s.invasion;
        if (inv.defender != p) return Make<RespondPrompt>(p, -1);
        int attackers = s.territories[inv.from].Units(), defenders = s.territories[inv.to].Units();
        for (size_t i = 0; i < s.players[p].hand.size(); i++) {
          const auto& c = CardCatalogue::Get(s.players[p].hand[i]);
          if (!g.ReactivePlayable(p, c)) continue;
          if (c.kind == CardKind::Stealth || c.kind == CardKind::StealthStation) return Make<RespondPrompt>(p, static_cast<int>(i));
          if (c.kind == CardKind::DeathTrap && attackers >= 4) return Make<RespondPrompt>(p, static_cast<int>(i));
          if (c.kind == CardKind::CeaseFire && attackers >= 6) return Make<RespondPrompt>(p, static_cast<int>(i));
          if (c.kind == CardKind::Evacuation && attackers >= defenders * 3 && defenders >= 3) return Make<RespondPrompt>(p, static_cast<int>(i));
        }
        return Make<RespondPrompt>(p, -1);
      }
      default:
        return Make<RespondPrompt>(p, pr.options.empty() ? 0 : pr.options.front());
    }
  }
};

void CheckInvariants(const GameState& s, const std::string& ctx) {
  std::array<int, kMaxPlayers + 1> stations{};
  std::set<std::pair<int, int>> seen;
  for (int t = 0; t < static_cast<int>(s.territories.size()); t++) {
    const auto& ts = s.territories[t];
    CHECK(ts.mods >= 0, ctx << ": negative MODs");
    if (ts.devastated) { CHECK(ts.owner == -1 && ts.Units() == 0 && !ts.station, ctx << ": devastated territory occupied"); continue; }
    if (ts.Units() > 0) CHECK(ts.owner >= 0, ctx << ": units on unowned territory " << t);
    if (ts.owner == -1) CHECK(ts.Units() == 0 && !ts.station, ctx << ": empty territory has pieces");
    if (ts.station) { CHECK(ts.owner >= 0 && ts.owner <= kMaxPlayers, ctx << ": station owner OOB"); stations[std::max(0, ts.owner)]++; }
    for (int c = 0; c < kNumCommanders; c++)
      if (ts.cmd[c]) CHECK(seen.insert({ts.owner, c}).second, ctx << ": duplicate commander for a player");
  }
  for (int p = 0; p < s.numRealPlayers; p++) {
    CHECK(s.players[p].energy >= 0, ctx << ": negative energy");
    CHECK(stations[p] <= kMaxStations, ctx << ": more than 4 stations");
    if (s.players[p].eliminated) {
      int units = 0;
      for (const auto& t : s.territories) if (t.owner == p) units += t.Units();
      CHECK(units == 0, ctx << ": eliminated player has units");
    }
  }
}

}  // namespace

int main() {
  MapGraph map = MapGraph::CreateStandard();
  CHECK(map.TerritoriesOfType(TerritoryType::Land).size() == 42, "42 land territories");
  CHECK(map.TerritoriesOfType(TerritoryType::Water).size() == 13, "13 water territories");
  CHECK(map.TerritoriesOfType(TerritoryType::Moon).size() == 14, "14 lunar territories");
  int landing = 0;
  for (const auto& t : map.Territories()) {
    CHECK(!t.adj.empty(), "territory " << t.name << " has neighbours");
    for (int n : t.adj) CHECK(map.AreAdjacent(n, t.id), "adjacency symmetric for " << t.name);
    if (t.landingSite) landing++;
  }
  CHECK(landing == 3, "three lunar landing sites");
  CHECK(map.AreAdjacent(map.Find("Sung Tzu"), map.Find("Java Cartel")), "Sung Tzu - Java Cartel link");
  CHECK(map.AreAdjacent(map.Find("New Atlantis"), map.Find("Neo Tokyo")), "Pacific wrap link");
  CHECK(map.AreAdjacent(map.Find("Poseidon"), map.Find("Continental Biospheres")), "Poseidon - Continental Biospheres");
  CHECK(map.AreAdjacent(map.Find("Nova Brasilia"), map.Find("Amazon Desert")), "Nova Brasilia - Amazon Desert");

  int totalGames = 0, totalCommands = 0;
  for (int players = 2; players <= 5; players++) {
    for (unsigned seed = 1; seed <= 500; seed++) {
      std::string ctx = "players=" + std::to_string(players) + " seed=" + std::to_string(seed);
      std::vector<std::string> names;
      std::vector<bool> bots;
      for (int p = 0; p < players; p++) { names.push_back("P" + std::to_string(p)); bots.push_back(true); }
      GameDirector g(map, names, bots, seed * 1000 + players);
      std::vector<Bot> brains;
      for (int p = 0; p < players; p++) brains.emplace_back(seed * 7 + p);
      g.Start();
      int steps = 0;
      while (g.State().phase != Phase::GameOver && steps < 40000) {
        int actor = g.ExpectedActor();
        CHECK(actor >= 0, ctx << ": engine waiting on nobody in phase " << ToString(g.State().phase));
        if (actor < 0) break;
        auto cmd = brains[actor].Decide(g, g.State(), actor);
        CHECK(cmd != nullptr, ctx << ": bot produced no command");
        if (!cmd) break;
        Result r = g.Submit(std::move(cmd));
        CHECK(r.ok, ctx << ": command rejected: " << r.error << " (phase " << ToString(g.State().phase) << ")");
        if (!r.ok) break;
        totalCommands++;
        steps++;
        if (steps % 50 == 0) CheckInvariants(g.State(), ctx + " step " + std::to_string(steps));
      }
      CHECK(g.State().phase == Phase::GameOver, ctx << ": game did not finish (" << steps << " steps, phase " << ToString(g.State().phase) << ")");
      CheckInvariants(g.State(), ctx + " final");
      for (int p = 0; p < players; p++)
        CHECK(g.State().players[p].finalScore >= g.CountTerritories(p), ctx << ": final score at least territory count");
      totalGames++;
    }
  }
  std::cout << "self-play: " << totalGames << " games, " << totalCommands << " commands, " << failures << " failures\n";
  return failures ? 1 : 0;
}
