#pragma once
#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

#include "risk2210/Agent.h"
#include "risk2210/Game.h"

namespace risk2210::gui {

/// What the engine is currently waiting on the human for.
enum class ReqKind {
    None,
    Claim,         // pick one of `options` (free land territories)
    Place,         // pick one of your territories for 1 starting MOD
    InitialSetup,  // pick station / land commander / diplomat territories
    Bid,           // int energy
    TurnOrder,     // pick one of `options` (marker indices)
    Deploy,        // phase: UI drives game.deployMods, then signals done
    Purchase,      // phase: hire/build/buy, then done
    Cards,         // phase: play cards, then done
    Invade,        // phase: declare/attack/moveIn, then done
    Fortify,       // phase: one fortify, then done
    DefenseDice,   // int in 1..maxDice
    Reactive,      // hand index or -1
    BonusDeck,     // pick one of `decks`
    Territory      // pick one of `options` (card effect)
};

struct Request {
    ReqKind kind = ReqKind::None;
    int player = -1;
    std::vector<int> options;
    std::vector<Commander> decks;
    std::string prompt;
    int maxDice = 0;
    int invFrom = -1, invTo = -1, invAttacker = -1;
};

/// State shared between the engine thread and the render thread. The engine
/// thread holds `mutex` for the whole game except while parked in a wait; the
/// render thread locks it per frame, so it only ever sees consistent state.
struct Shared {
    std::mutex mutex;
    std::condition_variable cv;
    std::unique_lock<std::mutex>* engineLock = nullptr;
    Game* game = nullptr;

    Request req;
    bool responded = false;
    int response = -1;
    InitialSetup setupResponse;

    bool quit = false;
    bool gameOver = false;
    bool paused = false;       // pause between AI turns
    int aiDelayMs = 900;       // pacing between AI turns so a spectator can follow
    std::vector<std::string> log;
};

class GuiAgent : public Agent {
public:
    explicit GuiAgent(Shared& sh) : sh_(sh) {}

    int chooseClaim(const Game&, int player, const std::vector<int>& freeTerritories) override {
        Request r;
        r.kind = ReqKind::Claim;
        r.player = player;
        r.options = freeTerritories;
        r.prompt = "Claim a territory";
        return wait(r);
    }
    int chooseInitialPlacement(const Game& g, int player) override {
        Request r;
        r.kind = ReqKind::Place;
        r.player = player;
        r.options = g.ownedTerritories(player);
        r.prompt = "Place 1 MOD (" + std::to_string(g.player(player).pool) + " left)";
        return wait(r);
    }
    InitialSetup chooseInitialSetup(const Game& g, int player) override {
        Request r;
        r.kind = ReqKind::InitialSetup;
        r.player = player;
        r.options = g.ownedTerritories(player);
        r.prompt = "Place your Space Station, Land Commander and Diplomat";
        wait(r);
        return sh_.setupResponse;
    }
    int bid(const Game&, int player) override {
        Request r;
        r.kind = ReqKind::Bid;
        r.player = player;
        r.prompt = "Bid energy for turn order";
        return wait(r);
    }
    int chooseTurnOrder(const Game&, int player, const std::vector<int>& markers) override {
        Request r;
        r.kind = ReqKind::TurnOrder;
        r.player = player;
        r.options = markers;
        r.prompt = "Choose your turn order marker";
        return wait(r);
    }
    void deployPhase(Game&, int player) override { phase(ReqKind::Deploy, player, "Deploy your MODs"); }
    void purchasePhase(Game&, int player) override { phase(ReqKind::Purchase, player, "Hire commanders, build stations, buy cards"); }
    void cardPhase(Game&, int player) override { phase(ReqKind::Cards, player, "Play command cards"); }
    void invadePhase(Game&, int player) override { phase(ReqKind::Invade, player, "Invade: click your territory, then a target"); }
    void fortifyPhase(Game&, int player) override { phase(ReqKind::Fortify, player, "Fortify: click source, then destination"); }

    int chooseDefenseDice(const Game& g, int player, const Invasion& inv, int maxDice) override {
        if (maxDice <= 1) return maxDice;
        Request r;
        r.kind = ReqKind::DefenseDice;
        r.player = player;
        r.maxDice = maxDice;
        r.invFrom = inv.from;
        r.invTo = inv.to;
        r.invAttacker = inv.attacker;
        r.prompt = g.player(inv.attacker).name + " attacks " + g.map().territory(inv.to).name + " - defend with how many dice?";
        return wait(r);
    }
    int reactiveCard(const Game& g, int player, const Invasion& inv) override {
        bool any = false;
        for (int id : g.player(player).hand)
            if (Cards::def(id).timing == CardTiming::OnInvasionDeclared) any = true;
        if (!any) return -1;
        Request r;
        r.kind = ReqKind::Reactive;
        r.player = player;
        r.invFrom = inv.from;
        r.invTo = inv.to;
        r.invAttacker = inv.attacker;
        r.prompt = g.player(inv.attacker).name + " invades " + g.map().territory(inv.to).name + " - play a reactive card?";
        return wait(r);
    }
    Commander chooseBonusDeck(const Game&, int player, const std::vector<Commander>& options) override {
        Request r;
        r.kind = ReqKind::BonusDeck;
        r.player = player;
        r.decks = options;
        r.prompt = "3-territory bonus! Draw a card from which deck?";
        int i = wait(r);
        if (i < 0 || i >= static_cast<int>(options.size())) i = 0;
        return options[i];
    }
    int chooseTerritory(const Game&, int player, const std::vector<int>& options, const std::string& prompt) override {
        Request r;
        r.kind = ReqKind::Territory;
        r.player = player;
        r.options = options;
        r.prompt = prompt;
        return wait(r);
    }

private:
    void phase(ReqKind k, int player, const char* prompt) {
        Request r;
        r.kind = k;
        r.player = player;
        r.prompt = prompt;
        wait(r);
    }
    /// Publishes the request and parks the engine thread (releasing the shared
    /// mutex) until the render thread answers or the window closes.
    int wait(const Request& r) {
        sh_.req = r;
        sh_.responded = false;
        sh_.response = -1;
        sh_.cv.wait(*sh_.engineLock, [&] { return sh_.responded || sh_.quit; });
        sh_.req = Request{};
        if (sh_.quit) throw std::runtime_error("window closed");
        return sh_.response;
    }
    Shared& sh_;
};

}  // namespace risk2210::gui
