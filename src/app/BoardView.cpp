#include "BoardView.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <string>

#include "rlgl.h"

namespace app {

namespace {

struct NodePos {
    const char* name;
    float x, y;
};

// Board layout in world units. Earth occupies roughly 0..1000 x 0..600, the Moon sits below it.
const NodePos kPositions[] = {
    {"Aleutian Empire", 75, 95}, {"Nunavut", 160, 70}, {"Exiled States of America", 310, 55}, {"Alberta", 140, 140},
    {"Canada", 215, 150}, {"Republique du Quebec", 300, 110}, {"Continental Biospheres", 140, 205},
    {"American Republic", 250, 225}, {"Mexitlopoctli", 175, 290},
    {"Nuevo Timoto", 235, 355}, {"Andean Nations", 215, 425}, {"Amazon Desert", 290, 415}, {"Argentina", 245, 500},
    {"Iceland GRC", 400, 110}, {"New Avalon", 405, 175}, {"Jotenheim", 475, 90}, {"Warsaw Republic", 495, 165},
    {"Ukrayina", 575, 140}, {"Andorra", 415, 240}, {"Imperial Balkania", 505, 230},
    {"Saharan Empire", 445, 330}, {"Egypt", 515, 300}, {"Ministry of Djibouti", 565, 375}, {"Zaire Military Zone", 500, 425},
    {"Lesotho", 510, 505}, {"Madagascar", 585, 495},
    {"Middle East", 605, 275}, {"Afghanistan", 645, 205}, {"Enclave of the Bear", 645, 135}, {"Siberia", 705, 80},
    {"Sakha", 785, 60}, {"Pevek", 865, 75}, {"Alden", 765, 135}, {"Khan Industrial State", 785, 195}, {"Japan", 885, 195},
    {"Hong Kong", 735, 255}, {"United Indiastan", 665, 305}, {"Angkhor Wat", 745, 325},
    {"Java Cartel", 785, 415}, {"New Guinea", 875, 405}, {"Aboriginal League", 795, 505}, {"Australian Testing Ground", 900, 525},
    {"Poseidon", 45, 205}, {"Hawaiian Preserve", 55, 285}, {"New Atlantis", 65, 365},
    {"Western Ireland", 350, 215}, {"New York City", 320, 265}, {"Nova Brasilia", 335, 335},
    {"Neo Paulo", 335, 480}, {"The Ivory Reef", 405, 465},
    {"South Ceylon", 640, 385}, {"Microcorp", 640, 455}, {"Akara", 695, 515},
    {"Neo Tokyo", 940, 275}, {"Sung Tzu", 940, 345},
    {"Harpalus", 90, 700}, {"Bay of Dew", 90, 790}, {"Sea of Rains", 185, 705}, {"Ocean of Storms", 195, 800},
    {"Aristotle", 320, 680}, {"Sea of Serenity", 345, 750}, {"Sea of Crisis", 445, 715}, {"Sea of Nectar", 425, 805},
    {"Rhaeticus", 575, 735}, {"Byrgius", 575, 835}, {"Sea of Clouds", 675, 775}, {"Straight Wall", 755, 710},
    {"Marsh of Diseases", 735, 845}, {"Tycho", 850, 800},
};

constexpr float kBoardW = 1000.0f;

Color withAlpha(Color c, float a) { return {c.r, c.g, c.b, static_cast<unsigned char>(std::clamp(a, 0.0f, 1.0f) * 255)}; }

/// Additive-ish soft glow: stacked translucent discs.
void glowDisc(Vector2 p, float radius, Color c, float strength) {
    for (int i = 4; i >= 1; --i) {
        float r = radius * (1.0f + i * 0.45f);
        DrawCircleV(p, r, withAlpha(c, 0.10f * strength / i));
    }
}

void glowLine(Vector2 a, Vector2 b, float width, Color c, float strength) {
    DrawLineEx(a, b, width * 4, withAlpha(c, 0.08f * strength));
    DrawLineEx(a, b, width * 2, withAlpha(c, 0.18f * strength));
    DrawLineEx(a, b, width, withAlpha(c, 0.9f));
}

}  // namespace

BoardView::BoardView(const risk2210::Map& map) : map_(map), pos_(map.size(), Vector2{0, 0}) {
    float minx = 1e9, miny = 1e9, maxx = -1e9, maxy = -1e9;
    for (const auto& np : kPositions) {
        int id = map_.find(np.name);
        if (id < 0) continue;
        pos_[id] = {np.x, np.y};
        minx = std::min(minx, np.x);
        miny = std::min(miny, np.y);
        maxx = std::max(maxx, np.x);
        maxy = std::max(maxy, np.y);
    }
    bounds_ = {minx - 60, miny - 60, maxx - minx + 120, maxy - miny + 120};
}

int BoardView::hitTest(Vector2 w) const {
    int best = -1;
    float bestD = 26.0f;
    for (size_t i = 0; i < pos_.size(); ++i) {
        float d = std::hypot(w.x - pos_[i].x, w.y - pos_[i].y);
        if (d < bestD) {
            bestD = d;
            best = static_cast<int>(i);
        }
    }
    return best;
}

std::vector<int> BoardView::path(int from, int to) const {
    if (from < 0 || to < 0) return {};
    std::vector<int> prev(map_.size(), -1);
    std::vector<bool> seen(map_.size(), false);
    std::queue<int> q;
    q.push(from);
    seen[from] = true;
    while (!q.empty()) {
        int cur = q.front();
        q.pop();
        if (cur == to) break;
        for (int n : map_.territory(cur).adjacent)
            if (!seen[n]) {
                seen[n] = true;
                prev[n] = cur;
                q.push(n);
            }
    }
    if (!seen[to]) return {};
    std::vector<int> out;
    for (int c = to; c != -1; c = prev[c]) out.push_back(c);
    std::reverse(out.begin(), out.end());
    return out;
}

std::vector<Vector2> BoardView::pathPoints(const std::vector<int>& ids) const {
    std::vector<Vector2> pts;
    for (int id : ids) pts.push_back(pos_[id]);
    return pts;
}

Color BoardView::playerColor(int p) {
    static const Color c[] = {{255, 70, 90, 255}, {60, 150, 255, 255}, {70, 230, 120, 255}, {255, 210, 60, 255}, {200, 120, 255, 255}, {140, 150, 170, 255}};
    return c[p < 0 ? 5 : p % 6];
}

Color BoardView::regionColor(int region) {
    static const Color c[] = {
        {250, 220, 90, 255},  {220, 140, 80, 255},  {190, 120, 255, 255}, {120, 230, 230, 255}, {120, 240, 130, 255},
        {255, 120, 110, 255}, {80, 190, 255, 255},  {255, 230, 70, 255},  {255, 110, 80, 255},  {140, 240, 100, 255},
        {255, 170, 70, 255},  {210, 210, 235, 255}, {180, 180, 215, 255}, {150, 150, 190, 255}};
    return c[region % 14];
}

void BoardView::draw(const risk2210::Game* game, int hovered, int selected, float time, const BoardStyle& style) const {
    using namespace risk2210;
    const float R = style.nodeRadius;

    // Moon panel plate
    DrawRectangleRounded({0, 640, kBoardW, 260}, 0.05f, 8, {12, 12, 22, 200});
    DrawRectangleRoundedLinesEx({0, 640, kBoardW, 260}, 0.05f, 8, 1.5f, {120, 120, 180, 120});
    DrawText("THE MOON", 16, 650, 20, {130, 130, 190, 255});

    // edges
    for (int t = 0; t < map_.size(); ++t) {
        for (int n : map_.territory(t).adjacent) {
            if (n < t) continue;
            Vector2 a = pos_[t], b = pos_[n];
            TerrType ta = map_.territory(t).type, tb = map_.territory(n).type;
            Color col = {90, 110, 150, 255};
            if (ta == TerrType::Water || tb == TerrType::Water) col = {60, 150, 240, 255};
            if (ta == TerrType::Moon) col = {140, 140, 200, 255};
            bool hl = (hovered == t || hovered == n || selected == t || selected == n);
            float strength = hl ? 2.2f * style.glowStrength : 0.6f * style.glowStrength;
            if (hl) col = {120, 230, 255, 255};
            if (std::fabs(a.x - b.x) > kBoardW * 0.6f) {  // wrap-around link
                Vector2 l = a.x < b.x ? a : b, r = a.x < b.x ? b : a;
                glowLine(l, {-30, l.y}, 1.5f, col, strength);
                glowLine(r, {kBoardW + 30, r.y}, 1.5f, col, strength);
            } else {
                glowLine(a, b, 1.5f, col, strength);
            }
        }
    }

    // nodes
    for (int t = 0; t < map_.size(); ++t) {
        const auto& info = map_.territory(t);
        Vector2 p = pos_[t];
        Color ring = regionColor(info.region);
        Color fill = {18, 26, 48, 255};
        bool devastated = false;
        int owner = -1, units = 0;
        bool station = false;
        std::string cmds;
        if (game) {
            const auto& ts = game->territory(t);
            devastated = ts.devastated;
            owner = ts.owner;
            units = ts.units();
            station = ts.spaceStation;
            for (int c = 0; c < kNumCommanders; ++c)
                if (ts.commanders[c]) cmds += "LDNXS"[c];
            if (owner >= 0) fill = playerColor(owner);
        }
        float pulse = 0.5f + 0.5f * std::sin(time * 4.0f + t * 0.7f);
        bool hl = hovered == t || selected == t;

        if (devastated) {
            ring = {160, 50, 50, 255};
            fill = {30, 12, 12, 255};
        }
        glowDisc(p, R, ring, (hl ? 2.5f : 0.9f) * style.glowStrength);
        if (info.type == TerrType::Water) DrawRing(p, R + 3, R + 5, 0, 360, 40, {40, 120, 220, 200});
        DrawCircleV(p, R, fill);
        DrawRing(p, R - 3.0f, R, 0, 360, 40, ring);
        if (info.lunarLandingSite) DrawRing(p, R + 5, R + 7, 0, 360, 40, withAlpha({190, 190, 255, 255}, 0.4f + 0.4f * pulse));
        if (hl) DrawRing(p, R + 4, R + 7 + 3 * pulse, 0, 360, 40, withAlpha({120, 235, 255, 255}, 0.85f));
        if (selected == t) DrawRing(p, R + 9, R + 11, 0, 360, 40, {255, 255, 255, 220});

        if (devastated) {
            DrawLineEx({p.x - 9, p.y - 9}, {p.x + 9, p.y + 9}, 3, {230, 70, 70, 255});
            DrawLineEx({p.x - 9, p.y + 9}, {p.x + 9, p.y - 9}, 3, {230, 70, 70, 255});
        } else if (owner >= 0) {
            std::string n = std::to_string(units);
            int w = MeasureText(n.c_str(), 20);
            DrawText(n.c_str(), static_cast<int>(p.x) - w / 2, static_cast<int>(p.y) - 10, 20, WHITE);
            if (!cmds.empty()) DrawText(cmds.c_str(), static_cast<int>(p.x) - 16, static_cast<int>(p.y) - 24, 10, {255, 240, 150, 255});
            if (station) DrawRectangle(static_cast<int>(p.x) + 8, static_cast<int>(p.y) - 24, 10, 10, WHITE);
        } else {
            DrawCircleV(p, 3, withAlpha(ring, 0.6f + 0.4f * pulse));
        }
        int w = MeasureText(info.name.c_str(), 10);
        DrawText(info.name.c_str(), static_cast<int>(p.x) - w / 2, static_cast<int>(p.y + R + 4), 10,
                 hl ? Color{200, 245, 255, 255} : Color{170, 190, 215, 255});
    }
}

void BoardView::drawOrb(const TravelOrb& orb, float time) const {
    if (orb.path.size() < 2) return;
    Vector2 p = orb.position();
    // trail
    for (size_t i = 1; i < orb.trail.size(); ++i) {
        float a = static_cast<float>(i) / orb.trail.size();
        DrawLineEx(orb.trail[i - 1], orb.trail[i], 3.0f * a, withAlpha(orb.color, 0.5f * a));
    }
    float pulse = 0.85f + 0.15f * std::sin(time * 12.0f);
    glowDisc(p, orb.radius * pulse, orb.color, 3.0f);
    DrawCircleV(p, orb.radius * pulse, orb.color);
    DrawCircleV(p, orb.radius * 0.45f, WHITE);
}

}  // namespace app
