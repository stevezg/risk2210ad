// Risk 2210 A.D. — raylib GUI. The rules engine runs on a worker thread; the
// human player's decisions are collected here and handed back through GuiAgent.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "raylib.h"
#include "risk2210/Agents.h"
#include "risk2210/Game.h"
#include "GuiAgent.h"

using namespace risk2210;
using namespace risk2210::gui;

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------
constexpr int kWinW = 1400, kWinH = 900;
constexpr int kBoardW = 1000;
constexpr int kMoonTop = 640;
constexpr float kNodeR = 20.0f;

struct NodePos {
    const char* name;
    float x, y;
};

static const NodePos kPositions[] = {
    // North America
    {"Aleutian Empire", 75, 95}, {"Nunavut", 160, 70}, {"Exiled States of America", 310, 55}, {"Alberta", 140, 140},
    {"Canada", 215, 150}, {"Republique du Quebec", 300, 110}, {"Continental Biospheres", 140, 205},
    {"American Republic", 250, 225}, {"Mexitlopoctli", 175, 290},
    // South America
    {"Nuevo Timoto", 235, 355}, {"Andean Nations", 215, 425}, {"Amazon Desert", 290, 415}, {"Argentina", 245, 500},
    // Europe
    {"Iceland GRC", 400, 110}, {"New Avalon", 405, 175}, {"Jotenheim", 475, 90}, {"Warsaw Republic", 495, 165},
    {"Ukrayina", 575, 140}, {"Andorra", 415, 240}, {"Imperial Balkania", 505, 230},
    // Africa
    {"Saharan Empire", 445, 330}, {"Egypt", 515, 300}, {"Ministry of Djibouti", 565, 375}, {"Zaire Military Zone", 500, 425},
    {"Lesotho", 510, 505}, {"Madagascar", 585, 495},
    // Asia
    {"Middle East", 605, 275}, {"Afghanistan", 645, 205}, {"Enclave of the Bear", 645, 135}, {"Siberia", 705, 80},
    {"Sakha", 785, 60}, {"Pevek", 865, 75}, {"Alden", 765, 135}, {"Khan Industrial State", 785, 195}, {"Japan", 885, 195},
    {"Hong Kong", 735, 255}, {"United Indiastan", 665, 305}, {"Angkhor Wat", 745, 325},
    // Australia
    {"Java Cartel", 785, 415}, {"New Guinea", 875, 405}, {"Aboriginal League", 795, 505}, {"Australian Testing Ground", 900, 525},
    // Water
    {"Poseidon", 45, 205}, {"Hawaiian Preserve", 55, 285}, {"New Atlantis", 65, 365},
    {"Western Ireland", 350, 215}, {"New York City", 320, 265}, {"Nova Brasilia", 335, 335},
    {"Neo Paulo", 335, 480}, {"The Ivory Reef", 405, 465},
    {"South Ceylon", 640, 385}, {"Microcorp", 640, 455}, {"Akara", 695, 515},
    {"Neo Tokyo", 940, 275}, {"Sung Tzu", 940, 345},
    // Moon
    {"Harpalus", 90, 700}, {"Bay of Dew", 90, 790}, {"Sea of Rains", 185, 705}, {"Ocean of Storms", 195, 800},
    {"Aristotle", 320, 680}, {"Sea of Serenity", 345, 750}, {"Sea of Crisis", 445, 715}, {"Sea of Nectar", 425, 805},
    {"Rhaeticus", 575, 735}, {"Byrgius", 575, 835}, {"Sea of Clouds", 675, 775}, {"Straight Wall", 755, 710},
    {"Marsh of Diseases", 735, 845}, {"Tycho", 850, 800},
};

static const Color kPlayerColors[] = {
    {220, 50, 47, 255}, {38, 139, 210, 255}, {60, 170, 80, 255}, {230, 190, 40, 255}, {90, 90, 100, 255}, {150, 150, 150, 255}};

static Color regionColor(const Map& m, int region) {
    static const Color c[] = {
        {230, 210, 90, 255},  {190, 120, 70, 255},  {160, 100, 200, 255}, {120, 200, 210, 255}, {110, 200, 110, 255},
        {230, 110, 100, 255}, {80, 170, 255, 255},  {240, 220, 60, 255},  {240, 90, 60, 255},   {120, 210, 90, 255},
        {240, 150, 60, 255},  {200, 200, 210, 255}, {170, 170, 190, 255}, {140, 140, 160, 255}};
    (void)m;
    return c[region % 14];
}

// ---------------------------------------------------------------------------
// UI helpers
// ---------------------------------------------------------------------------
static bool Button(Rectangle r, const char* text, bool enabled = true, Color base = {60, 70, 90, 255}) {
    Vector2 m = GetMousePosition();
    bool hover = enabled && CheckCollisionPointRec(m, r);
    Color fill = enabled ? (hover ? Color{(unsigned char)std::min(255, base.r + 30), (unsigned char)std::min(255, base.g + 30),
                                           (unsigned char)std::min(255, base.b + 30), 255}
                                 : base)
                         : Color{40, 42, 50, 255};
    DrawRectangleRec(r, fill);
    DrawRectangleLinesEx(r, 1, enabled ? Color{140, 160, 190, 255} : Color{70, 70, 80, 255});
    int fs = 10;
    int w = MeasureText(text, fs);
    DrawText(text, static_cast<int>(r.x + (r.width - w) / 2), static_cast<int>(r.y + (r.height - fs) / 2), fs,
             enabled ? RAYWHITE : GRAY);
    return hover && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
}

static void DrawWrapped(const std::string& text, int x, int y, int maxW, int fs, Color col) {
    std::string line, word;
    int cy = y;
    auto flush = [&] {
        DrawText(line.c_str(), x, cy, fs, col);
        cy += fs + 3;
        line.clear();
    };
    for (size_t i = 0; i <= text.size(); ++i) {
        char ch = i < text.size() ? text[i] : ' ';
        if (ch == ' ' || ch == '\n') {
            std::string cand = line.empty() ? word : line + " " + word;
            if (MeasureText(cand.c_str(), fs) > maxW && !line.empty()) {
                flush();
                line = word;
            } else {
                line = cand;
            }
            word.clear();
            if (ch == '\n') flush();
        } else {
            word += ch;
        }
    }
    if (!line.empty()) flush();
}

static const char* commanderInitial(Commander c) {
    switch (c) {
        case Commander::Land: return "L";
        case Commander::Diplomat: return "D";
        case Commander::Naval: return "N";
        case Commander::Nuclear: return "X";
        case Commander::Space: return "S";
    }
    return "?";
}

// ---------------------------------------------------------------------------
// Per-frame UI state (render thread only)
// ---------------------------------------------------------------------------
struct UiState {
    int selected = -1;       // first click (source territory)
    int hireType = -1;       // commander type being placed, or -2 for a space station
    std::vector<Commander> cart;
    int moveIn = 1;
    int fortifyN = 1;
    unsigned fortifyCmds = 0;
    int bidValue = 0;
    int setupStep = 0;
    InitialSetup setup;
    std::string status;
    BattleResult lastBattle;
    bool haveBattle = false;
    ReqKind lastKind = ReqKind::None;
    int lastPlayer = -1;
};

static std::vector<Vector2> g_pos;  // territory id -> screen position

static int hitTerritory(Vector2 m) {
    int best = -1;
    float bestD = kNodeR + 4;
    for (size_t i = 0; i < g_pos.size(); ++i) {
        float d = std::hypot(m.x - g_pos[i].x, m.y - g_pos[i].y);
        if (d < bestD) {
            bestD = d;
            best = static_cast<int>(i);
        }
    }
    return best;
}

static void respond(Shared& sh, int value) {
    sh.response = value;
    sh.responded = true;
    sh.cv.notify_all();
}

// ---------------------------------------------------------------------------
// Board rendering
// ---------------------------------------------------------------------------
static void drawBoard(const Game& g, const Shared& sh, const UiState& ui) {
    const Map& m = g.map();
    DrawRectangle(0, 0, kBoardW, kWinH, Color{14, 22, 45, 255});
    DrawRectangle(0, kMoonTop, kBoardW, kWinH - kMoonTop, Color{22, 22, 30, 255});
    DrawLine(0, kMoonTop, kBoardW, kMoonTop, Color{80, 80, 100, 255});
    DrawText("THE MOON", 12, kMoonTop + 8, 20, Color{150, 150, 170, 255});
    DrawText("landing sites: Sea of Crisis, Bay of Dew, Tycho (from a Space Station)", 130, kMoonTop + 13, 10, GRAY);

    // edges
    for (int t = 0; t < m.size(); ++t) {
        for (int n : m.territory(t).adjacent) {
            if (n < t) continue;
            Vector2 a = g_pos[t], b = g_pos[n];
            TerrType ta = m.territory(t).type, tb = m.territory(n).type;
            Color col = (ta == TerrType::Water || tb == TerrType::Water) ? Color{70, 140, 220, 160} : Color{110, 120, 140, 140};
            if (ta == TerrType::Moon) col = Color{120, 120, 140, 160};
            if (std::fabs(a.x - b.x) > kBoardW * 0.6f) {  // wrap-around link: draw stubs to the board edges
                Vector2 l = a.x < b.x ? a : b, r = a.x < b.x ? b : a;
                DrawLineEx(l, {0, l.y}, 2, col);
                DrawLineEx(r, {kBoardW, r.y}, 2, col);
                const char* leftLabel = (a.x < b.x ? m.territory(n).name : m.territory(t).name).c_str();
                const char* rightLabel = (a.x < b.x ? m.territory(t).name : m.territory(n).name).c_str();
                DrawText(leftLabel, 4, static_cast<int>(l.y) - 34, 10, col);
                DrawText(rightLabel, kBoardW - 6 - MeasureText(rightLabel, 10), static_cast<int>(r.y) - 34, 10, col);
            } else {
                DrawLineEx(a, b, 2, col);
            }
        }
    }
    // Earth<->Moon launch links for the current player's stations
    int cur = g.currentPlayer();
    if (cur >= 0)
        for (int t = 0; t < m.size(); ++t)
            if (g.territory(t).owner == cur && g.territory(t).spaceStation)
                for (int s = 0; s < m.size(); ++s)
                    if (m.territory(s).lunarLandingSite) DrawLineEx(g_pos[t], g_pos[s], 1, Color{200, 200, 255, 40});

    // highlight sets
    const Request& rq = sh.req;
    auto isOption = [&](int t) { return std::find(rq.options.begin(), rq.options.end(), t) != rq.options.end(); };
    bool optionMode = rq.kind == ReqKind::Claim || rq.kind == ReqKind::Place || rq.kind == ReqKind::Territory ||
                      rq.kind == ReqKind::InitialSetup;
    float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 5.0f);

    for (int t = 0; t < m.size(); ++t) {
        const auto& info = m.territory(t);
        const auto& ts = g.territory(t);
        Vector2 p = g_pos[t];
        Color ring = regionColor(m, info.region);
        Color fill = ts.owner >= 0 ? kPlayerColors[ts.owner] : Color{40, 48, 70, 255};
        if (ts.devastated) {
            fill = Color{25, 25, 25, 255};
            ring = Color{90, 40, 40, 255};
        }
        if (info.type == TerrType::Water) DrawCircleV(p, kNodeR + 3, Color{30, 80, 160, 255});
        DrawCircleV(p, kNodeR, fill);
        DrawRing(p, kNodeR - 3, kNodeR, 0, 360, 32, ring);
        if (ts.devastated) {
            DrawLineEx({p.x - 10, p.y - 10}, {p.x + 10, p.y + 10}, 3, Color{200, 60, 60, 255});
            DrawLineEx({p.x - 10, p.y + 10}, {p.x + 10, p.y - 10}, 3, Color{200, 60, 60, 255});
        } else if (ts.owner >= 0) {
            std::string n = std::to_string(ts.units());
            int w = MeasureText(n.c_str(), 20);
            DrawText(n.c_str(), static_cast<int>(p.x) - w / 2, static_cast<int>(p.y) - 10, 20, WHITE);
            std::string cmds;
            for (int c = 0; c < kNumCommanders; ++c)
                if (ts.commanders[c]) cmds += commanderInitial(static_cast<Commander>(c));
            if (!cmds.empty()) DrawText(cmds.c_str(), static_cast<int>(p.x) - 16, static_cast<int>(p.y) - 22, 10, Color{255, 240, 150, 255});
            if (ts.spaceStation) {
                DrawRectangle(static_cast<int>(p.x) + 8, static_cast<int>(p.y) - 22, 10, 10, WHITE);
                DrawRectangleLines(static_cast<int>(p.x) + 8, static_cast<int>(p.y) - 22, 10, 10, BLACK);
            }
        }
        if (info.lunarLandingSite) DrawRing(p, kNodeR + 4, kNodeR + 6, 0, 360, 32, Color{180, 180, 255, 120});

        // interaction highlights
        bool hl = false;
        Color hlc = YELLOW;
        if (optionMode && isOption(t)) hl = true;
        if (ui.selected == t) {
            hl = true;
            hlc = WHITE;
        }
        if (rq.kind == ReqKind::Invade && ui.selected >= 0 && ui.selected != t && g.canInvade(rq.player, ui.selected, t)) {
            hl = true;
            hlc = ORANGE;
        }
        if (rq.kind == ReqKind::Fortify && ui.selected >= 0 && ui.selected != t && g.fortifyPathExists(rq.player, ui.selected, t)) {
            hl = true;
            hlc = SKYBLUE;
        }
        const Invasion& inv = g.invasion();
        if (inv.active && (t == inv.from || t == inv.to)) {
            hl = true;
            hlc = t == inv.to ? RED : ORANGE;
        }
        if ((rq.kind == ReqKind::DefenseDice || rq.kind == ReqKind::Reactive) && (t == rq.invFrom || t == rq.invTo)) {
            hl = true;
            hlc = t == rq.invTo ? RED : ORANGE;
        }
        if (hl) DrawRing(p, kNodeR + 3, kNodeR + 6 + 2 * pulse, 0, 360, 32, hlc);

        // label
        int w = MeasureText(info.name.c_str(), 10);
        DrawText(info.name.c_str(), static_cast<int>(p.x) - w / 2, static_cast<int>(p.y) + kNodeR + 3, 10,
                 ts.devastated ? Color{120, 120, 120, 255} : Color{225, 225, 235, 255});
    }

    // legend of regions + bonuses
    int lx = 12, ly = 560;
    for (size_t r = 0; r < m.regions().size(); ++r) {
        const auto& rg = m.region(static_cast<int>(r));
        if (rg.type == TerrType::Moon) continue;
        int col = static_cast<int>(r) % 6;
        int row = static_cast<int>(r) / 6;
        int x = lx + col * 160, y = ly + row * 16;
        DrawRectangle(x, y + 2, 10, 10, regionColor(m, static_cast<int>(r)));
        DrawText(TextFormat("%s +%d", rg.name.c_str(), rg.bonus), x + 14, y + 2, 10, LIGHTGRAY);
    }
    int mx = 12, my = kWinH - 20;
    for (size_t r = 0; r < m.regions().size(); ++r) {
        const auto& rg = m.region(static_cast<int>(r));
        if (rg.type != TerrType::Moon) continue;
        DrawRectangle(mx, my + 2, 10, 10, regionColor(m, static_cast<int>(r)));
        DrawText(TextFormat("%s +%d", rg.name.c_str(), rg.bonus), mx + 14, my + 2, 10, LIGHTGRAY);
        mx += 130;
    }
}

// ---------------------------------------------------------------------------
// Side panel: status, prompt, controls, hand, log
// ---------------------------------------------------------------------------
static void drawPanel(Game& g, Shared& sh, UiState& ui) {
    const int px = kBoardW, pw = kWinW - kBoardW;
    DrawRectangle(px, 0, pw, kWinH, Color{28, 30, 38, 255});
    DrawLine(px, 0, px, kWinH, Color{80, 80, 100, 255});
    int x = px + 12, y = 10;

    DrawText("RISK 2210 A.D.", x, y, 20, Color{230, 80, 70, 255});
    DrawText(TextFormat("Year %d/%d   %s", g.year(), kNumYears, toString(g.phase())), x + 170, y + 6, 10, LIGHTGRAY);
    y += 30;

    // players
    for (int p = 0; p < g.numPlayers(); ++p) {
        const auto& ps = g.player(p);
        DrawRectangle(x, y + 2, 10, 10, kPlayerColors[p]);
        std::string cmds;
        for (int c = 0; c < kNumCommanders; ++c)
            if (g.commanderInPlay(p, static_cast<Commander>(c))) cmds += commanderInitial(static_cast<Commander>(c));
        std::string line = ps.name + (ps.eliminated ? " (out)" : "") + TextFormat("  E:%d  T:%d  S:%d  cards:%d  st:%d  [%s]",
                                                                                     ps.energy, g.countTerritories(p),
                                                                                     g.score(p), (int)ps.hand.size(),
                                                                                     g.countSpaceStations(p), cmds.c_str());
        DrawText(line.c_str(), x + 14, y + 2, 10, g.currentPlayer() == p ? YELLOW : LIGHTGRAY);
        y += 15;
    }
    y += 6;
    DrawLine(px, y, kWinW, y, Color{70, 70, 90, 255});
    y += 8;

    const Request& rq = sh.req;
    if (rq.kind != ui.lastKind || rq.player != ui.lastPlayer) {  // new request: reset transient state
        ui.selected = -1;
        ui.hireType = -1;
        ui.cart.clear();
        ui.setupStep = 0;
        ui.status.clear();
        ui.lastKind = rq.kind;
        ui.lastPlayer = rq.player;
        ui.fortifyN = 1;
        ui.fortifyCmds = 0;
        if (rq.kind == ReqKind::Bid) ui.bidValue = 0;
    }

    if (sh.gameOver) {
        DrawText("GAME OVER", x, y, 20, GOLD);
        y += 26;
        for (int p = 0; p < g.numPlayers(); ++p) {
            DrawText(TextFormat("%s: %d", g.player(p).name.c_str(), g.player(p).finalScore), x, y, 10, kPlayerColors[p]);
            y += 14;
        }
        y += 6;
    } else if (rq.kind == ReqKind::None) {
        DrawText(g.currentPlayer() >= 0 ? TextFormat("%s (AI) is playing...", g.player(g.currentPlayer()).name.c_str())
                                        : "AI players are acting...",
                 x, y, 10, LIGHTGRAY);
        y += 16;
        if (Button({(float)x, (float)y, 90, 22}, sh.paused ? "Resume AI" : "Pause AI")) sh.paused = !sh.paused;
        if (Button({(float)x + 100, (float)y, 90, 22}, sh.aiDelayMs > 0 ? "Fast forward" : "Normal speed"))
            sh.aiDelayMs = sh.aiDelayMs > 0 ? 0 : 900;
        y += 30;
    } else {
        const auto& me = g.player(rq.player);
        DrawText(TextFormat("%s - your move", me.name.c_str()), x, y, 20, kPlayerColors[rq.player]);
        y += 24;
        DrawWrapped(rq.prompt, x, y, pw - 24, 10, RAYWHITE);
        y += 30;
    }

    auto doneButton = [&](const char* label, bool enabled = true) {
        if (Button({(float)x, (float)y, 120, 26}, label, enabled, {50, 110, 70, 255})) respond(sh, 0);
        y += 32;
    };

    Vector2 mouse = GetMousePosition();
    bool boardClick = IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && mouse.x < kBoardW;
    int clicked = boardClick ? hitTerritory(mouse) : -1;
    auto isOption = [&](int t) { return std::find(rq.options.begin(), rq.options.end(), t) != rq.options.end(); };

    switch (rq.kind) {
        case ReqKind::None:
            break;
        case ReqKind::Claim:
        case ReqKind::Place:
        case ReqKind::Territory:
            DrawText("Click a highlighted territory on the board.", x, y, 10, GRAY);
            y += 16;
            if (clicked >= 0 && isOption(clicked)) respond(sh, clicked);
            break;
        case ReqKind::InitialSetup: {
            const char* steps[] = {"Click a territory for your SPACE STATION", "Click a territory for your LAND COMMANDER",
                                   "Click a territory for your DIPLOMAT"};
            DrawText(steps[std::min(ui.setupStep, 2)], x, y, 10, YELLOW);
            y += 16;
            if (clicked >= 0 && isOption(clicked)) {
                if (ui.setupStep == 0) ui.setup.spaceStation = clicked;
                else if (ui.setupStep == 1) ui.setup.landCommander = clicked;
                else ui.setup.diplomat = clicked;
                if (++ui.setupStep == 3) {
                    sh.setupResponse = ui.setup;
                    respond(sh, 0);
                }
            }
            break;
        }
        case ReqKind::Bid: {
            int maxBid = g.player(rq.player).energy;
            ui.bidValue = std::clamp(ui.bidValue, 0, maxBid);
            DrawText(TextFormat("Bid: %d  (you have %d energy)", ui.bidValue, maxBid), x, y, 10, RAYWHITE);
            y += 16;
            if (Button({(float)x, (float)y, 30, 24}, "-", ui.bidValue > 0)) --ui.bidValue;
            if (Button({(float)x + 36, (float)y, 30, 24}, "+", ui.bidValue < maxBid)) ++ui.bidValue;
            if (Button({(float)x + 80, (float)y, 100, 24}, "Confirm bid", true, {50, 110, 70, 255})) respond(sh, ui.bidValue);
            y += 32;
            break;
        }
        case ReqKind::TurnOrder: {
            for (size_t i = 0; i < rq.options.size(); ++i)
                if (Button({(float)x + i * 50, (float)y, 44, 24}, TextFormat("#%d", rq.options[i] + 1))) respond(sh, rq.options[i]);
            y += 32;
            break;
        }
        case ReqKind::Deploy: {
            int pool = g.player(rq.player).pool;
            DrawText(TextFormat("%d MODs to place. Click your territory (+1), or select and use buttons.", pool), x, y, 10, RAYWHITE);
            y += 16;
            if (clicked >= 0 && g.territory(clicked).owner == rq.player) {
                ui.selected = clicked;
                Result r = g.deployMods(rq.player, clicked, 1);
                ui.status = r.ok ? "" : r.error;
            }
            bool sel = ui.selected >= 0;
            if (Button({(float)x, (float)y, 40, 24}, "+5", sel && pool >= 5)) g.deployMods(rq.player, ui.selected, 5);
            if (Button({(float)x + 46, (float)y, 40, 24}, "All", sel && pool > 0)) g.deployMods(rq.player, ui.selected, pool);
            y += 30;
            doneButton("Done deploying", pool == 0);
            break;
        }
        case ReqKind::Purchase: {
            const auto& me = g.player(rq.player);
            DrawText(TextFormat("Energy: %d   Commanders 3E, Space Station 5E, cards 1E", me.energy), x, y, 10, RAYWHITE);
            y += 16;
            DrawText("Hire (then click a territory):", x, y, 10, GRAY);
            y += 14;
            for (int c = 0; c < kNumCommanders; ++c) {
                bool have = g.commanderInPlay(rq.player, static_cast<Commander>(c));
                Color base = ui.hireType == c ? Color{120, 90, 30, 255} : Color{60, 70, 90, 255};
                if (Button({(float)x + c * 74, (float)y, 70, 24}, toString(static_cast<Commander>(c)), !have && me.energy >= kCommanderCost, base))
                    ui.hireType = c;
            }
            y += 30;
            if (Button({(float)x, (float)y, 130, 24}, "Space Station (5E)",
                       me.energy >= kSpaceStationCost && g.countSpaceStations(rq.player) < kMaxSpaceStations,
                       ui.hireType == -2 ? Color{120, 90, 30, 255} : Color{60, 70, 90, 255}))
                ui.hireType = -2;
            y += 30;
            if (clicked >= 0 && ui.hireType != -1) {
                Result r = ui.hireType == -2 ? g.buildSpaceStation(rq.player, clicked)
                                             : g.hireCommander(rq.player, static_cast<Commander>(ui.hireType), clicked);
                ui.status = r.ok ? "" : r.error;
                if (r.ok) ui.hireType = -1;
            }
            DrawText(TextFormat("Buy cards (%d/%d this turn):", me.cardsBoughtThisTurn, kMaxCardsPerTurn), x, y, 10, GRAY);
            y += 14;
            for (int c = 0; c < kNumCommanders; ++c) {
                Commander cm = static_cast<Commander>(c);
                bool ok = g.commanderInPlay(rq.player, cm) && g.deckSize(cm) > 0 &&
                          static_cast<int>(ui.cart.size()) + me.cardsBoughtThisTurn < kMaxCardsPerTurn &&
                          static_cast<int>(ui.cart.size()) < me.energy;
                if (Button({(float)x + c * 74, (float)y, 70, 24}, TextFormat("%s (%d)", toString(cm), g.deckSize(cm)), ok)) ui.cart.push_back(cm);
            }
            y += 30;
            std::string cartTxt = "Cart:";
            for (Commander cm : ui.cart) cartTxt += std::string(" ") + commanderInitial(cm);
            DrawText(cartTxt.c_str(), x, y + 6, 10, RAYWHITE);
            if (Button({(float)x + 120, (float)y, 60, 24}, "Buy", !ui.cart.empty(), {50, 110, 70, 255})) {
                Result r = g.buyCards(rq.player, ui.cart);
                ui.status = r.ok ? "" : r.error;
                ui.cart.clear();
            }
            if (Button({(float)x + 186, (float)y, 60, 24}, "Clear", !ui.cart.empty())) ui.cart.clear();
            y += 30;
            doneButton("Done buying");
            break;
        }
        case ReqKind::Cards:
            DrawText("Click a card below to play it.", x, y, 10, GRAY);
            y += 16;
            doneButton("Done with cards");
            break;
        case ReqKind::Invade: {
            const Invasion& inv = g.invasion();
            if (!inv.active) {
                if (clicked >= 0) {
                    if (g.territory(clicked).owner == rq.player) {
                        ui.selected = clicked;
                        ui.status.clear();
                    } else if (ui.selected >= 0) {
                        Result r = g.declareInvasion(rq.player, ui.selected, clicked);
                        ui.status = r.ok ? "" : r.error;
                        ui.haveBattle = false;
                        if (r.ok) ui.moveIn = 1;
                    }
                }
                DrawText(ui.selected >= 0 ? TextFormat("From %s: click an orange target", g.map().territory(ui.selected).name.c_str())
                                          : "Click one of your territories to attack from.",
                         x, y, 10, RAYWHITE);
                y += 16;
                doneButton("End invasions");
            } else {
                DrawText(TextFormat("%s (%d) -> %s (%d)", g.map().territory(inv.from).name.c_str(), g.territory(inv.from).units(),
                                    g.map().territory(inv.to).name.c_str(), g.territory(inv.to).units()),
                         x, y, 10, RAYWHITE);
                y += 16;
                if (!inv.captured) {
                    int maxDice = std::min(3, g.territory(inv.from).units() - 1);
                    for (int d = 1; d <= 3; ++d)
                        if (Button({(float)x + (d - 1) * 70, (float)y, 64, 26}, TextFormat("Roll %d", d), d <= maxDice, {110, 50, 50, 255})) {
                            Result r = g.attack(rq.player, d, &ui.lastBattle);
                            ui.status = r.ok ? "" : r.error;
                            ui.haveBattle = r.ok;
                        }
                    if (Button({(float)x + 216, (float)y, 70, 26}, "Retreat", inv.attackedOnce)) {
                        Result r = g.endInvasion(rq.player);
                        ui.status = r.ok ? "" : r.error;
                    }
                    y += 32;
                } else {
                    int maxMove = g.territory(inv.from).units() - 1;
                    int minMove = std::max(1, std::min(inv.lastDice, maxMove));
                    ui.moveIn = std::clamp(ui.moveIn, minMove, std::max(minMove, maxMove));
                    DrawText(TextFormat("Captured! Move in %d units (min %d, max %d)", ui.moveIn, minMove, maxMove), x, y, 10, GOLD);
                    y += 16;
                    if (Button({(float)x, (float)y, 30, 24}, "-", ui.moveIn > minMove)) --ui.moveIn;
                    if (Button({(float)x + 36, (float)y, 30, 24}, "+", ui.moveIn < maxMove)) ++ui.moveIn;
                    if (Button({(float)x + 72, (float)y, 40, 24}, "Max")) ui.moveIn = maxMove;
                    if (Button({(float)x + 120, (float)y, 90, 24}, "Move in", true, {50, 110, 70, 255})) {
                        Result r = g.moveIn(rq.player, ui.moveIn);
                        ui.status = r.ok ? "" : r.error;
                        ui.selected = inv.to;  // keep attacking from the new territory
                    }
                    y += 30;
                }
            }
            if (ui.haveBattle) {
                std::string a = "You:", d = "Def:";
                for (int r : ui.lastBattle.attackRolls) a += " " + std::to_string(r);
                for (int r : ui.lastBattle.defendRolls) d += " " + std::to_string(r);
                if (ui.lastBattle.attackD8) a += TextFormat(" (%dxd8)", ui.lastBattle.attackD8);
                if (ui.lastBattle.defendD8) d += TextFormat(" (%dxd8)", ui.lastBattle.defendD8);
                DrawText(TextFormat("%s   %s   you -%d, they -%d", a.c_str(), d.c_str(), ui.lastBattle.attackerLost,
                                    ui.lastBattle.defenderLost),
                         x, y, 10, LIGHTGRAY);
                y += 16;
            }
            break;
        }
        case ReqKind::Fortify: {
            if (clicked >= 0 && g.territory(clicked).owner == rq.player) {
                if (ui.selected < 0 || clicked == ui.selected) {
                    ui.selected = clicked;
                    ui.fortifyN = std::max(0, g.territory(clicked).units() - 1);
                    ui.fortifyCmds = 0;
                } else {
                    std::array<bool, kNumCommanders> cmds{};
                    for (int c = 0; c < kNumCommanders; ++c) cmds[c] = (ui.fortifyCmds >> c) & 1u;
                    int nCmd = static_cast<int>(std::count(cmds.begin(), cmds.end(), true));
                    Result r = g.fortify(rq.player, ui.selected, clicked, ui.fortifyN - nCmd, cmds);
                    ui.status = r.ok ? "" : r.error;
                    if (r.ok) respond(sh, 0);
                }
            }
            if (ui.selected >= 0) {
                const auto& f = g.territory(ui.selected);
                int maxN = std::max(0, f.units() - 1);
                ui.fortifyN = std::clamp(ui.fortifyN, 0, maxN);
                DrawText(TextFormat("From %s: move %d unit(s). Click a blue destination.", g.map().territory(ui.selected).name.c_str(), ui.fortifyN),
                         x, y, 10, RAYWHITE);
                y += 16;
                if (Button({(float)x, (float)y, 30, 24}, "-", ui.fortifyN > 1)) --ui.fortifyN;
                if (Button({(float)x + 36, (float)y, 30, 24}, "+", ui.fortifyN < maxN)) ++ui.fortifyN;
                int bx = x + 80;
                for (int c = 0; c < kNumCommanders; ++c)
                    if (f.commanders[c]) {
                        bool on = (ui.fortifyCmds >> c) & 1u;
                        if (Button({(float)bx, (float)y, 40, 24}, commanderInitial(static_cast<Commander>(c)), true,
                                   on ? Color{120, 90, 30, 255} : Color{60, 70, 90, 255}))
                            ui.fortifyCmds ^= (1u << c);
                        bx += 44;
                    }
                y += 30;
            } else {
                DrawText("Click the territory to move units from.", x, y, 10, RAYWHITE);
                y += 16;
            }
            doneButton("Skip fortify");
            break;
        }
        case ReqKind::DefenseDice: {
            for (int d = 1; d <= rq.maxDice; ++d)
                if (Button({(float)x + (d - 1) * 90, (float)y, 84, 26}, TextFormat("Defend with %d", d))) respond(sh, d);
            y += 32;
            break;
        }
        case ReqKind::Reactive:
            DrawText("Click a reactive card below, or pass.", x, y, 10, GRAY);
            y += 16;
            if (Button({(float)x, (float)y, 80, 26}, "Pass")) respond(sh, -1);
            y += 32;
            break;
        case ReqKind::BonusDeck:
            for (size_t i = 0; i < rq.decks.size(); ++i)
                if (Button({(float)x + i * 74, (float)y, 70, 24}, toString(rq.decks[i]))) respond(sh, static_cast<int>(i));
            y += 32;
            break;
    }

    if (!ui.status.empty()) {
        DrawWrapped(ui.status, x, y, pw - 24, 10, Color{255, 120, 100, 255});
        y += 28;
    }

    // hand
    int handPlayer = rq.player >= 0 ? rq.player : -1;
    if (handPlayer >= 0) {
        y += 4;
        DrawLine(px, y, kWinW, y, Color{70, 70, 90, 255});
        y += 6;
        const auto& hand = g.player(handPlayer).hand;
        DrawText(TextFormat("Hand (%d)", (int)hand.size()), x, y, 10, GRAY);
        y += 14;
        bool playable = rq.kind == ReqKind::Cards || rq.kind == ReqKind::Reactive ||
                        (rq.kind == ReqKind::Invade && !g.player(rq.player).invasionDeclaredThisTurn);
        for (size_t i = 0; i < hand.size() && y < kWinH - 190; ++i) {
            const CardDef& c = Cards::def(hand[i]);
            Rectangle r{(float)x, (float)y, (float)pw - 24, 30};
            bool hover = CheckCollisionPointRec(mouse, r);
            bool usable = playable && (rq.kind == ReqKind::Reactive ? c.timing == CardTiming::OnInvasionDeclared
                                                                   : c.timing == CardTiming::BeforeFirstInvasion);
            DrawRectangleRec(r, usable ? (hover ? Color{70, 80, 110, 255} : Color{50, 58, 80, 255}) : Color{38, 40, 50, 255});
            DrawRectangleLinesEx(r, 1, kPlayerColors[5]);
            DrawText(TextFormat("%s  [%s %dE]", c.name.c_str(), toString(c.deck), c.cost), x + 4, y + 3, 10, usable ? RAYWHITE : GRAY);
            std::string t = c.text.size() > 62 ? c.text.substr(0, 60) + ".." : c.text;
            DrawText(t.c_str(), x + 4, y + 16, 10, GRAY);
            if (usable && hover && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                if (rq.kind == ReqKind::Reactive) respond(sh, static_cast<int>(i));
                else {
                    Result pr = g.playCard(rq.player, static_cast<int>(i));
                    ui.status = pr.ok ? "" : pr.error;
                }
            }
            y += 33;
        }
    }

    // log
    int logTop = kWinH - 180;
    DrawLine(px, logTop, kWinW, logTop, Color{70, 70, 90, 255});
    const auto& log = g.log();
    int lines = 13;
    int start = std::max(0, static_cast<int>(log.size()) - lines);
    int ly = logTop + 6;
    for (int i = start; i < static_cast<int>(log.size()); ++i) {
        std::string s = log[i];
        if (MeasureText(s.c_str(), 10) > pw - 20) {
            while (!s.empty() && MeasureText((s + "..").c_str(), 10) > pw - 20) s.pop_back();
            s += "..";
        }
        DrawText(s.c_str(), x, ly, 10, i == static_cast<int>(log.size()) - 1 ? RAYWHITE : Color{170, 170, 180, 255});
        ly += 13;
    }
}

// ---------------------------------------------------------------------------
// Engine thread
// ---------------------------------------------------------------------------
static void engineThread(Shared& sh, Game& game, const std::vector<bool>& isHuman) {
    std::unique_lock<std::mutex> lk(sh.mutex);
    sh.engineLock = &lk;
    auto pace = [&] {
        lk.unlock();
        int waited = 0;
        while (!sh.quit && (waited < sh.aiDelayMs || sh.paused)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            waited += 30;
        }
        lk.lock();
    };
    try {
        game.setup();
        for (int y = 0; y < kNumYears && !sh.quit; ++y) {
            game.startYear();
            for (int p : game.turnOrder()) {
                if (sh.quit) break;
                if (!game.isActive(p)) continue;
                game.takeTurn(p);
                if (!isHuman[p]) pace();
            }
        }
        if (!sh.quit) game.finalScoring();
    } catch (const std::exception&) {
        // window closed while waiting for input
    }
    sh.gameOver = true;
}

int main(int argc, char** argv) {
    int players = 3;
    std::vector<int> humans = {0};
    unsigned seed = std::random_device{}();
    std::string shotFile;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string { return (i + 1 < argc) ? argv[++i] : ""; };
        if (a == "--players") players = std::clamp(std::stoi(next()), 2, 5);
        else if (a == "--seed") seed = static_cast<unsigned>(std::stoul(next()));
        else if (a == "--human") {
            humans.clear();
            std::string s = next();
            size_t pos = 0;
            while (pos < s.size()) {
                size_t comma = s.find(',', pos);
                humans.push_back(std::stoi(s.substr(pos, comma - pos)));
                if (comma == std::string::npos) break;
                pos = comma + 1;
            }
        } else if (a == "--spectate") humans.clear();
        else if (a == "--shot") shotFile = next();  // dev aid: save a screenshot after a few seconds and exit
        else {
            printf("Usage: risk2210_gui [--players N] [--human I,J] [--spectate] [--seed S]\n");
            return a == "--help" ? 0 : 1;
        }
    }

    const char* names[] = {"Red", "Blue", "Green", "Yellow", "Black"};
    std::vector<std::string> playerNames(names, names + players);
    Game game(playerNames, seed);
    Shared sh;
    sh.game = &game;

    std::vector<std::unique_ptr<Agent>> agents;
    std::vector<bool> isHuman(players, false);
    for (int p = 0; p < players; ++p) {
        if (std::find(humans.begin(), humans.end(), p) != humans.end()) {
            agents.push_back(std::make_unique<GuiAgent>(sh));
            isHuman[p] = true;
        } else {
            agents.push_back(std::make_unique<RandomAgent>(seed + p + 1));
        }
        game.setAgent(p, agents.back().get());
    }

    g_pos.assign(game.map().size(), Vector2{0, 0});
    for (const auto& np : kPositions) {
        int id = game.map().find(np.name);
        if (id >= 0) g_pos[id] = {np.x, np.y};
    }

    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    InitWindow(kWinW, kWinH, "Risk 2210 A.D.");
    SetTargetFPS(60);

    std::thread worker(engineThread, std::ref(sh), std::ref(game), std::cref(isHuman));
    UiState ui;

    int frames = 0;
    while (!WindowShouldClose()) {
        if (!shotFile.empty() && ++frames == 240) {
            TakeScreenshot(shotFile.c_str());
            break;
        }
        BeginDrawing();
        {
            std::lock_guard<std::mutex> lk(sh.mutex);
            ClearBackground(BLACK);
            drawBoard(game, sh, ui);
            drawPanel(game, sh, ui);
        }
        EndDrawing();
    }
    {
        std::lock_guard<std::mutex> lk(sh.mutex);
        sh.quit = true;
    }
    sh.cv.notify_all();
    worker.join();
    CloseWindow();
    return 0;
}
